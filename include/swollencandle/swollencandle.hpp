#pragma once


#include <algorithm>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <cosevalues/cosevalues.hpp>


namespace swollencandle {


    struct candle {
        std::uint64_t time;
        std::uint32_t period;
        std::uint64_t count;
        double volume;
        double vwap_price;
        double open_price;
        double high_price;
        double low_price;
        double close_price;

        std::strong_ordering operator <=> (candle const& other) const noexcept {
            return time <=> other.time;
        }

        bool operator == (candle const&) const noexcept = default;
        bool operator != (candle const&) const noexcept = default;
    };


    enum class upscale_period {
        minute, hour, day, month, year
    };


    inline std::optional<upscale_period> parse_upscale_period(std::string const& text) noexcept {
        switch(text.size()) {
            case 3:
                if(text == "day")
                    return { upscale_period::day };
                return std::nullopt;
            case 4:
                if(text == "hour")
                    return { upscale_period::hour };
                if(text == "year")
                    return { upscale_period::year };
                return std::nullopt;
            case 5:
                if(text == "month")
                    return { upscale_period::month };
                return std::nullopt;
            case 6:
                if(text == "minute")
                    return { upscale_period::minute };
                return std::nullopt;
            default:
                return std::nullopt;
        }
    }


    inline std::uint32_t seconds_in(upscale_period up) noexcept {
        switch(up) {
            case upscale_period::minute:
                return 60u;
            case upscale_period::hour:
                return 3600u;
            case upscale_period::day:
                return 86400u;
            case upscale_period::month:
                return 2592000u;
            case upscale_period::year:
                return 31104000u;
            default:
                return 0u;
        }
    }


    struct trade {
        std::uint64_t time;
        double amount;
        double price;
    };


    enum class error {
        ok,
        non_constant_period,
        invalid_upscale_period,
        merging_periods_mismatch,
        duplicated_candle,
        mismatched_candles,
        invalid_candle_fields,
        invalid_trade_fields
    };


    class error_category : public std::error_category {
    public:

        char const* name() const noexcept override {
            return "swollencandle";
        }

        std::string message(int code) const noexcept override {
            switch(error(code)) {
                case error::ok:
                    return "Ok";
                case error::non_constant_period:
                    return "Non constant period";
                case error::invalid_upscale_period:
                    return "Invalid upscale_trades period";
                case error::merging_periods_mismatch:
                    return "Merging periods mismatch";
                case error::duplicated_candle:
                    return "Duplicated candle";
                case error::mismatched_candles:
                    return "Mismatched candles";
                case error::invalid_candle_fields:
                    return "Invalid candle fields";
                case error::invalid_trade_fields:
                    return "Invalid trade fields";
                default:
                    return "Unknown";
            }
        }
    };


    inline error_category const error_category_instance;


    inline std::error_code make_error_code(error e) noexcept {
        return {int(e), error_category_instance};
    }


    namespace detail {

        inline bool failed(std::error_code& ec, std::error_code errc) noexcept {
            ec = errc;
            return false;
        }


        bool check_integrity(std::vector<candle> const& candles, std::error_code& ec) noexcept {
            if(candles.empty())
                return true;
            auto last_time = candles.front().time;
            auto const period = candles.front().period;
            for(std::size_t i = 1; i != candles.size(); ++i) {
                auto const& each = candles[i];
                if(each.period != period)
                    return detail::failed(ec, make_error_code(error::non_constant_period));
                last_time = each.time;
            }
            return true;
        }

    } // detail


    bool upscale(std::vector<candle> const& source,
                 std::vector<candle>& result,
                 upscale_period up,
                 std::error_code& ec) {
        if(source.empty())
            return true;
        if(!detail::check_integrity(source, ec))
            return false;
        auto const period = source.front().period;
        auto const period_in_seconds = seconds_in(up);
        if(period_in_seconds % period != 0)
            return detail::failed(ec, make_error_code(error::invalid_upscale_period));
        if(period_in_seconds == period) {
            result.resize(source.size());
            std::copy(std::begin(source), std::end(source), std::begin(result));
            return true;
        }
        auto const candles_to_fit = period_in_seconds / period;
        result.resize(source.size() / candles_to_fit);
        for(std::size_t i = 0, j = 0; i != result.size(); ++i, j += candles_to_fit) {
            std::uint64_t count = source[j].count;
            double volume = source[j].volume;
            double turnover = source[j].vwap_price * source[j].volume;
            double high_price = source[j].high_price;
            double low_price = source[j].low_price;
            for(auto k = j + 1; k != j + candles_to_fit; ++k) {
                count += source[k].count;
                volume += source[k].volume;
                turnover += source[k].volume * source[k].vwap_price;
                if(source[k].high_price > high_price)
                    high_price = source[k].high_price;
                if(source[k].low_price < low_price)
                    low_price = source[k].low_price;
            }
            result[i] = candle {
                source[j].time / period_in_seconds * period_in_seconds,
                period_in_seconds,
                count,
                volume,
                turnover / volume,
                source[j].open_price,
                high_price,
                low_price,
                source[j + candles_to_fit - 1].close_price
            };
        }
        return true;
    }


    bool merge(std::vector<candle> const& x,
               std::vector<candle> const& y,
               std::vector<candle>& z,
               std::error_code& ec) {
        if(!x.empty() && !y.empty() && x.front().period != y.front().period)
            return detail::failed(ec, make_error_code(error::merging_periods_mismatch));
        std::unordered_map<std::uint64_t, candle const*> indexed;
        indexed.reserve(x.size() + y.size());
        for (auto const& each: x) {
            auto const placed = indexed.try_emplace(each.time, &each);
            if(!placed.second)
                return detail::failed(ec, make_error_code(error::duplicated_candle));
        }
        for (auto const& each: y) {
            auto const placed = indexed.try_emplace(each.time, &each);
            if(!placed.second && *placed.first->second != each)
                return detail::failed(ec, make_error_code(error::mismatched_candles));
        }
        std::vector<std::pair<std::uint64_t, candle const*>> sorted;
        sorted.resize(indexed.size());
        std::size_t i = 0;
        for (auto const& [time, ptr]: indexed) {
            sorted[i].first = time;
            sorted[i].second = ptr;
            ++i;
        }
        std::sort(sorted.begin(), sorted.end(),
                  [](auto const& x, auto const& y) { return x.first < y.first; });
        z.resize(sorted.size());
        for (i = 0; i != indexed.size(); ++i)
            z[i] = *sorted[i].second;
        return true;
    }


    bool upscale(std::vector<trade> const& trades,
                 std::vector<candle>& result,
                 upscale_period up,
                 std::error_code& ec) {
        if(trades.empty())
            return true;

        candle candle;
        auto& first_trade = trades.front();
        auto const period_in_seconds = seconds_in(up);
        candle.time = first_trade.time / period_in_seconds * period_in_seconds;
        candle.period = period_in_seconds;
        candle.count = 1;
        candle.volume = first_trade.amount;
        double turnover = first_trade.amount * first_trade.price;
        candle.open_price = first_trade.price;
        candle.high_price = first_trade.price;
        candle.low_price = first_trade.price;
        candle.close_price = first_trade.price;
        for(std::size_t i = 1; i != trades.size(); ++i) {
            auto const& each_trade = trades[i];
            if(each_trade.time >= candle.time + period_in_seconds) {
                candle.vwap_price = turnover / candle.volume;
                result.push_back(candle);
                candle.time = each_trade.time / period_in_seconds * period_in_seconds;
                candle.period = period_in_seconds;
                candle.count = 1;
                candle.volume = each_trade.amount;
                turnover = each_trade.amount * each_trade.price;
                candle.open_price = each_trade.price;
                candle.high_price = each_trade.price;
                candle.low_price = each_trade.price;
                candle.close_price = each_trade.price;
            } else {
                ++candle.count;
                candle.volume += each_trade.amount;
                turnover += each_trade.price * each_trade.amount;
                if(each_trade.price > candle.high_price)
                    candle.high_price = each_trade.price;
                else if (each_trade.price < candle.low_price)
                    candle.low_price = each_trade.price;
                candle.close_price = each_trade.price;
            }
        }
        candle.vwap_price = turnover / candle.volume;
        result.push_back(candle);

        return true;
    }


    bool read(std::string const& filename,
              std::vector<candle>& candles,
              std::error_code& ec) {

        auto maybe_reader = cosevalues::reader::from_file(filename, ec);
        if(!maybe_reader)
            return false;

        auto constexpr line_estimation = 72;
        candles.clear();
        candles.reserve(maybe_reader->text_size() / line_estimation + 1);
        candle candle;
        for(auto& row: maybe_reader->second_to_last_rows()) {
            if(!row.parse(candle.time, candle.period, candle.count, candle.volume,
                          candle.vwap_price, candle.open_price, candle.high_price,
                          candle.low_price, candle.close_price)) {
                ec = make_error_code(error::invalid_candle_fields);
                return false;
            }
            candles.push_back(candle);
        }

        return true;
    }


    bool write(std::string const& filename,
              std::vector<candle> const& candles,
              std::error_code& ec) {
        auto writer = cosevalues::writer();
        auto constexpr line_estimation = 72;
        writer.reserve(candles.size() * line_estimation);
        writer.format("time", "period", "trades", "volume", "vwap_price",
                      "open_price", "high_price", "low_price", "close_price");
        for(auto const& candle: candles)
            writer.format(candle.time, candle.period, candle.count, candle.volume,
                          candle.vwap_price, candle.open_price, candle.high_price,
                          candle.low_price, candle.close_price);
        if(!writer.to_file(filename, ec))
            return false;

        return true;
    }


    bool read(std::string const& filename,
              std::vector<trade>& trades,
              std::error_code& ec) {

        auto maybe_reader = cosevalues::reader::from_file(filename, ec);
        if(!maybe_reader)
            return false;

        auto constexpr line_estimation = 32;
        trades.clear();
        trades.reserve(maybe_reader->text_size() / line_estimation + 1);
        trade trade;
        for(auto& row: maybe_reader->first_to_last_rows()) {
            if(!row.parse(trade.time, trade.price, trade.amount)) {
                ec = make_error_code(error::invalid_trade_fields);
                return false;
            }
            trades.push_back(trade);
        }

        return true;
    }


    bool write(std::string const& filename,
               std::vector<trade> const& trades,
               std::error_code& ec) {
        auto writer = cosevalues::writer();
        auto constexpr line_estimation = 72;
        writer.reserve(trades.size() * line_estimation);
        for(auto const& trade: trades)
            writer.format(trade.time, trade.price, trade.amount);
        if(!writer.to_file(filename, ec))
            return false;

        return true;
    }


}


namespace std {

    template <> struct is_error_code_enum<swollencandle::error> : true_type {};

} // std
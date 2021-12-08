# swollencandle

C++20 single header library to upscale and merge candlesticks


## Dependencies

* [cosevalues](https://github.com/ortfero/cosevalues) - to read/write CSV files

## Install

Place `swollencandle/swollencandle.hpp`, `cosevalues/cosevalues.hpp` at include path

## Snippets

### Candle data type

```cpp
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
```


### Trade data type

```cpp
struct trade {
    std::uint64_t time;
    double amount;
    double price;
};
```


### Upscale period

```cpp
enum class upscale_period {
    minute, hour, day, month, year
};
```


### Parse upscale period

```cpp
std::optional<upscale_period> maybe_upscale_period =
    swollencandle::parse_upscale_period("month");
```


### Upscale trades

```cpp
std::vector<swollencandle::trade> trades;
std::vector<swollencandle::candle> candles;
std::error_code ec;
if(!swollencandle::upscale(trades, candles, swollencandle::upscale_period::day, ec)) {
    std::cerr << ec.message() << '\n';
}
```


### Upscale candlesticks

```cpp
std::vector<swollencandle::candle> source;
std::vector<swollencandle::candle> result;
std::error_code ec;
if(!swollencandle::upscale(source, result, swollencandle::upscale_period::day, ec)) {
    std::cerr << ec.message() << '\n';
}
```


### Merge candlesticks

```cpp
std::vector<swollencandle::candle> x, y;
std::vector<swollencandle::candle> result;
std::error_code ec;
if(!swollencandle::merge(x, y, result, ec)) {
    std::cerr << ec.message() << '\n';
}
```

### Read candlesticks

```cpp
std::vector<swollencandle::candle> candles;
std::error_code ec;
if(!swollencandle::read("candles.csv", candles, ec)) {
    std::cerr << ec.message() << '\n';
}
```

### Write candlesticks

```cpp
std::vector<swollencandle::candle> candles;
std::error_code ec;
if(!swollencandle::write("candles.csv", candles, ec)) {
    std::cerr << ec.message() << '\n';
}
```

### Read trades


/* This file is part of confetti library
 * Copyright (c) 2021 Andrei Ilin <ortfero@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once


#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <vector>


namespace cosevalues {

    class reader;
    
    
    class row {
    friend class reader;
    private:
        
        char const* cursor_{nullptr};

    public:

        row() noexcept = default;
        row(row const&) noexcept = default;
        row& operator = (row const&) noexcept = default;
        
        
        bool operator == (row const& other) const noexcept {
            return cursor_ == other.cursor_;
        }

        
        bool operator != (row const& other) const noexcept {
            return cursor_ != other.cursor_;
        }
        
        
        template<typename Arg, typename... Args>
        bool parse(Arg& arg, Args&... args) {
            if(!parse_args<1 + sizeof...(Args)>(arg, args...))
                return false;
            return true;
        }
      
      
    private:

        row(char const* cursor) noexcept
            : cursor_{cursor}
        { }
        
        
        void skip_line() noexcept {
            for(;;)
                switch(*cursor_) {
                    case '\n':
                        ++cursor_; return;
                    case '\0':
                        return;
                    default:
                        ++cursor_; continue;
                }
                
        }
        
        
        void skip_whitespaces() noexcept {
            for(;;)
                switch(*cursor_) {
                    case ' ': case '\t': case '\r':
                        ++cursor_; continue;
                    default:
                        return;
                }
        }
        
        
        bool scan_quoted(bool& has_inner_quotes) noexcept {
            ++cursor_;
            for(;;)
                switch(*cursor_) {
                    case '\n': case '\0':
                        return false;
                    case '\"':
                        if(*(cursor_ + 1) == '\"') {
                            has_inner_quotes = true;
                            cursor_ += 2;
                            continue;
                        }
                        return true;
                default:
                    ++cursor_;
                    continue;
                }
        }
        
        
        void scan_token() noexcept {
            for(;;)
                switch(*cursor_) {
                    case '\t': case '\r': case '\n': case '\0': case ',':
                        return;
                    default:
                        ++cursor_; continue;
                }
        }
        
        
        template<std::size_t I, typename Arg, typename... Args> bool
        parse_args(Arg& arg, Args&... args) noexcept {
            if(!parse_arg(arg))
                return false;
            if constexpr (I == 1) {
                skip_whitespaces();
                switch(*cursor_) {
                    case '\n': case '\0':
                        return true;
                    default:
                        return false;
                }
            } else {
                skip_whitespaces();
                if(*cursor_ != ',')
                    return false;
                ++cursor_;
                skip_whitespaces();
                if(!parse_args<I - 1>(args...))
                    return false;
            }
            return true;
        }
        
        
        template<typename Arg> bool parse_arg(Arg& arg) noexcept {
            static_assert(std::is_same_v<Arg, std::int32_t> ||
                std::is_same_v<Arg, std::uint32_t> ||
                std::is_same_v<Arg, std::int64_t> ||
                std::is_same_v<Arg, std::uint64_t> ||
                std::is_same_v<Arg, float> ||
                std::is_same_v<Arg, double> ||
                std::is_same_v<Arg, std::string>,
                        "Parsed argument should have integer, floating point or string type");

            if(*cursor_ == '"') {
                char const* mark = cursor_ + 1;
                bool has_inner_quotes = false;
                if(!scan_quoted(has_inner_quotes))
                    return false;
                if constexpr(std::is_same_v<Arg, std::string>) {
                    auto const converted = has_inner_quotes ?
                            try_parse_quoted(mark, cursor_, arg) : try_parse(mark, cursor_, arg);
                    if(!converted)
                        return false;
                } else {
                    if(!try_parse(mark, cursor_, arg))
                        return false;
                }
                ++cursor_;
            } else {
                char const* mark = cursor_;
                scan_token();
                if(mark == cursor_)
                    return false;
                if(!try_parse(mark, cursor_, arg))
                    return false;
            }
            return true;
        }


        static bool try_parse(char const* begin, char const* end, std::int32_t& arg) noexcept {
            auto const converted = std::from_chars(begin, end, arg);
            return converted.ec == std::errc{};
        }


        static bool try_parse(char const* begin, char const* end, std::uint32_t& arg) noexcept {
            auto const converted = std::from_chars(begin, end, arg);
            return converted.ec == std::errc{};
        }


        static bool try_parse(char const* begin, char const* end, std::int64_t& arg) noexcept {
            auto const converted = std::from_chars(begin, end, arg);
            return converted.ec == std::errc{};
        }


        static bool try_parse(char const* begin, char const* end, std::uint64_t& arg) noexcept {
            auto const converted = std::from_chars(begin, end, arg);
            return converted.ec == std::errc{};
        }


        static bool try_parse(char const* begin, char const* end, float& arg) noexcept {
            auto const converted = std::from_chars(begin, end, arg);
            return converted.ec == std::errc{};
        }


        static bool try_parse(char const* begin, char const* end, double& arg) noexcept {
            auto const converted = std::from_chars(begin, end, arg);
            return converted.ec == std::errc{};
        }


        static bool try_parse(char const* begin, char const* end, std::string& arg) noexcept {
            arg = std::string(begin, end);
            return true;
        }


        static bool try_parse_quoted(char const* begin, char const* end, std::string& arg) noexcept {
            arg.clear();
            arg.reserve(std::size_t(end - begin));
            for(char const* p = begin; p != end; ++p) {
                if (*p == '\"')
                    ++p;
                arg.push_back(*p);
            }
            return true;
        }
    }; // row
    
    
    class reader {
    public:
        
        using size_type = std::size_t;
        
        class const_iterator {
            friend class reader;
        private:
            row fields_;
            
        public:
            const_iterator() noexcept = default;
            const_iterator(const_iterator const&) noexcept = default;
            const_iterator& operator = (const_iterator const&) noexcept = default;
            
            bool operator == (const_iterator const& other) const noexcept {
                return fields_ == other.fields_;
            }

            bool operator != (const_iterator const& other) const noexcept {
                return fields_ != other.fields_;
            }

            row& operator * () noexcept { return fields_; }
            row* operator -> () noexcept { return &fields_; }
            
            const_iterator& operator ++ () noexcept {
                fields_.skip_line();
                fields_.skip_whitespaces();
                return *this;
            }
            
            const_iterator operator ++ (int) noexcept {
                const_iterator it{*this};
                fields_.skip_line();
                fields_.skip_whitespaces();
                return it;
            }
            
        private:

            explicit const_iterator(char const* cursor): fields_{cursor} {
                fields_.skip_whitespaces();
            }
        };
        
        
        class range {
        friend class reader;
        public:
            
            range(range const&) noexcept = default;
            range& operator = (range const&) noexcept = default;
            const_iterator begin() const noexcept { return begin_; }
            const_iterator end() const noexcept { return end_; }
            
        private:
            const_iterator begin_;
            const_iterator end_;
            
            range(const_iterator b, const_iterator e): begin_{b}, end_{e} { }
        };
        
        
    private:
        std::string source_;
        
    public:
        
        static std::optional<reader> from_file(std::string const& filename,
                                               std::error_code& ec) {
            reader r;
            if(!r.read_file(filename, ec))
                return std::nullopt;
            return { std::move(r) };
        }


        static reader from_string(std::string text) {
            reader r;
            r.read_string(std::move(text));
            return r;
        }
        
        
        reader() = default;
        reader(reader const&) = default;
        reader& operator = (reader const&) = default;
        reader(reader&&) noexcept = default;
        reader& operator = (reader&&) noexcept = default;
        
        
        bool read_file(std::string const& filename,
                       std::error_code& ec) {
            auto maybe_text = read_to_string(filename, ec);
            if(!maybe_text)
                return false;
            read_string(std::move(*maybe_text));
            return true;
        }


        void read_string(std::string text) {
            source_ = std::move(text);
        }
        
        
        size_type text_size() const noexcept {
            return source_.size();
        }


        row first_row() const noexcept {
            const_iterator begin{source_.data()};
            return begin.fields_;
        }
        
        
        range first_to_last_rows() const noexcept {
            return range{const_iterator{source_.data()},
                         const_iterator{source_.data() + source_.size()}};
        }
        
        
        range second_to_last_rows() const noexcept {
            const_iterator end{source_.data() + source_.size()};
            const_iterator second_line{source_.data()};
            if(second_line == end)
                return range{end, end};
            ++second_line;
            return range{second_line, end};
        }

        
    private:
        
        static std::optional<std::string> read_to_string(std::string const& file_name,
                                                         std::error_code& ec) {
            using namespace std;
            unique_ptr<FILE, int (*)(FILE *)>
                file{fopen(file_name.data(), "rb"), fclose};
            if (!file) {
                ec = std::make_error_code(static_cast<std::errc>(errno));
                return nullopt;
            }
            fseek(file.get(), 0, SEEK_END);
            auto const file_size = std::size_t(ftell(file.get()));
            if (file_size == std::size_t(-1L)) {
                return nullopt;
            }
            fseek(file.get(), 0, SEEK_SET);
            string source;
            source.resize(file_size);
            auto const bytes_read = fread(source.data(), 1, file_size, file.get());
            if (bytes_read != file_size) {
                ec = std::make_error_code(static_cast<std::errc>(errno));
                return nullopt;
            }
            return { std::move(source) };
        }
    }; // reader


    class writer {
    private:
        std::string buffer_;

    public:

        writer() = default;
        writer(writer const&) = default;
        writer& operator = (writer const&) = default;
        writer(writer&&) = default;
        writer& operator = (writer&&) = default;

        void reserve(std::size_t n) {
            buffer_.reserve(nearest_power_of_2(n));
        }


        std::string to_string() {
            return buffer_;
        }


        bool to_file(std::string const& file_name, std::error_code& ec) {
            using namespace std;
            unique_ptr<FILE, int (*)(FILE *)>
                    file{fopen(file_name.data(), "wb+"), fclose};
            if (!file) {
                ec = std::make_error_code(static_cast<std::errc>(errno));
                return false;
            }
            auto const bytes_written = fwrite(buffer_.data(), 1, buffer_.size(), file.get());
            if (bytes_written != buffer_.size()) {
                ec = std::make_error_code(static_cast<std::errc>(errno));
                return false;
            }
            return true;
        }


        template<typename Arg, typename... Args>
        void format(Arg const& arg, Args const&... args) {
            format_args<1 + sizeof...(Args)>(arg, args...);
        }


    private:

        static uint64_t nearest_power_of_2(uint64_t n) {
            if(n < 2)
                return 2;
            n--;
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            n |= n >> 32;
            n++;
            return n;
        }


        char* allocate(std::size_t size) {
            auto const n = buffer_.size();
            auto const new_size = buffer_.size() + size;
            if(new_size > buffer_.capacity())
                buffer_.resize(nearest_power_of_2(new_size));
            else
                buffer_.resize(new_size);
            return buffer_.data() + n;
        }


        void free(char* end) {
            auto const begin = buffer_.data();
            auto const size = std::size_t(end - begin);
            buffer_.resize(size);
        }


        template<std::size_t I, typename Arg, typename... Args> void
        format_args(Arg const& arg, Args const&... args) {
            format_arg(arg);
            if constexpr (I == 1) {
                buffer_.push_back('\n');
            } else {
                buffer_.push_back(',');
                format_args<I - 1>(args...);
            }
        }


        template<std::size_t N> void format_arg(char const (&arg)[N]) {
            auto p = allocate(N + 2);
            *p++ = '\"';
            std::memcpy(p, arg, N - 1);
            p += N - 1;
            *p++ = '\"';
            free(p);
        }


        void format_arg(char const* arg) {
            if(arg == nullptr)
                return;
            auto const n = std::strlen(arg);
            auto p = allocate(n + 2);
            *p++ = '\"';
            std::memcpy(p, arg, n);
            p += n;
            *p++ = '\"';
            free(p);
        }


        void format_arg(std::int32_t arg) {
            auto constexpr int32_digits = 11;
            auto p = allocate(int32_digits);
            auto const converted = std::to_chars(p, p + int32_digits, arg);
            free(converted.ptr);
        }


        void format_arg(std::uint32_t arg) {
            auto constexpr uint32_digits = 10;
            auto p = allocate(uint32_digits);
            auto const converted = std::to_chars(p, p + uint32_digits, arg);
            free(converted.ptr);
        }


        void format_arg(std::int64_t arg) {
            auto constexpr int64_digits = 19;
            auto p = allocate(int64_digits);
            auto const converted = std::to_chars(p, p + int64_digits, arg);
            free(converted.ptr);
        }


        void format_arg(std::uint64_t arg) {
            auto constexpr uint64_digits = 18;
            auto p = allocate(uint64_digits);
            auto const converted = std::to_chars(p, p + uint64_digits, arg);
            free(converted.ptr);
        }


        void format_arg(float arg) {
            auto constexpr float_digits = 16;
            auto p = allocate(float_digits);
            auto const converted = std::to_chars(p, p + float_digits, arg);
            free(converted.ptr);
        }


        void format_arg(double arg) {
            auto constexpr double_digits = 32;
            auto p = allocate(double_digits);
            auto const converted = std::to_chars(p, p + double_digits, arg);
            free(converted.ptr);
        }


    }; // writer
    
    
} // cosevalues

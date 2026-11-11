// Copyright 2017 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of scnlib:
//     https://github.com/eliaskosunen/scnlib

#include <scn/istream.h>
#include <scn/xchar.h>

#include "test_common.h"

#if !SCN_DISABLE_IOSTREAM

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <limits>
#include <locale>
#include <sstream>
#include <string>

#if SCN_POSIX && !SCN_WINDOWS
#include <unistd.h>
#endif

using testing::FieldsAre;

namespace {
// Reads from the streambuf directly, ignoring the state of the stream, like
// `std::istreambuf_iterator`. Constructing a string from an
// `istreambuf_iterator` range triggers a bogus `-Wnull-dereference` in
// libstdc++ with some GCC versions (GCC bug 105580).
template <typename CharT>
std::basic_string<CharT> read_rest(std::basic_istream<CharT>& stream)
{
    using traits = typename std::basic_istream<CharT>::traits_type;
    auto& buffer = *stream.rdbuf();
    std::basic_string<CharT> rest;
    for (auto ch = buffer.sbumpc(); !traits::eq_int_type(ch, traits::eof());
         ch = buffer.sbumpc()) {
        rest.push_back(traits::to_char_type(ch));
    }
    return rest;
}

// Only has the bare minimum of a streambuf:
// no get area, and thus no support for putback
class string_streambuf : public std::streambuf {
public:
    explicit string_streambuf(std::string data) : m_data(std::move(data)) {}

protected:
    int_type underflow() override
    {
        if (m_pos == m_data.size()) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(m_data[m_pos]);
    }
    int_type uflow() override
    {
        auto ch = underflow();
        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            ++m_pos;
        }
        return ch;
    }

    std::string m_data;
    std::size_t m_pos{0};
};

class unbuffered_streambuf : public string_streambuf {
public:
    explicit unbuffered_streambuf(std::string data,
                                  std::streamsize available = 0)
        : string_streambuf(std::move(data)), m_available(available)
    {
    }

private:
    std::streamsize showmanyc() override
    {
        return m_available;
    }

    std::streamsize m_available;
};

// Records the furthest position the stream was asked to read
class tracking_streambuf : public unbuffered_streambuf {
public:
    using unbuffered_streambuf::unbuffered_streambuf;

    std::size_t furthest_read() const
    {
        return m_furthest;
    }

private:
    int_type underflow() override
    {
        m_furthest = std::max(m_furthest, m_pos);
        return unbuffered_streambuf::underflow();
    }

    std::size_t m_furthest{0};
};

// Claims that more characters are available than there are
class overpromising_streambuf : public string_streambuf {
public:
    using string_streambuf::string_streambuf;

private:
    std::streamsize showmanyc() override
    {
        return 100;
    }
    std::streamsize xsgetn(char* s, std::streamsize n) override
    {
        const auto count =
            std::min(static_cast<std::size_t>(n), m_data.size() - m_pos);
        std::copy_n(m_data.data() + m_pos, count, s);
        m_pos += count;
        return static_cast<std::streamsize>(count);
    }
};

// Claims that characters are available, but can't read them in bulk
class no_bulk_read_streambuf : public overpromising_streambuf {
public:
    using overpromising_streambuf::overpromising_streambuf;

private:
    std::streamsize xsgetn(char*, std::streamsize) override
    {
        return 0;
    }
};

// Has no get area, and can't put characters back
// until it's given a budget for them
class gated_putback_streambuf : public unbuffered_streambuf {
public:
    using unbuffered_streambuf::unbuffered_streambuf;

    std::size_t putback_budget{0};

private:
    int_type pbackfail(int_type ch) override
    {
        if (putback_budget == 0 || m_pos == 0) {
            return traits_type::eof();
        }
        if (!traits_type::eq_int_type(ch, traits_type::eof()) &&
            !traits_type::eq(traits_type::to_char_type(ch),
                             m_data[m_pos - 1])) {
            return traits_type::eof();
        }
        --putback_budget;
        --m_pos;
        return traits_type::to_int_type(m_data[m_pos]);
    }
};

enum class seek_behavior {
    none,
    correct,
    // Relative seeks report success, but don't move
    relative_doesnt_move,
    // Relative backward seeks report success, but stop one character short
    relative_one_short,
};

// Reads its contents through a get area of at most `chunk_size` characters.
// Like libc++'s `filebuf`, only `putback_limit` characters before the get area
// can be put back after a refill. This also applies at the end of the contents.
template <typename CharT>
class limited_putback_streambuf : public std::basic_streambuf<CharT> {
    using base = std::basic_streambuf<CharT>;

public:
    using typename base::int_type;
    using typename base::off_type;
    using typename base::pos_type;
    using typename base::traits_type;

    limited_putback_streambuf(std::basic_string<CharT> data,
                              std::size_t putback_limit,
                              seek_behavior seek,
                              std::size_t chunk_size = 4096)
        : m_data(std::move(data)),
          m_putback_limit(putback_limit),
          m_chunk_size(chunk_size),
          m_seek(seek)
    {
        set_position(0);
    }

    int seekoff_calls{0};
    // Seeking to a saved position fails
    bool seekpos_fails{false};

protected:
    int_type underflow() override
    {
        const auto pos = position();
        const auto begin = pos - std::min(pos, m_putback_limit);
        const auto end = std::min(m_data.size(), pos + m_chunk_size);
        this->setg(m_data.data() + begin, m_data.data() + pos,
                   m_data.data() + end);
        if (pos == m_data.size()) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(m_data[pos]);
    }

    pos_type seekoff(off_type off,
                     std::ios_base::seekdir dir,
                     std::ios_base::openmode which) override
    {
        ++seekoff_calls;
        if (m_seek == seek_behavior::none || dir != std::ios_base::cur ||
            (which & std::ios_base::in) == 0) {
            return pos_type(off_type(-1));
        }
        const auto current = static_cast<off_type>(position());
        const auto requested = current + off;
        if (requested < 0 || requested > static_cast<off_type>(m_data.size())) {
            return pos_type(off_type(-1));
        }
        auto actual = requested;
        if (m_seek == seek_behavior::relative_doesnt_move) {
            actual = current;
        }
        else if (m_seek == seek_behavior::relative_one_short && off < 0) {
            actual = requested + 1;
        }
        set_position(static_cast<std::size_t>(actual));
        return pos_type(requested);
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
    {
        const auto offset = static_cast<off_type>(pos);
        if (seekpos_fails || m_seek == seek_behavior::none ||
            (which & std::ios_base::in) == 0 || offset < 0 ||
            offset > static_cast<off_type>(m_data.size())) {
            return pos_type(off_type(-1));
        }
        set_position(static_cast<std::size_t>(offset));
        return pos;
    }

private:
    std::size_t position() const
    {
        return static_cast<std::size_t>(this->gptr() - m_data.data());
    }
    void set_position(std::size_t pos)
    {
        auto* ptr = m_data.data() + pos;
        this->setg(ptr, ptr, ptr);
    }

    std::basic_string<CharT> m_data;
    std::size_t m_putback_limit;
    std::size_t m_chunk_size;
    seek_behavior m_seek;
};

// A wide-to-narrow conversion with the given `encoding()`:
// 0 means variable-width, like UTF-8
class codecvt_with_encoding
    : public std::codecvt<wchar_t, char, std::mbstate_t> {
public:
    explicit codecvt_with_encoding(int encoding) : m_encoding(encoding) {}

protected:
    int do_encoding() const noexcept override
    {
        return m_encoding;
    }

private:
    int m_encoding;
};

template <typename CharT>
std::basic_string<CharT> rest_of(std::basic_streambuf<CharT>& buf)
{
    std::basic_string<CharT> result;
    using traits = std::char_traits<CharT>;
    for (auto ch = buf.sbumpc(); !traits::eq_int_type(ch, traits::eof());
         ch = buf.sbumpc()) {
        result.push_back(traits::to_char_type(ch));
    }
    return result;
}

// The characters that haven't been scanned: the unsynced ones in the wrapper,
// and the rest of the stream buffer
template <typename CharT>
std::basic_string<CharT> unscanned(scn::basic_scan_istream<CharT>& wrapper,
                                   std::basic_streambuf<CharT>& buf)
{
    return std::basic_string<CharT>{wrapper.unsynced()} + rest_of(buf);
}

// Removes the file when destroyed
class temporary_file_path {
public:
    temporary_file_path(const std::string& name, const std::string& contents)
        : m_path(testing::TempDir() + name)
    {
        std::ofstream out{m_path, std::ios::binary};
        out << contents;
    }

    ~temporary_file_path()
    {
        std::remove(m_path.c_str());
    }

    temporary_file_path(const temporary_file_path&) = delete;
    temporary_file_path& operator=(const temporary_file_path&) = delete;

    const std::string& path() const
    {
        return m_path;
    }

private:
    std::string m_path;
};
}  // namespace

TEST(IstreamSourceTest, Stringstream)
{
    std::istringstream ss{"123 abc"};
    auto res = scn::scan<int, std::string>(ss, "{} {}");
    ASSERT_THAT(res, Succeeded());
    EXPECT_THAT(res->values(), FieldsAre(123, "abc"));
    EXPECT_EQ(&res->source(), &ss);
}

TEST(IstreamSourceTest, StringstreamMultiline)
{
    std::istringstream ss{"123\n456\n789"};
    auto res1 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res1, Succeeded());
    EXPECT_EQ(res1->value(), 123);

    auto res2 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res2, Succeeded());
    EXPECT_EQ(res2->value(), 456);

    auto res3 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res3, Succeeded());
    EXPECT_EQ(res3->value(), 789);
}

TEST(IstreamSourceTest, StringstreamEOF)
{
    std::istringstream ss{"42"};
    auto res1 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res1, Succeeded());
    EXPECT_EQ(res1->value(), 42);

    auto res2 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res2, FailedWith(scn::scan_error::end_of_input));
}

TEST(IstreamSourceTest, EmptyStringstream)
{
    std::istringstream ss{""};
    auto res = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res, FailedWith(scn::scan_error::end_of_input));
}

TEST(IstreamSourceTest, StringstreamWithWhitespace)
{
    std::istringstream ss{"  42  "};
    auto res = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res, Succeeded());
    EXPECT_EQ(res->value(), 42);
}

TEST(IstreamSourceTest, StringstreamChar)
{
    std::istringstream ss{"abc"};
    auto res = scn::scan<char, char, char>(ss, "{}{}{}");
    ASSERT_THAT(res, Succeeded());
    auto [a, b, c] = res->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
    EXPECT_EQ(c, 'c');
}

TEST(IstreamSourceTest, StringstreamBool)
{
    std::istringstream ss{"true false"};
    auto res = scn::scan<bool, bool>(ss, "{} {}");
    ASSERT_THAT(res, Succeeded());
    auto [a, b] = res->values();
    EXPECT_TRUE(a);
    EXPECT_FALSE(b);
}

TEST(IstreamSourceTest, StringstreamNegativeDouble)
{
    std::istringstream ss{"-3.14"};
    auto res = scn::scan<double>(ss, "{}");
    ASSERT_THAT(res, Succeeded());
    EXPECT_NEAR(res->value(), -3.14, 0.001);
}

TEST(IstreamSourceTest, StringstreamMixedTypes)
{
    std::istringstream ss{"42 hello 3.14 true"};
    auto res = scn::scan<int, std::string, double, bool>(ss, "{} {} {} {}");
    ASSERT_THAT(res, Succeeded());
    auto [i, s, d, b] = res->values();
    EXPECT_EQ(i, 42);
    EXPECT_EQ(s, "hello");
    EXPECT_NEAR(d, 3.14, 0.001);
    EXPECT_TRUE(b);
}

TEST(IstreamSourceTest, NullStreambuf)
{
    std::istream stream{nullptr};
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int>(wrapper, "{}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
}

TEST(IstreamSourceTest, StreambufWithHugeAvailableCount)
{
    unbuffered_streambuf buf{"12 34",
                             std::numeric_limits<std::streamsize>::max()};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 12);
    EXPECT_EQ(b, 34);
}

TEST(IstreamSourceTest, StreambufWithHugeAvailableCountLongInput)
{
    const std::string word(10000, 'a');
    unbuffered_streambuf buf{word + " 34",
                             std::numeric_limits<std::streamsize>::max()};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<std::string, int>(wrapper, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [str, num] = result->values();
    EXPECT_EQ(str, word);
    EXPECT_EQ(num, 34);
}

TEST(IstreamSourceTest, StreambufWithoutPutbackConsecutiveScans)
{
    unbuffered_streambuf buf{"12 34"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};

    auto first = scn::scan<int>(wrapper, "{}");
    ASSERT_THAT(first, Succeeded());
    EXPECT_EQ(first->value(), 12);

    auto second = scn::scan<int>(wrapper, "{}");
    ASSERT_THAT(second, Succeeded());
    EXPECT_EQ(second->value(), 34);
}

TEST(IstreamSourceTest, StreambufWithoutPutbackFailedScan)
{
    unbuffered_streambuf buf{"12 x tail"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};

    // The characters read by the failed scan can't be put back,
    // so they are stored in the wrapper
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_FALSE(wrapper.unsynced().empty());
    EXPECT_FALSE(wrapper.stream().has_value());
    EXPECT_EQ(unscanned(wrapper, buf), "12 x tail");
}

// Putting back what a failed scan read falls short.
// The rest is restored by seeking back.
TEST(IstreamSourceTest, SeekableStreambufWithLimitedPutbackFailedScan)
{
    struct test_case {
        std::string data;
        std::size_t chunk_size;
        scn::scan_error::code_t error;
    };
    const std::array<test_case, 2> test_cases{{
        // Reads to the end: only one character can be put back
        {"12", 4096, scn::scan_error::end_of_input},
        // A refill in the middle: the first character can't be put back
        {"12 x tail", 2, scn::scan_error::invalid_scanned_value},
    }};
    for (const auto& [data, chunk_size, error] : test_cases) {
        limited_putback_streambuf<char> buf{data, 1, seek_behavior::correct,
                                            chunk_size};
        std::istream stream{&buf};
        scn::scan_istream wrapper{stream};
        auto result = scn::scan<int, int>(wrapper, "{} {}");
        ASSERT_THAT(result, FailedWith(error)) << data;
#if !SCN_WINDOWS
        EXPECT_TRUE(wrapper.unsynced().empty()) << data;
#endif
        EXPECT_EQ(unscanned(wrapper, buf), data);
    }
}

TEST(IstreamSourceTest, NonSeekableStreambufWithLimitedPutbackFailedScan)
{
    limited_putback_streambuf<char> buf{"12", 1, seek_behavior::none};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
    EXPECT_EQ(wrapper.unsynced(), "1");
    EXPECT_EQ(rest_of(buf), "2");
}

// A seek that doesn't end up where it should is undone
TEST(IstreamSourceTest, LyingSeekableStreambufWithLimitedPutbackFailedScan)
{
    for (auto seek : {seek_behavior::relative_doesnt_move,
                      seek_behavior::relative_one_short}) {
        // After a seek one character short, the characters read again are the
        // same as the ones that should be put back. Only the position after
        // reading them tells the difference.
        limited_putback_streambuf<char> buf{"1111111", 1, seek};
        std::istream stream{&buf};
        scn::scan_istream wrapper{stream};
        auto result = scn::scan<int, int>(wrapper, "{} {}");
        ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
        // As much as without seeking
        EXPECT_EQ(wrapper.unsynced(), "111111");
        EXPECT_EQ(rest_of(buf), "1");
    }
}

// If undoing the seek fails, the position is unknown
TEST(IstreamSourceTest, SeekableStreambufWithFailingUndoSeek)
{
#if SCN_WINDOWS
    GTEST_SKIP() << "No seeking on Windows";
#else
    limited_putback_streambuf<char> buf{"1111111", 1,
                                        seek_behavior::relative_one_short};
    buf.seekpos_fails = true;
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
    EXPECT_FALSE(wrapper.unsynced().empty());
    EXPECT_FALSE(wrapper.sync());
    EXPECT_FALSE(wrapper.unsynced().empty());
#endif
}

// Positions in a variable-width encoding (`encoding() == 0`) aren't
// character counts, so no seek is attempted
TEST(IstreamSourceTest, WideStreambufWithVariableWidthEncoding)
{
    limited_putback_streambuf<wchar_t> buf{L"12", 1, seek_behavior::correct};
    buf.pubimbue(
        std::locale(std::locale::classic(), new codecvt_with_encoding{0}));
    std::wistream stream{&buf};
    scn::wscan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, L"{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
    EXPECT_EQ(buf.seekoff_calls, 0);
    EXPECT_EQ(wrapper.unsynced(), L"1");
    EXPECT_EQ(rest_of(buf), L"2");
}

TEST(IstreamSourceTest, WideStreambufWithFixedWidthEncoding)
{
    limited_putback_streambuf<wchar_t> buf{L"12", 1, seek_behavior::correct};
    buf.pubimbue(
        std::locale(std::locale::classic(), new codecvt_with_encoding{1}));
    std::wistream stream{&buf};
    scn::wscan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, L"{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
#if !SCN_WINDOWS
    EXPECT_TRUE(wrapper.unsynced().empty());
#endif
    EXPECT_EQ(unscanned(wrapper, buf), L"12");
}

// libc++'s `filebuf` can put back only one character after reaching the end
TEST(IstreamSourceTest, IfstreamFailedScanAtEnd)
{
#if SCN_WINDOWS
    GTEST_SKIP() << "No seeking on Windows, and putback is limited there";
#else
    temporary_file_path file{"scn_istream_source_test_ifstream.txt", "12"};
    std::ifstream stream{file.path(), std::ios::binary};
    ASSERT_TRUE(stream.is_open());
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
    EXPECT_TRUE(wrapper.unsynced().empty());

    std::string rest = read_rest(stream);
    EXPECT_EQ(rest, "12");
#endif
}

// A scan that fails after reading across several refills of the tiny `filebuf`
// buffer has to put back characters across a refill
TEST(IstreamSourceTest, IfstreamFailedScanAcrossBufferRefill)
{
#if SCN_WINDOWS
    GTEST_SKIP() << "No seeking on Windows, and putback is limited there";
#else
    const std::string input = "1234567890 abcdefghijklmnop";
    temporary_file_path file{"scn_istream_source_test_refill.txt", input};
    std::ifstream stream;
    char buffer[4];
    stream.rdbuf()->pubsetbuf(buffer, sizeof(buffer));
    stream.open(file.path(), std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_TRUE(wrapper.unsynced().empty());

    std::string rest = read_rest(stream);
    EXPECT_EQ(rest, input);
#endif
}

TEST(IstreamSourceTest, StreambufOverpromisingAvailableCharacters)
{
    overpromising_streambuf buf{"12 34"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 12);
    EXPECT_EQ(b, 34);
}

TEST(IstreamSourceTest, StreambufWithoutBulkReads)
{
    no_bulk_read_streambuf buf{"12 34"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 12);
    EXPECT_EQ(b, 34);
}

TEST(IstreamSourceTest, WidthLongerThanValue)
{
    std::istringstream ss{"12\n"};
    auto result = scn::scan<int>(ss, "{:5}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::length_too_short));
}

TEST(IstreamSourceTest, WidthLeavesTheRest)
{
    std::istringstream ss{"abcdef"};
    auto result = scn::scan<std::string>(ss, "{:3}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "abc");

    std::string rest;
    std::getline(ss, rest);
    EXPECT_EQ(rest, "def");
}

// Like stdin: reading past the field could wait for more input
TEST(IstreamSourceTest, WidthDoesntReadPastTheField)
{
    for (auto fmt : {"{:3}", "{:.3}", "{:3c}"}) {
        tracking_streambuf buf{"abcdef"};
        std::istream stream{&buf};
        scn::scan_istream wrapper{stream};
        auto result = scn::scan<std::string>(wrapper, scn::runtime_format(fmt));
        ASSERT_THAT(result, Succeeded()) << fmt;
        EXPECT_EQ(result->value(), "abc") << fmt;
        EXPECT_EQ(buf.furthest_read(), 2u) << fmt;
    }

    // U+00E4 is two bytes
    tracking_streambuf buf{"a\xc3\xa4zz"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<std::string>(wrapper, "{:2}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "a\xc3\xa4");
    EXPECT_EQ(buf.furthest_read(), 2u);
}

TEST(IstreamSourceTest, SequentialSingleFieldsWithSpecs)
{
    // "{:...}" takes the single-field fast path, "{0:...}" never does.
    // Both must leave the stream at the same position after each scan.
    constexpr std::array<const char*, 2> field_prefixes{"{:", "{0:"};
    for (auto field_prefix : field_prefixes) {
        std::istringstream ss{"ff 12abc  777 xyz"};
        auto format = [&](std::string_view specs) {
            return std::string{field_prefix} + std::string{specs} + "}";
        };

        auto hex = scn::scan<int>(ss, scn::runtime_format(format("x")));
        ASSERT_THAT(hex, Succeeded()) << field_prefix;
        EXPECT_EQ(hex->value(), 0xff);

        auto dec = scn::scan<int>(ss, scn::runtime_format(format(".3d")));
        ASSERT_THAT(dec, Succeeded()) << field_prefix;
        EXPECT_EQ(dec->value(), 12);

        auto str =
            scn::scan<std::string>(ss, scn::runtime_format(format("[a-c]")));
        ASSERT_THAT(str, Succeeded()) << field_prefix;
        EXPECT_EQ(str->value(), "abc");

        auto oct = scn::scan<int>(ss, scn::runtime_format(format("o")));
        ASSERT_THAT(oct, Succeeded()) << field_prefix;
        EXPECT_EQ(oct->value(), 0777);

        auto failed = scn::scan<int>(ss, scn::runtime_format(format("d")));
        EXPECT_THAT(failed, FailedWith(scn::scan_error::invalid_scanned_value))
            << field_prefix;

        std::string rest;
        std::getline(ss, rest);
        EXPECT_EQ(rest, " xyz") << field_prefix;
    }
}

TEST(IstreamSourceTest, UnsyncedCharactersAreReadByTheNextScan)
{
    gated_putback_streambuf buf{"12 x tail"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};

    auto failed = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(failed, FailedWith(scn::scan_error::invalid_scanned_value));
    ASSERT_FALSE(wrapper.unsynced().empty());

    auto result = scn::scan<int, std::string>(wrapper, "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(12, "x"));
    EXPECT_EQ(&result->source(), &wrapper);
}

TEST(IstreamSourceTest, SyncGivesTheUnsyncedCharactersBack)
{
    gated_putback_streambuf buf{"12 x tail"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};

    EXPECT_TRUE(wrapper.sync());

    ASSERT_THAT((scn::scan<int, int>(wrapper, "{} {}")), Failed());
    const std::string unsynced{wrapper.unsynced()};
    ASSERT_FALSE(unsynced.empty());

    // Nothing can be put back yet
    EXPECT_FALSE(wrapper.sync());
    EXPECT_EQ(wrapper.unsynced(), unsynced);
    EXPECT_FALSE(wrapper.stream().has_value());

    buf.putback_budget = 100;
    EXPECT_TRUE(wrapper.sync());
    EXPECT_TRUE(wrapper.unsynced().empty());
    ASSERT_TRUE(wrapper.stream().has_value());
    EXPECT_EQ(*wrapper.stream(), &stream);
    EXPECT_EQ(rest_of(buf), "12 x tail");
}

TEST(IstreamSourceTest, SyncPartially)
{
    gated_putback_streambuf buf{"12 x tail"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};

    ASSERT_THAT((scn::scan<int, int>(wrapper, "{} {}")), Failed());
    const auto n_unsynced = wrapper.unsynced().size();
    ASSERT_GE(n_unsynced, 2u);

    // The characters that were read last are put back first
    buf.putback_budget = 1;
    EXPECT_FALSE(wrapper.sync());
    EXPECT_EQ(wrapper.unsynced().size(), n_unsynced - 1);
    EXPECT_FALSE(wrapper.stream().has_value());
    EXPECT_EQ(unscanned(wrapper, buf), "12 x tail");
}

TEST(IstreamSourceTest, ReleaseUnsynced)
{
    gated_putback_streambuf buf{"12 x tail"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};

    EXPECT_TRUE(wrapper.release_unsynced().empty());

    ASSERT_THAT((scn::scan<int, int>(wrapper, "{} {}")), Failed());
    const std::string unsynced{wrapper.unsynced()};
    ASSERT_FALSE(unsynced.empty());

    EXPECT_EQ(wrapper.release_unsynced(), unsynced);
    EXPECT_TRUE(wrapper.unsynced().empty());
    ASSERT_TRUE(wrapper.stream().has_value());
    EXPECT_EQ(*wrapper.stream(), &stream);
    EXPECT_EQ(unsynced + rest_of(buf), "12 x tail");
}

TEST(IstreamSourceTest, MovedFromWrapperHasNoStream)
{
    gated_putback_streambuf buf{"12 x tail"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    ASSERT_THAT((scn::scan<int, int>(wrapper, "{} {}")), Failed());
    const std::string unsynced{wrapper.unsynced()};
    ASSERT_FALSE(unsynced.empty());

    scn::scan_istream moved{std::move(wrapper)};
    EXPECT_EQ(moved.unsynced(), unsynced);
    EXPECT_FALSE(moved.stream().has_value());
    EXPECT_TRUE(wrapper.unsynced().empty());
    ASSERT_TRUE(wrapper.stream().has_value());
    EXPECT_EQ(*wrapper.stream(), nullptr);

    scn::scan_istream assigned{stream};
    assigned = std::move(moved);
    EXPECT_EQ(assigned.unsynced(), unsynced);
    EXPECT_TRUE(moved.unsynced().empty());
    ASSERT_TRUE(moved.stream().has_value());
    EXPECT_EQ(*moved.stream(), nullptr);

    auto result = scn::scan<int>(wrapper, "{}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
    // Nothing to give back
    EXPECT_TRUE(wrapper.sync());
}

TEST(IstreamSourceTest, SelfMove)
{
    gated_putback_streambuf buf{"12 x tail"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    ASSERT_THAT((scn::scan<int, int>(wrapper, "{} {}")), Failed());
    const std::string unsynced{wrapper.unsynced()};

    // Through a reference, so that `-Wself-move` doesn't fire
    auto& alias = wrapper;
    wrapper = std::move(alias);
    EXPECT_EQ(wrapper.unsynced(), unsynced);
    EXPECT_FALSE(wrapper.stream().has_value());
}

TEST(IstreamSourceTest, NullStreambufKeepsTheUnsyncedCharacters)
{
    gated_putback_streambuf buf{"12 x tail"};
    std::istream stream{&buf};
    scn::scan_istream wrapper{stream};
    ASSERT_THAT((scn::scan<int, int>(wrapper, "{} {}")), Failed());
    const std::string unsynced{wrapper.unsynced()};
    ASSERT_FALSE(unsynced.empty());

    stream.rdbuf(nullptr);
    auto result = scn::scan<int>(wrapper, "{}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
    EXPECT_EQ(wrapper.unsynced(), unsynced);
    EXPECT_FALSE(wrapper.sync());
    EXPECT_EQ(wrapper.unsynced(), unsynced);

    stream.rdbuf(&buf);
    auto success = scn::scan<int, std::string>(wrapper, "{} {}");
    ASSERT_THAT(success, Succeeded());
    EXPECT_THAT(success->values(), FieldsAre(12, "x"));
}

TEST(IstreamSourceTest, WideWrapperSync)
{
    limited_putback_streambuf<wchar_t> buf{L"12", 1, seek_behavior::none};
    std::wistream stream{&buf};
    scn::wscan_istream wrapper{stream};
    EXPECT_TRUE(wrapper.sync());

    auto result = scn::scan<int, int>(wrapper, L"{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
    EXPECT_EQ(wrapper.unsynced(), L"1");

    // Can't put back, since there's no room before the get area
    EXPECT_FALSE(wrapper.sync());
    EXPECT_EQ(wrapper.unsynced(), L"1");
    EXPECT_EQ(wrapper.release_unsynced(), L"1");
    EXPECT_EQ(rest_of(buf), L"2");
}

TEST(IstreamSourceTest, WrapperResultSource)
{
    std::istringstream ss{"12 34"};
    scn::scan_istream wrapper{ss};
    auto result = scn::scan<int>(wrapper, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(&result->source(), &wrapper);
    static_assert(
        std::is_same_v<decltype(result->source()), scn::scan_istream&>);

    // The result type doesn't depend on the value category
    static_assert(std::is_same_v<scn::vscan_result<scn::scan_istream&>,
                                 scn::vscan_result<scn::scan_istream>>);
}

TEST(IstreamSourceTest, WideStringstreamResultSource)
{
    std::wistringstream ss{L"12 34"};
    auto result = scn::scan<int>(ss, L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(&result->source(), &ss);
    static_assert(
        std::is_same_v<decltype(result->source()), std::wistringstream&>);
}

TEST(IstreamSourceTest, StringstreamWithBothDirections)
{
    std::stringstream ss{"12 34"};
    auto result = scn::scan<int, int>(ss, "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(12, 34));
    EXPECT_EQ(&result->source(), &ss);
}

TEST(IstreamSourceTest, FailedStringstreamScanLosesNothing)
{
    std::istringstream ss{"12 x tail"};
    auto result = scn::scan<int, int>(ss, "{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_EQ(ss.str(), "12 x tail");
    std::string rest;
    std::getline(ss, rest);
    EXPECT_EQ(rest, "12 x tail");
}

namespace {
struct derived_stringstream : std::istringstream {
    using std::istringstream::istringstream;
};
}  // namespace

TEST(IstreamSourceTest, DerivedStringstream)
{
    derived_stringstream ss{"12 34"};
    auto result = scn::scan<int, int>(ss, "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(12, 34));
    EXPECT_EQ(&result->source(), &ss);
    static_assert(
        std::is_same_v<decltype(result->source()), derived_stringstream&>);
}

// The stream is read through the string stream's own buffer
TEST(IstreamSourceTest, StringstreamWithReplacedBuffer)
{
    std::istringstream ss{"12 34"};
    std::istringstream other{"56 78"};
    // Replaces the buffer seen by the `std::istream` base class,
    // but `ss.rdbuf()` is still the `std::stringbuf`
    ss.std::ios::rdbuf(other.rdbuf());
    auto result = scn::scan<int, int>(ss, "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(12, 34));
}

#if defined(__cpp_lib_spanstream)
TEST(IstreamSourceTest, Spanstream)
{
    const char data[] = "12 34 56";
    std::ispanstream in{std::span<const char>{data, sizeof(data) - 1}};
    auto result = scn::scan<int, int>(in, "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(12, 34));
    EXPECT_EQ(&result->source(), &in);

    char buffer[] = "78 90";
    std::spanstream both{std::span<char>{buffer, sizeof(buffer) - 1}};
    auto both_result = scn::scan<int, int>(both, "{} {}");
    ASSERT_THAT(both_result, Succeeded());
    EXPECT_THAT(both_result->values(), FieldsAre(78, 90));
}
#endif

#if SCN_POSIX && !SCN_WINDOWS
namespace {
// The read end of a pipe, opened as a file with `/dev/fd`,
// so that it can be given to a `std::basic_ifstream`
class pipe_with_contents {
public:
    explicit pipe_with_contents(const std::string& contents)
    {
        int fds[2];
        if (::pipe(fds) != 0) {
            return;
        }
        m_read_fd = fds[0];
        // The contents must fit in the pipe (16 KiB on macOS), because nothing
        // reads from it yet
        if (::write(fds[1], contents.data(), contents.size()) !=
            static_cast<::ssize_t>(contents.size())) {
            m_read_fd = -1;
        }
        ::close(fds[1]);
    }

    ~pipe_with_contents()
    {
        if (m_read_fd != -1) {
            ::close(m_read_fd);
        }
    }

    pipe_with_contents(const pipe_with_contents&) = delete;
    pipe_with_contents& operator=(const pipe_with_contents&) = delete;

    std::string path() const
    {
        return "/dev/fd/" + std::to_string(m_read_fd);
    }

    bool valid() const
    {
        return m_read_fd != -1;
    }

private:
    int m_read_fd{-1};
};

// Whether the file descriptor of a `std::basic_filebuf` is accessible
constexpr bool has_filebuf_descriptor =
#if defined(__cpp_lib_fstream_native_handle) || defined(_LIBCPP_VERSION)
    true;
#else
    false;
#endif

template <typename CharT>
void expect_filebuf_seekability(const std::basic_filebuf<CharT>& filebuf,
                                bool expected)
{
    const auto result = scn::detail::is_seekable_filebuf(filebuf);
    if (has_filebuf_descriptor) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, expected);
    }
    else if (result) {
        EXPECT_EQ(*result, expected);
    }
}

class derived_ifstream : public std::ifstream {
public:
    using std::ifstream::ifstream;
};
}  // namespace

TEST(IstreamSourceTest, SeekableFilebufRegularFile)
{
    temporary_file_path file{"scn_istream_source_test_seekable.txt", "12"};
    std::ifstream stream{file.path(), std::ios::binary};
    ASSERT_TRUE(stream.is_open());
    expect_filebuf_seekability(*stream.rdbuf(), true);

    std::wifstream wstream{file.path(), std::ios::binary};
    ASSERT_TRUE(wstream.is_open());
    expect_filebuf_seekability(*wstream.rdbuf(), true);
}

TEST(IstreamSourceTest, SeekableFilebufCharacterDevice)
{
    std::ifstream stream{"/dev/null", std::ios::binary};
    if (!stream.is_open()) {
        GTEST_SKIP() << "Can't open /dev/null";
    }
    expect_filebuf_seekability(*stream.rdbuf(), false);

    std::wifstream wstream{"/dev/null", std::ios::binary};
    ASSERT_TRUE(wstream.is_open());
    expect_filebuf_seekability(*wstream.rdbuf(), false);
}

TEST(IstreamSourceTest, SeekableFilebufPipe)
{
    pipe_with_contents pipe{"12"};
    ASSERT_TRUE(pipe.valid());
    std::ifstream stream{pipe.path(), std::ios::binary};
    if (!stream.is_open()) {
        GTEST_SKIP() << "Can't open " << pipe.path();
    }
    expect_filebuf_seekability(*stream.rdbuf(), false);
}

TEST(IstreamSourceTest, SeekableFilebufClosed)
{
    std::ifstream stream;
    auto result = scn::detail::is_seekable_filebuf(*stream.rdbuf());
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(*result);

    temporary_file_path file{"scn_istream_source_test_closed.txt", "12"};
    stream.open(file.path(), std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    stream.close();
    result = scn::detail::is_seekable_filebuf(*stream.rdbuf());
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(*result);
}

// Nothing is lost or repeated when the filebuf can't put back everything
// it has read, and the file isn't seekable
TEST(IstreamSourceTest, IfstreamOnPipeFailedScan)
{
    const std::string input = "1234567890 abcdefghijklmnop";
    pipe_with_contents pipe{input};
    ASSERT_TRUE(pipe.valid());
    std::ifstream stream;
    char buffer[4];
    stream.rdbuf()->pubsetbuf(buffer, sizeof(buffer));
    stream.open(pipe.path(), std::ios::binary);
    if (!stream.is_open()) {
        GTEST_SKIP() << "Can't open " << pipe.path();
    }
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, "{} {}");
    ASSERT_THAT(result, Failed());

    const std::string unsynced{wrapper.unsynced()};
    const std::string rest = read_rest(stream);
    EXPECT_EQ(unsynced + rest, input);
}

TEST(IstreamSourceTest, WideIfstreamFailedScanAtEnd)
{
    temporary_file_path file{"scn_istream_source_test_wifstream.txt", "12"};
    std::wifstream stream{file.path(), std::ios::binary};
    ASSERT_TRUE(stream.is_open());
    scn::wscan_istream wrapper{stream};
    auto result = scn::scan<int, int>(wrapper, L"{} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));

    // The wide filebuf of some standard libraries has a variable-width
    // encoding. In that case, no seek is attempted, and characters stay
    // unsynced.
    const std::wstring rest = read_rest(stream);
    EXPECT_EQ(std::wstring{wrapper.unsynced()} + rest, L"12");
}

TEST(IstreamSourceTest, Fstream)
{
    temporary_file_path file{"scn_istream_source_test_fstream.txt", "12 34"};
    std::fstream stream{file.path(), std::ios::in | std::ios::binary};
    ASSERT_TRUE(stream.is_open());
    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int>(wrapper, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 12);
    EXPECT_TRUE(wrapper.sync());

    std::string rest = read_rest(stream);
    EXPECT_EQ(rest, " 34");
}

TEST(IstreamSourceTest, DerivedIfstreamIsReadThroughItsFilebuf)
{
    temporary_file_path file{"scn_istream_source_test_derived.txt", "12 34"};
    derived_ifstream stream{file.path(), std::ios::binary};
    ASSERT_TRUE(stream.is_open());
    std::stringbuf other{"99 99"};
    static_cast<std::istream&>(stream).rdbuf(&other);

    scn::scan_istream wrapper{stream};
    auto result = scn::scan<int>(wrapper, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 12);

    // Without the filebuf, the replaced stream buffer is read
    scn::scan_istream plain{static_cast<std::istream&>(stream)};
    auto plain_result = scn::scan<int>(plain, "{}");
    ASSERT_TRUE(plain_result);
    EXPECT_EQ(plain_result->value(), 99);
}

TEST(IstreamSourceTest, MoveCarriesTheFilebuf)
{
    temporary_file_path file{"scn_istream_source_test_move.txt", "12 34 56"};
    std::ifstream stream{file.path(), std::ios::binary};
    ASSERT_TRUE(stream.is_open());
    std::stringbuf other{"99 99"};
    static_cast<std::istream&>(stream).rdbuf(&other);

    scn::scan_istream wrapper{stream};
    scn::scan_istream moved{std::move(wrapper)};
    auto result = scn::scan<int>(moved, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 12);

    scn::scan_istream assigned{static_cast<std::istream&>(stream)};
    assigned = std::move(moved);
    auto assigned_result = scn::scan<int>(assigned, " {}");
    ASSERT_TRUE(assigned_result);
    EXPECT_EQ(assigned_result->value(), 34);

    auto moved_from = scn::scan<int>(moved, "{}");
    EXPECT_THAT(moved_from, FailedWith(scn::scan_error::invalid_source_state));
}
#endif

#endif  // !SCN_DISABLE_IOSTREAM

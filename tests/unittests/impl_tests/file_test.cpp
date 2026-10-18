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

#include <scn/fwd.h>

// The deprecated C FILE* sources are used on purpose.
// Must precede the library headers.
SCN_GCC_COMPAT_PUSH
SCN_GCC_COMPAT_IGNORE("-Wdeprecated-declarations")

#include <scn/impl.h>

#include "../test_common.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <locale>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace {

// Common state of the mock files: the entire contents of the file,
// as returned by successive reads from the underlying (OS-level) file.
struct mock_file_base {
    explicit mock_file_base(std::vector<std::string> chunks) noexcept
        : m_chunks(std::move(chunks))
    {
    }

    static void lock() {}
    static void unlock() {}

    SCN_NODISCARD static bool is_never_readable()
    {
        return false;
    }

    SCN_NODISCARD std::string contents() const
    {
        return std::accumulate(m_chunks.begin(), m_chunks.end(), std::string{});
    }

    // The shortest sequence of whole chunks that starts with `prefix`
    SCN_NODISCARD std::string chunks_covering(const std::string& prefix) const
    {
        std::string result;
        for (const auto& chunk : m_chunks) {
            if (result.size() >= prefix.size()) {
                break;
            }
            result += chunk;
        }
        return result;
    }

    bool fail_all_putbacks{false};
    // Reaching the end of the file is reported as a read error, not as EOF
    bool read_error_at_end{false};

protected:
    SCN_NODISCARD scn::impl::stdio_file_error end_of_file_error() const
    {
        return read_error_at_end ? scn::impl::stdio_file_error::error
                                 : scn::impl::stdio_file_error::eof;
    }

    std::vector<std::string> m_chunks;
};

// A file without a buffer: it's read one character at a time,
// and putback works like `ungetc` (only the previously read character can be
// put back).
struct unbuffered_mock_file : mock_file_base {
    static constexpr bool is_buffered = false;

    explicit unbuffered_mock_file(std::vector<std::string> chunks)
        : mock_file_base(std::move(chunks)), m_contents(contents())
    {
    }

    SCN_NODISCARD static bool has_buffering()
    {
        return false;
    }

    SCN_NODISCARD static std::string_view buffer()
    {
        return {};
    }
    [[noreturn]] static void unsafe_advance_n(std::ptrdiff_t)
    {
        SCN_UNREACHABLE;
    }
    [[noreturn]] static bool fill_buffer()
    {
        SCN_UNREACHABLE;
    }
    [[noreturn]] static bool has_error()
    {
        SCN_UNREACHABLE;
    }

    SCN_NODISCARD scn::expected<char, scn::impl::stdio_file_error> read_one()
    {
        if (m_position == m_contents.size()) {
            return scn::unexpected(end_of_file_error());
        }
        ++m_position;
        m_furthest_position = std::max(m_furthest_position, m_position);
        return m_contents[m_position - 1];
    }

    SCN_NODISCARD bool putback(char ch)
    {
        if (fail_all_putbacks || m_position == 0 ||
            m_contents[m_position - 1] != ch) {
            return false;
        }
        --m_position;
        return true;
    }

    // What would be read from the file next
    SCN_NODISCARD std::string remaining() const
    {
        return m_contents.substr(m_position);
    }

    // Everything ever read from the file, even if later put back
    SCN_NODISCARD std::string reached() const
    {
        return m_contents.substr(0, m_furthest_position);
    }

private:
    std::string m_contents;
    std::size_t m_position{0};
    std::size_t m_furthest_position{0};
};

// A file with a buffer, modeled after glibc:
//  - every refill of the buffer gets the next chunk
//  - characters can be put back into the buffer, if they match what was
//    previously read from it; otherwise they go into a separate backup area,
//    which is read from before the main buffer
struct buffered_mock_file : mock_file_base {
    static constexpr bool is_buffered = true;

    using mock_file_base::mock_file_base;

    SCN_NODISCARD static bool has_buffering()
    {
        return true;
    }

    SCN_NODISCARD std::string_view buffer() const
    {
        if (!m_backup.empty()) {
            return m_backup;
        }
        return std::string_view{m_buffer}.substr(m_buffer_position);
    }

    void unsafe_advance_n(std::ptrdiff_t n)
    {
        auto count = static_cast<std::size_t>(n);
        SCN_EXPECT(count <= buffer().size());
        if (!m_backup.empty()) {
            m_backup.erase(0, count);
        }
        else {
            m_buffer_position += count;
        }
    }

    SCN_NODISCARD bool fill_buffer()
    {
        SCN_EXPECT(buffer().empty());
        if (m_next_chunk == m_chunks.size()) {
            return false;
        }
        m_buffer = m_chunks[m_next_chunk++];
        m_buffer_position = 0;
        return !m_buffer.empty() || fill_buffer();
    }

    SCN_NODISCARD bool has_error() const
    {
        return read_error_at_end;
    }

    SCN_NODISCARD scn::expected<char, scn::impl::stdio_file_error> read_one()
    {
        ++read_one_calls;
        if (buffer().empty() && !fill_buffer()) {
            return scn::unexpected(end_of_file_error());
        }
        const auto ch = buffer().front();
        unsafe_advance_n(1);
        return ch;
    }

    SCN_NODISCARD bool putback(char ch)
    {
        if (fail_all_putbacks) {
            return false;
        }
        if (m_backup.empty() && m_buffer_position > 0 &&
            m_buffer[m_buffer_position - 1] == ch) {
            --m_buffer_position;
            return true;
        }
        m_backup.insert(m_backup.begin(), ch);
        return true;
    }

    // `read_one` is not used by the buffered scanning code
    int read_one_calls{0};

    // The chunks that have been read from the underlying file so far,
    // even if their contents were later put back
    SCN_NODISCARD std::string reached() const
    {
        return std::accumulate(
            m_chunks.begin(),
            m_chunks.begin() + static_cast<std::ptrdiff_t>(m_next_chunk),
            std::string{});
    }

    // What would be read from the file next
    SCN_NODISCARD std::string remaining() const
    {
        std::string result{m_backup};
        result.append(m_buffer, m_buffer_position);
        for (auto i = m_next_chunk; i < m_chunks.size(); ++i) {
            result.append(m_chunks[i]);
        }
        return result;
    }

private:
    std::string m_buffer;
    std::size_t m_buffer_position{0};
    std::size_t m_next_chunk{0};
    std::string m_backup;
};

template <typename MockFile>
class mock_file_buffer : public scn::detail::basic_scan_buffer<char> {
    using base = scn::detail::basic_scan_buffer<char>;
    using interface = scn::impl::file_buffer_interface<char, MockFile>;

public:
    mock_file_buffer(MockFile& file)
        : base(base::non_contiguous_tag{}), m_file(file)
    {
        interface::construct(m_file, m_source_error);
    }

    ~mock_file_buffer() override
    {
        interface::destruct(m_file);
    }

    bool do_fill() override
    {
        return interface::fill(m_file, this->m_current_view,
                               this->m_putback_buffer, this->m_source_error,
                               m_latest);
    }

    bool do_sync(std::ptrdiff_t pos) override
    {
        if (auto i = interface::sync(m_file, pos, this->m_current_view,
                                     this->m_putback_buffer);
            i != pos) {
            scn::detail::set_prelude_after_sync(
                m_prelude, pos, i, m_current_view, m_putback_buffer);
        }
        return true;
    }

    std::string& prelude()
    {
        return m_prelude;
    }
    const std::string& prelude() const
    {
        return m_prelude;
    }

private:
    MockFile& m_file;
    std::string m_prelude{};
    std::optional<char> m_latest{std::nullopt};
};

template <typename MockFile>
mock_file_buffer(MockFile&) -> mock_file_buffer<MockFile>;

enum class chunking_method {
    by_char,
    by_word,
    by_line,
    by_8bytes,
    by_all,
};

std::vector<std::string> chunk_up(const std::string& input,
                                  chunking_method method)
{
    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wswitch-default")

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wswitch-default")

    switch (method) {
        case chunking_method::by_char: {
            std::vector<std::string> result;
            result.reserve(input.size());
            std::transform(input.begin(), input.end(),
                           std::back_inserter(result),
                           [](char ch) { return std::string{ch}; });
            return result;
        }
        case chunking_method::by_word: {
            std::vector<std::string> result;
            for (std::size_t i = 0; i < input.size();) {
                const auto word_end = input.find_first_of(" \n", i);
                const auto chunk_end =
                    word_end == std::string::npos
                        ? input.size()
                        : input.find_first_not_of(" \n", word_end);
                result.emplace_back(input.substr(
                    i, chunk_end == std::string::npos ? chunk_end
                                                      : chunk_end - i));
                i = chunk_end == std::string::npos ? input.size() : chunk_end;
            }
            return result;
        }
        case chunking_method::by_line: {
            std::vector<std::string> result;
            for (std::size_t i = 0; i < input.size();) {
                const auto line_end = input.find('\n', i);
                const auto chunk_end =
                    line_end == std::string::npos ? input.size() : line_end + 1;
                result.emplace_back(input.substr(i, chunk_end - i));
                i = chunk_end;
            }
            return result;
        }
        case chunking_method::by_8bytes: {
            std::vector<std::string> result;
            for (std::size_t i = 0; i < input.size();) {
                if (input.size() - i >= 8) {
                    result.emplace_back(input.substr(i, 8));
                    i += 8;
                }
                else {
                    result.emplace_back(input.substr(i));
                    i = input.size();
                }
            }
            return result;
        }
        case chunking_method::by_all:
            return {input};
    }

    SCN_CLANG_POP
    SCN_GCC_POP

    SCN_UNREACHABLE;
}

struct custom_type {
    int a{}, b{};
};

#ifdef __GLIBC__
// A file that returns EOF or an error once in the middle of its contents,
// like a terminal receiving Ctrl-D
struct interrupted_cookie {
    std::string before{"123"};
    std::string after{"456 rest"};
    bool fail_with_error{false};
    int interruptions{0};
};

ssize_t interrupted_cookie_read(void* cookie, char* buf, std::size_t size)
{
    auto& state = *static_cast<interrupted_cookie*>(cookie);
    if (!state.before.empty()) {
        const auto n = std::min(size, state.before.size());
        std::memcpy(buf, state.before.data(), n);
        state.before.erase(0, n);
        return static_cast<ssize_t>(n);
    }
    if (state.interruptions == 0) {
        ++state.interruptions;
        if (state.fail_with_error) {
            errno = EIO;
            return -1;
        }
        return 0;
    }
    const auto n = std::min(size, state.after.size());
    std::memcpy(buf, state.after.data(), n);
    state.after.erase(0, n);
    return static_cast<ssize_t>(n);
}

class InterruptedFileTest : public testing::Test {
protected:
    void open(bool fail_with_error)
    {
        m_state.fail_with_error = fail_with_error;
        cookie_io_functions_t functions{interrupted_cookie_read, nullptr,
                                        nullptr, nullptr};
        m_handle = fopencookie(&m_state, "r", functions);
    }
    void TearDown() override
    {
        if (m_handle) {
            std::fclose(m_handle);
        }
    }

    interrupted_cookie m_state;
    std::FILE* m_handle{nullptr};
};
#endif

// A temporary file with the given contents, opened for reading with the given
// stdio buffer size (0 for unbuffered).
// MSVC requires buffer sizes to be at least 2.
class temporary_file {
public:
    temporary_file(const std::string& contents, std::size_t buffer_size)
        : m_handle(std::tmpfile())
    {
        if (!m_handle) {
            return;
        }
        // Must precede any other operation on the file
        m_buffer.resize(std::max(buffer_size, std::size_t{1}));
        std::setvbuf(m_handle, m_buffer.data(),
                     buffer_size == 0 ? _IONBF : _IOFBF, buffer_size);
        std::fwrite(contents.data(), 1, contents.size(), m_handle);
        std::rewind(m_handle);
    }

    ~temporary_file()
    {
        if (m_handle) {
            std::fclose(m_handle);
        }
    }

    temporary_file(const temporary_file&) = delete;
    temporary_file& operator=(const temporary_file&) = delete;

    std::FILE* handle() const
    {
        return m_handle;
    }

    // Reads everything left in the file
    std::string read_rest() const
    {
        std::string result;
        for (int ch = std::fgetc(m_handle); ch != EOF;
             ch = std::fgetc(m_handle)) {
            result.push_back(static_cast<char>(ch));
        }
        return result;
    }

private:
    std::vector<char> m_buffer;
    std::FILE* m_handle{nullptr};
};

class FileRealTest : public testing::TestWithParam<std::size_t> {
protected:
    temporary_file make_file(const std::string& contents) const
    {
        return temporary_file{contents, GetParam()};
    }
};

// The numbers from 0 to `count - 1`, separated by spaces, and then "end"
std::string make_many_numbers(int count)
{
    std::string result{};
    for (int i = 0; i < count; ++i) {
        result += std::to_string(i) + ' ';
    }
    result += "end";
    return result;
}

}  // namespace

template <>
struct scn::scanner<custom_type> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> decltype(pctx.begin())
    {
        return pctx.begin();
    }

    template <typename Ctx>
    auto scan(custom_type& val, Ctx& ctx) const
        -> scn::scan_expected<typename Ctx::iterator>
    {
        auto res = scn::scan<int, int>(ctx.range(), "{} {}");
        if (!res) {
            return scn::unexpected(res.error());
        }
        std::tie(val.a, val.b) = res->values();
        return res->begin();
    }
};

#if !SCN_DISABLE_LOCALE
namespace {
struct comma_numpunct : std::numpunct<char> {
    char do_decimal_point() const override
    {
        return ',';
    }
};

std::locale make_comma_locale()
{
    return {std::locale::classic(), new comma_numpunct};
}

// Records the decimal point of the locale it's scanned with,
// without consuming anything
struct decimal_point_probe {
    char decimal_point{};
};
}  // namespace

template <>
struct scn::scanner<decimal_point_probe> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> decltype(pctx.begin())
    {
        return pctx.begin();
    }

    template <typename Ctx>
    auto scan(decimal_point_probe& val, Ctx& ctx) const
        -> scn::scan_expected<typename Ctx::iterator>
    {
        if (!ctx.locale()) {
            val.decimal_point = '?';
        }
        else {
            val.decimal_point = std::use_facet<std::numpunct<char>>(
                                    ctx.locale().template get<std::locale>())
                                    .decimal_point();
        }
        return ctx.begin();
    }
};
#endif

using namespace std::string_literals;
using testing::ElementsAre;
using testing::FieldsAre;

TEST(FileTest, ChunkUp)
{
    const auto source = "Hello world!\n123 456\nfoobar"s;

    EXPECT_THAT(chunk_up(source, chunking_method::by_all), ElementsAre(source));
    EXPECT_THAT(
        chunk_up(source, chunking_method::by_8bytes),
        testing::ElementsAre("Hello wo", "rld!\n123", " 456\nfoo", "bar"));
    EXPECT_THAT(chunk_up(source, chunking_method::by_line),
                testing::ElementsAre("Hello world!\n", "123 456\n", "foobar"));
    EXPECT_THAT(
        chunk_up(source, chunking_method::by_word),
        testing::ElementsAre("Hello ", "world!\n", "123 ", "456\n", "foobar"));
    EXPECT_THAT(
        chunk_up(source, chunking_method::by_char),
        testing::ElementsAre("H", "e", "l", "l", "o", " ", "w", "o", "r", "l",
                             "d", "!", "\n", "1", "2", "3", " ", "4", "5", "6",
                             "\n", "f", "o", "o", "b", "a", "r"));
}

TEST(FileTest, NonReadableFile)
{
    scn::scan_file file{stderr};
    auto result = scn::scan<int>(file, "{}");
    ASSERT_THAT(result, Failed());
}

TEST(FileTest, NonReadableFileKeepsPrelude)
{
    scn::scan_file file{stderr};
    scn::detail::scan_file_access::get_prelude(file) = "123";
    auto result = scn::scan<int>(file, "{}");
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(file.prelude(), "123");
}

TEST(FileTest, NullHandle)
{
    scn::scan_file file{nullptr};
    auto result = scn::scan<int>(file, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
}

TEST(FileTest, NullHandleKeepsPrelude)
{
    scn::scan_file file{nullptr};
    scn::detail::scan_file_access::get_prelude(file) = "123";
    auto result = scn::scan<int>(file, "{}");
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(file.prelude(), "123");
}

TEST(FileTest, NullCFile)
{
    std::FILE* file = nullptr;
    auto result = scn::scan<int>(file, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
}

TEST(FileTest, MovedFrom)
{
    temporary_file temporary{"", 0};
    ASSERT_NE(temporary.handle(), nullptr);
    scn::scan_file file{temporary.handle()};
    scn::scan_file moved{std::move(file)};

    EXPECT_EQ(moved.handle(), std::optional<std::FILE*>{temporary.handle()});
    // The handle of a moved-from file is null, not UB
    EXPECT_EQ(file.handle(), std::optional<std::FILE*>{nullptr});

    auto result = scn::scan<int>(file, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
}

TEST(FileTest, SelfMoveAssignment)
{
    temporary_file temporary{"123", 0};
    ASSERT_NE(temporary.handle(), nullptr);

    scn::scan_file file{temporary.handle()};
    scn::detail::scan_file_access::get_prelude(file) = "4";

    // Through a reference, so that `-Wself-move` doesn't fire
    auto& same_file = file;
    file = std::move(same_file);

    EXPECT_EQ(file.prelude(), "4");
    EXPECT_EQ(file.contents().second, temporary.handle());
    auto result = scn::scan<int>(file, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 4123);
    EXPECT_EQ(file.prelude(), "");
    EXPECT_EQ(temporary.read_rest(), "");
}

TEST(FileTest, InitialValues)
{
    temporary_file temporary{"", 0};
    ASSERT_NE(temporary.handle(), nullptr);
    scn::scan_file file{temporary.handle()};
    scn::detail::scan_file_access::get_prelude(file) = "123 456\n";

    auto result = scn::scan<int, std::string>(file, "{} {}", {1, "abc"});
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(123, "456"));
    EXPECT_EQ(&result->file(), &file);

    auto value = scn::scan_value(file, 42);
    ASSERT_THAT(value, FailedWith(scn::scan_error::end_of_input));
}

template <typename File, chunking_method Method>
struct param {
    using file_type = File;
    static constexpr chunking_method method = Method;
};

// Chunking doesn't affect unbuffered files: they're read one char at a time
using type_list =
    ::testing::Types<param<unbuffered_mock_file, chunking_method::by_all>,
                     param<buffered_mock_file, chunking_method::by_all>,
                     param<buffered_mock_file, chunking_method::by_line>,
                     param<buffered_mock_file, chunking_method::by_word>,
                     param<buffered_mock_file, chunking_method::by_char>,
                     param<buffered_mock_file, chunking_method::by_8bytes>>;

template <typename T>
struct FileTestP : public testing::Test {
protected:
    using file_type = typename T::file_type;
    using buffer_type = mock_file_buffer<file_type>;
    using range_type = typename buffer_type::range_type;
    static constexpr chunking_method method = T::method;

    range_type& get(const std::string& input)
    {
        m_file.emplace(chunk_up(input, method));
        return next_scan();
    }

    // Like a new call to `scn::scan` with the same file:
    // the old buffer is synced and destroyed, and a new one is created.
    // Characters that couldn't be put back would be lost.
    range_type& next_scan()
    {
        m_range.reset();
        m_buffer.reset();
        m_buffer = std::make_unique<buffer_type>(*m_file);
        m_range.emplace(m_buffer->get());
        return *m_range;
    }

    template <typename Res>
    std::string get_remainder(const Res& res)
    {
        std::string out{};
        for (char ch : res) {
            out.push_back(ch);
        }
        return out;
    }

    // What would be read next with this file,
    // including the characters that couldn't be put back into it
    std::string get_file_remainder() const
    {
        return m_buffer->prelude() + m_file->remaining();
    }

    // How far the file was read. A buffered file is read in whole chunks.
    void expect_reached(const std::string& expected) const
    {
        if constexpr (file_type::is_buffered) {
            EXPECT_EQ(m_file->reached(), m_file->chunks_covering(expected));
        }
        else {
            EXPECT_EQ(m_file->reached(), expected);
        }
    }
    // What couldn't be put back into the file.
    // A buffered file needs to put back less, depending on the chunking.
    void expect_prelude(const std::string& expected) const
    {
        if constexpr (file_type::is_buffered) {
            EXPECT_THAT(expected, testing::StartsWith(m_buffer->prelude()));
        }
        else {
            EXPECT_EQ(m_buffer->prelude(), expected);
        }
    }

    std::optional<file_type> m_file;
    // Not std::optional: that triggers -Wmaybe-uninitialized false positives
    std::unique_ptr<buffer_type> m_buffer;
    std::optional<range_type> m_range;
};

TYPED_TEST_SUITE(FileTestP, type_list);

TYPED_TEST(FileTestP, OneChar)
{
    auto& range = this->get("abc\ndef");
    auto result = scn::scan<char>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 'a');
    this->expect_reached("a");
    EXPECT_EQ(this->get_file_remainder(), "bc\ndef");
    EXPECT_EQ(this->get_remainder(*result), "bc\ndef");
}

TYPED_TEST(FileTestP, OneInteger)
{
    auto& range = this->get("123\n");
    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    this->expect_reached("123\n");
    EXPECT_EQ(this->get_file_remainder(), "\n");
    EXPECT_EQ(this->get_remainder(*result), "\n");
}

TYPED_TEST(FileTestP, TwoIntegers)
{
    auto& range = this->get("123\n456");
    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
    this->expect_reached("123\n456");
    EXPECT_EQ(result->begin(), result->end());
    EXPECT_EQ(this->get_file_remainder(), "");
}

TYPED_TEST(FileTestP, ThreeIntegers)
{
    auto& range = this->get("123 456\n789");
    auto result = scn::scan<int, int, int>(range, "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b, c] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
    EXPECT_EQ(c, 789);
    this->expect_reached("123 456\n789");
    EXPECT_EQ(result->begin(), result->end());
    EXPECT_EQ(this->get_file_remainder(), "");
}

TYPED_TEST(FileTestP, LeftoverString)
{
    auto& range = this->get("abc\ndef");
    auto result = scn::scan<std::string>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "abc");
    this->expect_reached("abc\n");
    EXPECT_EQ(this->get_file_remainder(), "\ndef");
    EXPECT_EQ(this->get_remainder(*result), "\ndef");
}

TYPED_TEST(FileTestP, PutbackAll1)
{
    auto& range = this->get("abc");
    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("a");
    EXPECT_EQ(this->get_file_remainder(), "abc");
}

TYPED_TEST(FileTestP, PutbackAll2)
{
    auto& range = this->get("123 abc");
    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("123 a");
    EXPECT_EQ(this->get_file_remainder(), "123 abc");
}

TYPED_TEST(FileTestP, PutbackAll3)
{
    auto& range = this->get("123 456 abc\ndef");
    auto result = scn::scan<int, int, int>(range, "{} {} {}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("123 456 a");
    EXPECT_EQ(this->get_file_remainder(), "123 456 abc\ndef");
}

TYPED_TEST(FileTestP, CustomType)
{
    auto& range = this->get("123 456");
    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), FieldsAre(123, 456));
    this->expect_reached("123 456");
    EXPECT_EQ(result->begin(), result->end());
    EXPECT_EQ(this->get_file_remainder(), "");
}

TYPED_TEST(FileTestP, CustomTypeFail1)
{
    auto& range = this->get("123 abc");
    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("123 a");
    EXPECT_EQ(this->get_file_remainder(), "123 abc");
}

TYPED_TEST(FileTestP, CustomTypeFail2)
{
    auto& range = this->get("abc def");
    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("a");
    EXPECT_EQ(this->get_file_remainder(), "abc def");
}

TYPED_TEST(FileTestP, EmptyFile)
{
    auto& range = this->get("");
    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
    EXPECT_EQ(this->get_file_remainder(), "");
}

TYPED_TEST(FileTestP, EndOfFileBeforeAllValues)
{
    auto& range = this->get("123 456");
    auto result = scn::scan<int, int, int>(range, "{} {} {}");
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(this->get_file_remainder(), "123 456");
}

TYPED_TEST(FileTestP, ConsecutiveScans)
{
    auto& first_range = this->get("123 abc\n456 def");
    auto first = scn::scan<int, std::string>(first_range, "{} {}");
    ASSERT_THAT(first, Succeeded());
    EXPECT_THAT(first->values(), FieldsAre(123, "abc"));

    auto& second_range = this->next_scan();
    auto second = scn::scan<int, std::string>(second_range, "{} {}");
    ASSERT_THAT(second, Succeeded());
    EXPECT_THAT(second->values(), FieldsAre(456, "def"));
    EXPECT_EQ(this->get_file_remainder(), "");

    auto& third_range = this->next_scan();
    auto third = scn::scan<int>(third_range, "{}");
    ASSERT_THAT(third, FailedWith(scn::scan_error::end_of_input));
}

TYPED_TEST(FileTestP, ConsecutiveScansAfterFailure)
{
    auto& first_range = this->get("123 abc 456");
    auto first = scn::scan<int, int>(first_range, "{} {}");
    ASSERT_THAT(first, Failed());

    auto& second_range = this->next_scan();
    auto second = scn::scan<int, std::string, int>(second_range, "{} {} {}");
    ASSERT_THAT(second, Succeeded());
    EXPECT_THAT(second->values(), FieldsAre(123, "abc", 456));
    EXPECT_EQ(this->get_file_remainder(), "");
}

TYPED_TEST(FileTestP, ManyValues)
{
    std::string input{};
    for (int i = 0; i < 200; ++i) {
        input += std::to_string(i * 37) + (i % 10 == 9 ? "\n" : " ");
    }
    input += "end";

    auto* range = &this->get(input);
    for (int i = 0; i < 200; ++i) {
        auto result = scn::scan<int>(*range, "{}");
        ASSERT_THAT(result, Succeeded()) << i;
        EXPECT_EQ(result->value(), i * 37);
        range = &this->next_scan();
    }
    auto end = scn::scan<std::string>(*range, "{}");
    ASSERT_THAT(end, Succeeded());
    EXPECT_EQ(end->value(), "end");
    EXPECT_EQ(this->get_file_remainder(), "");
}

TYPED_TEST(FileTestP, PutbackFail1)
{
    auto& range = this->get("123");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    this->expect_reached("123");
    this->expect_prelude("");
    EXPECT_EQ(this->get_file_remainder(), "");
    EXPECT_EQ(this->get_remainder(*result), "");
}

TYPED_TEST(FileTestP, PutbackFail2)
{
    auto& range = this->get("123\n456");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(123, 456));
    this->expect_reached("123\n456");
    this->expect_prelude("");
    EXPECT_EQ(this->get_file_remainder(), "");
    EXPECT_EQ(this->get_remainder(*result), "");
}

TYPED_TEST(FileTestP, PutbackFailWithError1)
{
    auto& range = this->get("abc");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("a");
    this->expect_prelude("a");
    EXPECT_EQ(this->get_file_remainder(), "abc");
}

TYPED_TEST(FileTestP, PutbackFailWithError2)
{
    auto& range = this->get("123\nabc");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("123\na");
    this->expect_prelude("123\na");
    EXPECT_EQ(this->get_file_remainder(), "123\nabc");
}

TYPED_TEST(FileTestP, PutbackFailWithCustomType)
{
    auto& range = this->get("123 456");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), FieldsAre(123, 456));
    this->expect_reached("123 456");
    this->expect_prelude("");
    EXPECT_EQ(this->get_file_remainder(), "");
    EXPECT_EQ(this->get_remainder(*result), "");
}

TYPED_TEST(FileTestP, PutbackFailWithCustomTypeFail1)
{
    auto& range = this->get("123 abc");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("123 a");
    this->expect_prelude("123 a");
    EXPECT_EQ(this->get_file_remainder(), "123 abc");
}

TYPED_TEST(FileTestP, PutbackFailWithCustomTypeFail2)
{
    auto& range = this->get("abc def");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_THAT(result, Failed());
    this->expect_reached("a");
    this->expect_prelude("a");
    EXPECT_EQ(this->get_file_remainder(), "abc def");
}

#if !SCN_DISABLE_LOCALE
TYPED_TEST(FileTestP, LocaleWithSingleArgument)
{
    auto& range = this->get("abc");
    auto result =
        scn::scan<decimal_point_probe>(make_comma_locale(), range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().decimal_point, ',');
    EXPECT_EQ(this->get_file_remainder(), "abc");
}

TYPED_TEST(FileTestP, NoLocaleWithSingleArgument)
{
    auto& range = this->get("abc");
    auto result = scn::scan<decimal_point_probe>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().decimal_point, '?');
    EXPECT_EQ(this->get_file_remainder(), "abc");
}
#endif

TYPED_TEST(FileTestP, ScanValue)
{
    auto& range = this->get("123 456");
    auto result = scn::scan_value<int>(range);
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    this->expect_reached("123 ");
    EXPECT_EQ(this->get_file_remainder(), " 456");
    EXPECT_EQ(this->get_remainder(*result), " 456");
}

TYPED_TEST(FileTestP, ScanValueToEnd)
{
    auto& range = this->get("123");
    auto result = scn::scan_value<int>(range);
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    this->expect_reached("123");
    EXPECT_EQ(this->get_file_remainder(), "");
}

TYPED_TEST(FileTestP, ScanValueFail)
{
    auto& range = this->get("abc 123");
    auto result = scn::scan_value<int>(range);
    ASSERT_THAT(result, Failed());
    this->expect_reached("a");
    EXPECT_EQ(this->get_file_remainder(), "abc 123");
}

TYPED_TEST(FileTestP, ScanValueFailWithPutbackFailure)
{
    auto& range = this->get("abc 123");
    this->m_file->fail_all_putbacks = true;
    auto result = scn::scan_value<int>(range);
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(this->get_file_remainder(), "abc 123");
}

TYPED_TEST(FileTestP, ConsecutiveScanValues)
{
    auto& first_range = this->get("123 abc 456");
    auto first = scn::scan_value<int>(first_range);
    ASSERT_THAT(first, Succeeded());
    EXPECT_EQ(first->value(), 123);

    auto& failed_range = this->next_scan();
    auto failed = scn::scan_value<int>(failed_range);
    ASSERT_THAT(failed, Failed());
    EXPECT_EQ(this->get_file_remainder(), " abc 456");

    auto& word_range = this->next_scan();
    auto word = scn::scan_value<std::string>(word_range);
    ASSERT_THAT(word, Succeeded());
    EXPECT_EQ(word->value(), "abc");

    auto& last_range = this->next_scan();
    auto last = scn::scan_value<int>(last_range);
    ASSERT_THAT(last, Succeeded());
    EXPECT_EQ(last->value(), 456);
    EXPECT_EQ(this->get_file_remainder(), "");

    auto& end_range = this->next_scan();
    auto end = scn::scan_value<int>(end_range);
    ASSERT_THAT(end, FailedWith(scn::scan_error::end_of_input));
}

TYPED_TEST(FileTestP, ReadErrorOnEmptyFile)
{
    auto& range = this->get("");
    this->m_file->read_error_at_end = true;
    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
}

TYPED_TEST(FileTestP, ReadErrorWhenScanEndsBeforeEnd)
{
    auto& range = this->get("123 456");
    this->m_file->read_error_at_end = true;
    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    EXPECT_EQ(this->get_file_remainder(), " 456");
}

// The end of the integer is only known after reading past it
TYPED_TEST(FileTestP, ReadErrorWhenScanReachesEnd)
{
    auto& range = this->get("123");
    this->m_file->read_error_at_end = true;
    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
    EXPECT_EQ(this->get_file_remainder(), "123");
}

// `read_one` clears the EOF and error flags, and reads again,
// so it must not be used to retry a failed refill of the buffer
TEST(BufferedFileTest, NoReadOneAfterFailedRefill)
{
    buffered_mock_file file{{"123"}};
    mock_file_buffer buffer{file};
    auto range = buffer.get();
    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    EXPECT_EQ(file.remaining(), "");
    EXPECT_EQ(file.read_one_calls, 0);
}

TEST(BufferedFileTest, NoReadOneAfterFailedRefillWithError)
{
    buffered_mock_file file{{"123"}};
    file.read_error_at_end = true;
    mock_file_buffer buffer{file};
    auto range = buffer.get();
    auto result = scn::scan<int>(range, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
    EXPECT_EQ(file.read_one_calls, 0);
}

#ifdef __GLIBC__
TEST_F(InterruptedFileTest, DataAfterEofIsNotJoined)
{
    open(/*fail_with_error=*/false);
    ASSERT_TRUE(m_handle);
    scn::scan_file source{m_handle};
    auto result = scn::scan<int>(source, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    EXPECT_NE(std::feof(m_handle), 0);
}

TEST_F(InterruptedFileTest, DataAfterErrorIsNotJoined)
{
    open(/*fail_with_error=*/true);
    ASSERT_TRUE(m_handle);
    scn::scan_file source{m_handle};
    auto result = scn::scan<int>(source, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_source_state));
    EXPECT_NE(std::ferror(m_handle), 0);
}
#endif  // __GLIBC__

INSTANTIATE_TEST_SUITE_P(BufferSizes,
                         FileRealTest,
                         testing::Values(0, 2, 4, 5, 4096));

TEST_P(FileRealTest, SuccessfulScan)
{
    auto file = make_file("123 456 abc");
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};
    auto result = scn::scan<int, int>(source, "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(123, 456));
    EXPECT_EQ(
        scn::detail::scan_file_access::get_prelude(source) + file.read_rest(),
        " abc");
}

TEST_P(FileRealTest, FailedScanLeavesFileIntact)
{
    auto file = make_file("123 456 abc\ndef");
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};
    auto result = scn::scan<int, int, int>(source, "{} {} {}");
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(
        scn::detail::scan_file_access::get_prelude(source) + file.read_rest(),
        "123 456 abc\ndef");
}

TEST_P(FileRealTest, ConsecutiveScans)
{
    auto file = make_file("123 abc\n456 def\n");
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};

    auto first = scn::scan<int, std::string>(source, "{} {}");
    ASSERT_THAT(first, Succeeded());
    EXPECT_THAT(first->values(), FieldsAre(123, "abc"));

    auto failed = scn::scan<int, int>(source, "{} {}");
    ASSERT_THAT(failed, Failed());

    auto second = scn::scan<int, std::string>(source, "{} {}");
    ASSERT_THAT(second, Succeeded());
    EXPECT_THAT(second->values(), FieldsAre(456, "def"));

    auto end = scn::scan<int>(source, "{}");
    ASSERT_THAT(end, FailedWith(scn::scan_error::end_of_input));
}

TEST_P(FileRealTest, InputLongerThanBuffer)
{
    auto file = make_file(make_many_numbers(2000));
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};
    for (int i = 0; i < 2000; ++i) {
        auto result = scn::scan<int>(source, "{}");
        ASSERT_THAT(result, Succeeded()) << i;
        ASSERT_EQ(result->value(), i);
    }

    auto failed = scn::scan<std::string, int>(source, "{} {}");
    ASSERT_THAT(failed, Failed());
    EXPECT_EQ(
        scn::detail::scan_file_access::get_prelude(source) + file.read_rest(),
        " end");
}

TEST_P(FileRealTest, Prelude)
{
    auto file = make_file("34 56 abc");
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};
    scn::detail::scan_file_access::get_prelude(source) = "12 ";

    auto result = scn::scan<int, int, int>(source, "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(12, 34, 56));

    auto rest = scn::scan<std::string>(source, "{}");
    ASSERT_THAT(rest, Succeeded());
    EXPECT_EQ(rest->value(), "abc");
}

TEST_P(FileRealTest, PreludeWithFailedScan)
{
    auto file = make_file("34 56 abc");
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};
    scn::detail::scan_file_access::get_prelude(source) = "12 ";

    auto failed = scn::scan<int, int, int, int>(source, "{} {} {} {}");
    ASSERT_THAT(failed, Failed());

    auto result = scn::scan<int, int, int, std::string>(source, "{} {} {} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(12, 34, 56, "abc"));
}

TEST_P(FileRealTest, PreludeOnly)
{
    auto file = make_file("");
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};
    scn::detail::scan_file_access::get_prelude(source) = "123 456";

    auto first = scn::scan<int>(source, "{}");
    ASSERT_THAT(first, Succeeded());
    EXPECT_EQ(first->value(), 123);

    auto second = scn::scan<int>(source, "{}");
    ASSERT_THAT(second, Succeeded());
    EXPECT_EQ(second->value(), 456);
}

TEST_P(FileRealTest, ScanValue)
{
    auto file = make_file("123 456 abc");
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};

    auto first = scn::scan_value<int>(source);
    ASSERT_THAT(first, Succeeded());
    EXPECT_EQ(first->value(), 123);

    auto second = scn::scan_value<int>(source);
    ASSERT_THAT(second, Succeeded());
    EXPECT_EQ(second->value(), 456);

    auto failed = scn::scan_value<int>(source);
    ASSERT_THAT(failed, Failed());

    auto rest = scn::scan_value<std::string>(source);
    ASSERT_THAT(rest, Succeeded());
    EXPECT_EQ(rest->value(), "abc");
}

TEST_P(FileRealTest, ScanValueWithPrelude)
{
    auto file = make_file("456");
    ASSERT_TRUE(file.handle());
    scn::scan_file source{file.handle()};
    scn::detail::scan_file_access::get_prelude(source) = "123 ";

    auto first = scn::scan_value<int>(source);
    ASSERT_THAT(first, Succeeded());
    EXPECT_EQ(first->value(), 123);

    auto second = scn::scan_value<int>(source);
    ASSERT_THAT(second, Succeeded());
    EXPECT_EQ(second->value(), 456);
}

TEST_P(FileRealTest, RawFileSuccessfulScan)
{
    auto file = make_file("123 456 abc");
    ASSERT_TRUE(file.handle());
    auto result = scn::scan<int, int>(file.handle(), "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), FieldsAre(123, 456));
    EXPECT_EQ(file.read_rest(), " abc");
}

TEST_P(FileRealTest, RawFileConsecutiveScans)
{
    auto file = make_file("123 abc\n456 def\n");
    ASSERT_TRUE(file.handle());

    auto first = scn::scan<int, std::string>(file.handle(), "{} {}");
    ASSERT_THAT(first, Succeeded());
    EXPECT_THAT(first->values(), FieldsAre(123, "abc"));

    auto second = scn::scan<int, std::string>(file.handle(), "{} {}");
    ASSERT_THAT(second, Succeeded());
    EXPECT_THAT(second->values(), FieldsAre(456, "def"));

    auto end = scn::scan<int>(file.handle(), "{}");
    ASSERT_THAT(end, FailedWith(scn::scan_error::end_of_input));
}

TEST_P(FileRealTest, RawFileFailedScan)
{
    auto file = make_file("123 456 abc\ndef");
    ASSERT_TRUE(file.handle());
    auto result = scn::scan<int, int, int>(file.handle(), "{} {} {}");
    ASSERT_THAT(result, Failed());
    // Without a prelude, characters that can't be put back with `ungetc`
    // are lost, in which case the scan fails because of the failed sync.
    // Which characters can be put back depends on the C library, and the
    // buffer size.
    if (result.error().code() == scn::scan_error::invalid_source_state) {
#ifdef __GLIBC__
        FAIL() << "glibc can put back all of the characters";
#else
        GTEST_SKIP() << "ungetc can't put back all of the characters";
#endif
    }
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
    EXPECT_EQ(file.read_rest(), "123 456 abc\ndef");
}

TEST_P(FileRealTest, RawFileInputLongerThanBuffer)
{
    auto file = make_file(make_many_numbers(2000));
    ASSERT_TRUE(file.handle());
    for (int i = 0; i < 2000; ++i) {
        auto result = scn::scan<int>(file.handle(), "{}");
        ASSERT_THAT(result, Succeeded()) << i;
        ASSERT_EQ(result->value(), i);
    }
    EXPECT_EQ(file.read_rest(), " end");
}

SCN_GCC_COMPAT_POP

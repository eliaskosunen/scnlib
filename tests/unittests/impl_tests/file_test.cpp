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

#include <scn/impl.h>

#include "../wrapped_gtest.h"

#include <numeric>

namespace {

struct chunked_source {
    struct chunk {
        std::string content;
        bool ever_loaded{false};
    };

    chunked_source(std::vector<std::string> source) : chunks(std::move(source))
    {
    }

    std::string& get_active_chunk()
    {
        if (!active_chunk_index) {
            active_chunk_index = 0;
            return chunks[0];
        }
        assert(active_chunk_index.value() < chunks.size());
        return chunks[active_chunk_index.value()];
    }

    SCN_NODISCARD bool load_chunk(std::size_t chunk_idx)
    {
        if (chunk_idx >= chunks.size()) {
            return false;
        }

        active_char_index.reset();

        if (!active_chunk_index) {
            active_chunk_index = 0;
        }

        active_chunk_index = chunk_idx;

        return true;
    }

    void seek_chunk_end()
    {
        assert(active_chunk_index);
        active_char_index = chunks[*active_chunk_index].size() - 1;
    }

    std::vector<std::string> chunks;
    std::optional<std::size_t> active_chunk_index;
    std::optional<std::size_t> active_char_index;
};

struct unbuffered_mock_file {
    unbuffered_mock_file(std::vector<std::string> source) noexcept
        : m_source(std::move(source))
    {
    }

    static void lock() {}
    static void unlock() {}

    SCN_NODISCARD static bool is_never_readable()
    {
        return false;
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
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    [[noreturn]] static bool fill_buffer()
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    SCN_NODISCARD scn::expected<char, scn::impl::stdio_file_error> read_one()
    {
        auto* chunk = &m_source.get_active_chunk();

        if (!m_source.active_char_index) {
            m_source.active_char_index = 0;
            ++chars_read;
            return chunk->front();
        }

        if (m_source.active_char_index.value() == chunk->size()) {
            return scn::unexpected(scn::impl::stdio_file_error::eof);
        }

        m_source.active_char_index.value() += 1;

        if (m_source.active_char_index.value() == chunk->size()) {
            if (!m_source.load_chunk(m_source.active_chunk_index.value() + 1)) {
                return scn::unexpected(scn::impl::stdio_file_error::eof);
            }
            m_source.active_char_index = 0;
            chunk = &m_source.get_active_chunk();
        }

        if (chars_put_back == 0) {
            ++chars_read;
        }
        else {
            --chars_put_back;
        }
        return chunk->at(m_source.active_char_index.value());
    }

    static void prepare_putback() {}
    static void finalize_putback() {}

    SCN_NODISCARD bool putback(char ch)
    {
        if (fail_all_putbacks) {
            return false;
        }
        if (fail_next_putback) {
            fail_next_putback = false;
            return false;
        }

        if (!m_source.active_chunk_index) {
            return false;
        }

        auto& current_chunk = m_source.get_active_chunk();
        if (current_chunk.at(m_source.active_char_index.value()) != ch) {
            return false;
        }

        if (m_source.active_char_index.value() > 0) {
            m_source.active_char_index.value() -= 1;
            ++chars_put_back;
            return true;
        }
        if (m_source.active_chunk_index.value() == 0) {
            return false;
        }

        if (!m_source.load_chunk(m_source.active_chunk_index.value() - 1)) {
            assert(false);
        }
        m_source.seek_chunk_end();
        ++chars_put_back;
        return true;
    }

    chunked_source m_source;
    std::size_t chars_read{0}, chars_put_back{0};
    bool fail_next_putback{false}, fail_all_putbacks{false};
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

    bool fill() override
    {
        return interface::fill(m_file, this->m_current_view,
                               this->m_putback_buffer, this->m_source_error,
                               m_latest);
    }

    bool sync(std::ptrdiff_t pos) override
    {
        if (auto i = interface::sync(m_file, pos, *this, this->m_current_view,
                                     this->m_putback_buffer, m_prelude.empty());
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
            auto input_view = scn::ranges::subrange{input.begin(), input.end()};
            while (auto r = scn::scan<std::string_view, std::string_view>(
                       input_view, "{}{:/\\s*/}")) {
                auto [word, spaces] = r->values();
                result.emplace_back(std::string{word} + std::string{spaces});
                input_view = r->range();
            }
            return result;
        }
        case chunking_method::by_line: {
            std::vector<std::string> result;
            auto input_view = scn::ranges::subrange{input.begin(), input.end()};
            while (auto r = scn::scan<std::string_view, std::string_view>(
                       input_view, "{:[^\n]}{:/\n*/}")) {
                auto [line, breaks] = r->values();
                result.emplace_back(std::string{line} + std::string{breaks});
                input_view = r->range();
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

    SCN_EXPECT(false);
    SCN_UNREACHABLE;
}

struct custom_type {
    int a{}, b{};
};

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

TEST(FileTest, Simple)
{
    unbuffered_mock_file file{{"123 456"}};
    mock_file_buffer buffer{file};
    auto range = buffer.get();
    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_TRUE(result);
    auto [a, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(FileTest, CustomType)
{
    unbuffered_mock_file file{{"123 456"}};
    mock_file_buffer buffer{file};
    auto range = buffer.get();
    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), FieldsAre(123, 456));
}

TEST(FileTest, NonReadableFile)
{
    scn::scan_file file{stderr};
    auto result = scn::scan<int>(file, "{}");
    ASSERT_FALSE(result);
}

TEST(FileTest, Prelude)
{
    struct file_handle_guard {
        file_handle_guard() = default;

        FILE* handle{std::fopen("./scn_file_test_prelude_temp.txt", "wb+")};

        ~file_handle_guard()
        {
            std::fclose(handle);
        }
    };
    file_handle_guard handle{};
    scn::scan_file file{handle.handle};
    scn::detail::scan_file_access::get_prelude(file) = "123 456\n";
    auto result = scn::scan<int>(file, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 123);
}

template <typename File, chunking_method Method>
struct param {
    using file_type = File;
    static constexpr chunking_method method = Method;
};

using type_list =
    ::testing::Types<param<unbuffered_mock_file, chunking_method::by_all>,
                     param<unbuffered_mock_file, chunking_method::by_line>,
                     param<unbuffered_mock_file, chunking_method::by_word>,
                     param<unbuffered_mock_file, chunking_method::by_char>,
                     param<unbuffered_mock_file, chunking_method::by_8bytes>>;

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
        m_buffer.emplace(*m_file);
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

    std::string get_reached() const
    {
        const auto n = m_file->chars_read;

        std::string out;
        out.reserve(n);
        for (auto it = m_file->m_source.chunks.begin();
             it != m_file->m_source.chunks.end() && out.size() < n; ++it) {
            for (auto ch : *it) {
                if (out.size() == n) {
                    break;
                }
                out.push_back(ch);
            }
        }
        return out;
    }

    std::optional<file_type> m_file;
    std::optional<buffer_type> m_buffer;
    std::optional<range_type> m_range;
};

TYPED_TEST_SUITE(FileTestP, type_list);

TYPED_TEST(FileTestP, OneChar)
{
    auto& range = this->get("abc\ndef");
    auto result = scn::scan<char>(range, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 'a');
    EXPECT_EQ(this->get_reached(), "a");
    EXPECT_EQ(this->get_remainder(*result), "bc\ndef");
}

TYPED_TEST(FileTestP, OneInteger)
{
    auto& range = this->get("123\n");
    auto result = scn::scan<int>(range, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 123);
    EXPECT_EQ(this->get_reached(), "123\n");
    EXPECT_EQ(this->get_remainder(*result), "\n");
}

TYPED_TEST(FileTestP, TwoIntegers)
{
    auto& range = this->get("123\n456");
    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_TRUE(result);
    auto [a, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
    EXPECT_EQ(this->get_reached(), "123\n456");
    EXPECT_EQ(result->begin(), result->end());
}

TYPED_TEST(FileTestP, ThreeIntegers)
{
    auto& range = this->get("123 456\n789");
    auto result = scn::scan<int, int, int>(range, "{} {} {}");
    ASSERT_TRUE(result);
    auto [a, b, c] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
    EXPECT_EQ(c, 789);
    EXPECT_EQ(this->get_reached(), "123 456\n789");
    EXPECT_EQ(result->begin(), result->end());
}

TYPED_TEST(FileTestP, LeftoverString)
{
    auto& range = this->get("abc\ndef");
    auto result = scn::scan<std::string>(range, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "abc");
    EXPECT_EQ(this->get_reached(), "abc\n");
    EXPECT_EQ(this->get_remainder(*result), "\ndef");
}

TYPED_TEST(FileTestP, PutbackAll1)
{
    auto& range = this->get("abc");
    auto result = scn::scan<int>(range, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(this->get_reached(), "a");
}

TYPED_TEST(FileTestP, PutbackAll2)
{
    auto& range = this->get("123 abc");
    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_FALSE(result);
    EXPECT_EQ(this->get_reached(), "123 a");
}

TYPED_TEST(FileTestP, CustomType)
{
    auto& range = this->get("123 456");
    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), FieldsAre(123, 456));
    EXPECT_EQ(this->get_reached(), "123 456");
    EXPECT_EQ(result->begin(), result->end());
}

TYPED_TEST(FileTestP, CustomTypeFail1)
{
    auto& range = this->get("123 abc");
    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(this->get_reached(), "123 a");
}

TYPED_TEST(FileTestP, CustomTypeFail2)
{
    auto& range = this->get("abc def");
    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(this->get_reached(), "a");
}

TYPED_TEST(FileTestP, PutbackFail1)
{
    auto& range = this->get("123");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<int>(range, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 123);
    EXPECT_EQ(this->get_reached(), "123");
    EXPECT_EQ(this->m_buffer->prelude(), "");
    EXPECT_EQ(this->get_remainder(*result), "");
}

TYPED_TEST(FileTestP, PutbackFail2)
{
    auto& range = this->get("123\n456");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->values(), FieldsAre(123, 456));
    EXPECT_EQ(this->get_reached(), "123\n456");
    EXPECT_EQ(this->m_buffer->prelude(), "");
    EXPECT_EQ(this->get_remainder(*result), "");
}

TYPED_TEST(FileTestP, PutbackFailWithError1)
{
    auto& range = this->get("abc");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<int>(range, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(this->get_reached(), "a");
    EXPECT_EQ(this->m_buffer->prelude(), "a");
}

TYPED_TEST(FileTestP, PutbackFailWithError2)
{
    auto& range = this->get("123\nabc");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<int, int>(range, "{} {}");
    ASSERT_FALSE(result);
    EXPECT_EQ(this->get_reached(), "123\na");
    EXPECT_EQ(this->m_buffer->prelude(), "123\na");
}

TYPED_TEST(FileTestP, PutbackFailWithCustomType)
{
    auto& range = this->get("123 456");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), FieldsAre(123, 456));
    EXPECT_EQ(this->get_reached(), "123 456");
    EXPECT_EQ(this->m_buffer->prelude(), "");
    EXPECT_EQ(this->get_remainder(*result), "");
}

TYPED_TEST(FileTestP, PutbackFailWithCustomTypeFail1)
{
    auto& range = this->get("123 abc");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(this->get_reached(), "123 a");
    EXPECT_EQ(this->m_buffer->prelude(), "123 a");
}

TYPED_TEST(FileTestP, PutbackFailWithCustomTypeFail2)
{
    auto& range = this->get("abc def");
    this->m_file->fail_all_putbacks = true;

    auto result = scn::scan<custom_type>(range, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(this->get_reached(), "a");
    EXPECT_EQ(this->m_buffer->prelude(), "a");
}

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

#include <scn/scan.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <locale>
#include <optional>
#include <streambuf>
#include <string>
#include <thread>
#include <utility>

#if !SCN_DISABLE_IOSTREAM
#include <iostream>
#endif

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include "wrapped_gtest.h"

namespace {

class StdinFile : public testing::Test {
protected:
    void SetUp() override
    {
        static_cast<void>(scn::stdin_file()->release_unsynced());
    }

    void TearDown() override
    {
        if (!m_fed) {
            return;
        }
        // On Windows, the file can't be removed while it's open as stdin
        std::freopen(null_device(), "rb", stdin);
        std::remove(input_path().c_str());
    }

    void feed(const std::string& data)
    {
        {
            std::ofstream file{input_path(), std::ios_base::binary};
            file << data;
        }
        std::FILE* reopened = std::freopen(input_path().c_str(), "rb", stdin);
        m_fed = true;
        ASSERT_NE(reopened, nullptr);
    }

private:
    static std::string input_path()
    {
        return testing::TempDir() + "scn_stdin_file_test_input.txt";
    }

    static const char* null_device()
    {
#if defined(_WIN32)
        return "NUL";
#else
        return "/dev/null";
#endif
    }

    bool m_fed{false};
};

TEST_F(StdinFile, InputReadsFromStdin)
{
    feed("12 34");
    auto first = scn::input<int>("{}");
    ASSERT_TRUE(first);
    EXPECT_EQ(first->value(), 12);
    auto second = scn::input<int>("{}");
    ASSERT_TRUE(second);
    EXPECT_EQ(second->value(), 34);
}

TEST_F(StdinFile, FailedInputGivesCharactersBack)
{
    feed("123abc");
    EXPECT_FALSE((scn::input<int, int>("{} {}")));

    auto result = scn::input<std::string>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "123abc");
}

// Longer than what a C library is guaranteed to take back with ungetc.
// If some characters can't be given back, they stay in the scan_file,
// and the next input reads them first.
TEST_F(StdinFile, FailedInputWithLongReadAhead)
{
    const std::string word(20000, 'a');
    feed(word + " x y");
    EXPECT_FALSE((scn::input<std::string, int>("{} {}")));

    auto first = scn::input<std::string>("{}");
    ASSERT_TRUE(first);
    EXPECT_EQ(first->value(), word);
    auto second = scn::input<char>("{:c}");
    ASSERT_TRUE(second);
    EXPECT_EQ(second->value(), ' ');
}

TEST_F(StdinFile, UnsyncedCharactersCanBeReleased)
{
    feed(std::string(20000, 'a') + " x");
    EXPECT_FALSE((scn::input<std::string, int>("{} {}")));

    auto file = scn::stdin_file();
    if (file->unsynced().empty()) {
        // Everything was given back to the FILE
        EXPECT_TRUE(file->release_unsynced().empty());
        return;
    }
    const std::string unsynced{file->unsynced()};
    EXPECT_FALSE(file->file().has_value());
    EXPECT_EQ(file->release_unsynced(), unsynced);
    EXPECT_TRUE(file->unsynced().empty());
    EXPECT_TRUE(file->file().has_value());
}

TEST_F(StdinFile, Accessors)
{
    feed("abc def");
    auto file = scn::stdin_file();
    EXPECT_TRUE(file->unsynced().empty());
    EXPECT_TRUE(file->sync());
    EXPECT_TRUE(file->release_unsynced().empty());
    ASSERT_TRUE(file->file().has_value());
    std::FILE* const stdin_handle = stdin;
    EXPECT_EQ(*file->file(), stdin_handle);
    EXPECT_EQ(&*file, file.operator->());

    scn::scan_file& underlying = *file;
    auto result = scn::scan<std::string>(underlying, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "abc");
    EXPECT_EQ(&result->source(), &underlying);
}

TEST_F(StdinFile, MoveTransfersTheLock)
{
    feed("1 2");
    std::optional<scn::locked_stdin_file> first{scn::stdin_file()};
    scn::locked_stdin_file second{std::move(*first)};
    first.reset();

    auto result = scn::scan<int>(second, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 1);

    second = scn::stdin_file();
    result = scn::scan<int>(second, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 2);
}

TEST_F(StdinFile, InputWhileHoldingTheFile)
{
    feed("1 2 3");
    auto file = scn::stdin_file();

    auto first = scn::input<int>("{}");
    ASSERT_TRUE(first);
    EXPECT_EQ(first->value(), 1);

    auto second = scn::stdin_file();
    EXPECT_EQ(&*second, &*file);

    auto third = scn::scan<int>(file, "{}");
    ASSERT_TRUE(third);
    EXPECT_EQ(third->value(), 2);
}

TEST_F(StdinFile, ScanFromLockedStdinFile)
{
    feed("1 2 3 4");

    auto first = scn::scan<int>(scn::stdin_file(), "{}");
    ASSERT_TRUE(first);
    EXPECT_EQ(first->value(), 1);

    auto file = scn::stdin_file();
    auto second = scn::scan<int>(file, "{}");
    ASSERT_TRUE(second);
    EXPECT_EQ(second->value(), 2);

    auto third = scn::scan<int, int>(file, "{} {}");
    ASSERT_TRUE(third);
    EXPECT_EQ(std::get<0>(third->values()), 3);
    EXPECT_EQ(std::get<1>(third->values()), 4);

    static_assert(
        std::is_same_v<decltype(first), decltype(scn::input<int>("{}"))>);
}

TEST_F(StdinFile, ScanFromLockedStdinFileFailureGivesCharactersBack)
{
    feed("123abc");
    EXPECT_FALSE((scn::scan<int, int>(scn::stdin_file(), "{} {}")));

    auto result = scn::input<std::string>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "123abc");
}

TEST_F(StdinFile, ScanFromScanFileDoesNotSync)
{
    feed("123abc");
    auto file = scn::stdin_file();
    auto result = scn::scan<int>(*file, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 123);
}

TEST_F(StdinFile, ResultSource)
{
    feed("1 2");
    auto result = scn::input<int>("{}");
    ASSERT_TRUE(result);

    auto source = result->source();
    static_assert(std::is_same_v<decltype(source), scn::locked_stdin_file>);
    auto file = scn::stdin_file();
    EXPECT_EQ(&*source, &*file);

    auto locked_result = scn::scan<int>(file, "{}");
    ASSERT_TRUE(locked_result);
    EXPECT_EQ(locked_result->value(), 2);
    auto locked_source = locked_result->source();
    EXPECT_EQ(&*locked_source, &*file);
}

TEST_F(StdinFile, InputOnAnotherThreadWaitsForTheLock)
{
    feed("1 2");

    std::atomic<bool> done{false};
    std::optional<int> value{};
    std::thread other;
    {
        auto file = scn::stdin_file();
        other = std::thread{[&] {
            auto result = scn::input<int>("{}");
            if (result) {
                value = result->value();
            }
            done = true;
        }};
        // The thread can only finish if the lock isn't working
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_FALSE(done);
    }
    other.join();
    EXPECT_TRUE(done);
    EXPECT_EQ(value, std::optional<int>{1});
}

#if !SCN_DISABLE_IOSTREAM
// Holds what is written to it until it is synced, like a buffered stream
class holding_streambuf : public std::streambuf {
protected:
    int_type overflow(int_type ch) override
    {
        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            m_held.push_back(traits_type::to_char_type(ch));
        }
        return traits_type::not_eof(ch);
    }

    std::streamsize xsputn(const char* data, std::streamsize count) override
    {
        m_held.append(data, static_cast<std::size_t>(count));
        return count;
    }

    int sync() override
    {
        std::fwrite(m_held.data(), 1, m_held.size(), stdout);
        m_held.clear();
        return 0;
    }

private:
    std::string m_held{};
};
#endif

#if !defined(_WIN32)
// Redirects the stdout file descriptor to a temporary file while alive
class stdout_capture {
public:
    stdout_capture()
    {
        std::fflush(stdout);
        m_saved = ::dup(STDOUT_FILENO);
        m_file = std::tmpfile();
        if (m_saved != -1 && m_file != nullptr) {
            ::dup2(fileno(m_file), STDOUT_FILENO);
        }
    }

    stdout_capture(const stdout_capture&) = delete;
    stdout_capture& operator=(const stdout_capture&) = delete;

    ~stdout_capture()
    {
        restore();
        if (m_file != nullptr) {
            std::fclose(m_file);
        }
    }

    bool valid() const
    {
        return m_saved != -1 && m_file != nullptr;
    }

    std::string finish()
    {
        std::fflush(stdout);
        restore();
        std::rewind(m_file);
        std::string output(256, '\0');
        output.resize(std::fread(output.data(), 1, output.size(), m_file));
        return output;
    }

private:
    void restore()
    {
        if (m_saved != -1) {
            ::dup2(m_saved, STDOUT_FILENO);
            ::close(m_saved);
            m_saved = -1;
        }
    }

    int m_saved{-1};
    std::FILE* m_file{nullptr};
};
#endif

#if !defined(_WIN32) && !SCN_DISABLE_IOSTREAM
// The prompt must flush std::cout itself. With the streams synchronized with
// stdio, cout would be unbuffered, so this test replaces its buffer.
TEST_F(StdinFile, PromptWritesAfterCout)
{
    feed("42");

    stdout_capture capture;
    ASSERT_TRUE(capture.valid());

    holding_streambuf holder;
    auto* const original = std::cout.rdbuf(&holder);
    std::cout << "a";
    auto result = scn::prompt<int>("b", "{}");
    std::cout << "c" << std::flush;
    std::cout.rdbuf(original);

    const auto output = capture.finish();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 42);
    EXPECT_EQ(output, "abc");
}
#endif

TEST_F(StdinFile, MovedFromLockedStdinFileCannotBeScanned)
{
    feed("1 2");
    auto lock = scn::stdin_file();
    auto other = std::move(lock);

    auto moved_from = scn::scan<int>(lock, "{}");
    ASSERT_FALSE(moved_from);
    EXPECT_EQ(moved_from.error().code(), scn::scan_error::invalid_source_state);

    auto result = scn::scan<int>(other, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 1);
}

TEST_F(StdinFile, MovingTheScanFileOutBreaksInput)
{
    feed("1 2");
    {
        auto lock = scn::stdin_file();
        scn::scan_file taken{std::move(*lock)};

        auto result = scn::input<int>("{}");
        EXPECT_FALSE(result);
        if (!result) {
            EXPECT_EQ(result.error().code(),
                      scn::scan_error::invalid_source_state);
        }

        // Restore it for the other tests
        *lock = std::move(taken);
    }

    auto result = scn::input<int>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 1);
}

// Scanner that runs a scan from stdin inside the scan it is a part of
struct nested_probe {};

enum class nested_action {
    input,
    input_localized,
    prompt,
    scan_locked,
    scan_value_locked,
    scan_file_itself,
};

struct nested_state {
    nested_action action{nested_action::input};
    bool ran{false};
    std::optional<scn::scan_error> error{};
};

nested_state& nested()
{
    static nested_state state;
    return state;
}

template <typename Result>
void record_nested(const Result& result)
{
    nested().ran = true;
    nested().error.reset();
    if (!result) {
        nested().error = result.error();
    }
}

}  // namespace

template <>
struct scn::scanner<nested_probe, char> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> typename ParseCtx::iterator
    {
        return pctx.begin();
    }

    template <typename Context>
    auto scan(nested_probe&, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        switch (nested().action) {
            case nested_action::input:
                record_nested(scn::input<int>("{}"));
                break;
            case nested_action::input_localized:
#if !SCN_DISABLE_LOCALE
                record_nested(scn::input<int>(std::locale::classic(), "{}"));
#endif
                break;
            case nested_action::prompt:
                record_nested(scn::prompt<int>("nested prompt", "{}"));
                break;
            case nested_action::scan_locked:
                record_nested(scn::scan<int>(scn::stdin_file(), "{}"));
                break;
            case nested_action::scan_value_locked:
                record_nested(scn::scan_value<int>(scn::stdin_file()));
                break;
            case nested_action::scan_file_itself: {
                auto file = scn::stdin_file();
                record_nested(scn::scan<int>(*file, "{}"));
                break;
            }

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wcovered-switch-default")
            default:
                SCN_UNREACHABLE;
                SCN_CLANG_POP
        }
        return ctx.begin();
    }
};

namespace {

enum class outer_kind { input, locked, scan_file_itself };

// Reads 1, runs the nested action, reads 2
std::optional<scn::scan_error::code_t> run_nested(outer_kind kind,
                                                  nested_action action)
{
    nested() = nested_state{};
    nested().action = action;

    auto check = [](const auto& result) {
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_EQ(std::get<0>(result->values()), 1);
            EXPECT_EQ(std::get<2>(result->values()), 2);
        }
    };
    switch (kind) {
        case outer_kind::input:
            check(scn::input<int, nested_probe, int>("{} {} {}"));
            break;
        case outer_kind::locked: {
            auto lock = scn::stdin_file();
            check(scn::scan<int, nested_probe, int>(lock, "{} {} {}"));
            break;
        }
        case outer_kind::scan_file_itself: {
            auto lock = scn::stdin_file();
            check(scn::scan<int, nested_probe, int>(*lock, "{} {} {}"));
            break;
        }

            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wcovered-switch-default")
        default:
            SCN_UNREACHABLE;
            SCN_CLANG_POP
    }

    EXPECT_TRUE(nested().ran);
    if (!nested().error) {
        return std::nullopt;
    }
    return nested().error->code();
}

TEST_F(StdinFile, NestedStdinScanFails)
{
    const nested_action actions[] = {
        nested_action::input,
        nested_action::prompt,
        nested_action::scan_locked,
        nested_action::scan_value_locked,
        nested_action::scan_file_itself,
#if !SCN_DISABLE_LOCALE
        nested_action::input_localized,
#endif
    };
    const outer_kind kinds[] = {outer_kind::input, outer_kind::locked,
                                outer_kind::scan_file_itself};

    for (auto kind : kinds) {
        for (auto action : actions) {
            SCOPED_TRACE(testing::Message()
                         << "outer " << static_cast<int>(kind) << ", nested "
                         << static_cast<int>(action));
            feed("1 2 3");
            EXPECT_EQ(run_nested(kind, action),
                      std::optional{scn::scan_error::invalid_source_state});

            // The outer scan has used 1 and 2, the nested one nothing
            auto rest = scn::input<int>("{}");
            ASSERT_TRUE(rest);
            EXPECT_EQ(rest->value(), 3);
        }
    }
}

TEST_F(StdinFile, NestedStdinScanDoesNotBreakTheNextOne)
{
    feed("1 2 3 4 5");
    EXPECT_EQ(run_nested(outer_kind::input, nested_action::input),
              std::optional{scn::scan_error::invalid_source_state});

    auto next = scn::input<int, int>("{} {}");
    ASSERT_TRUE(next);
    EXPECT_EQ(std::get<0>(next->values()), 3);
    EXPECT_EQ(std::get<1>(next->values()), 4);
}

#if !defined(_WIN32)
TEST_F(StdinFile, NestedPromptDoesNotPrint)
{
    feed("1 2 3");

    stdout_capture capture;
    ASSERT_TRUE(capture.valid());
    EXPECT_EQ(run_nested(outer_kind::input, nested_action::prompt),
              std::optional{scn::scan_error::invalid_source_state});
    EXPECT_EQ(capture.finish(), "");
}
#endif

}  // namespace

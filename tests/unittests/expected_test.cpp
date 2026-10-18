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

#include "test_common.h"

#include <scn/scan.h>

#include <any>
#include <stdexcept>

#if SCN_GCC_COMPAT
#define SCN_TEST_NOINLINE __attribute__((noinline))
#elif SCN_MSVC
#define SCN_TEST_NOINLINE __declspec(noinline)
#else
#define SCN_TEST_NOINLINE
#endif

namespace {
// Not trivially relocatable: points to itself
struct self_referencing {
    self_referencing() : self(this) {}
    self_referencing(const self_referencing&) : self(this) {}
    self_referencing& operator=(const self_referencing&)
    {
        return *this;
    }

    self_referencing* self;
};

template <typename T>
SCN_TEST_NOINLINE std::string take_string_result_by_value(T result)
{
    return result->value();
}

template <typename T>
SCN_TEST_NOINLINE bool take_self_referencing_by_value(T value)
{
    return value->self == &*value;
}

// Counts live instances, checks that only constructed ones are destroyed, and
// has a potentially-throwing constructor from int
struct live_counted {
    static inline int live = 0;
    static constexpr unsigned constructed = 0xC0FFEE;

    live_counted() noexcept
    {
        ++live;
    }
    live_counted(int)
    {
        ++live;
    }
    live_counted(const live_counted&) noexcept
    {
        ++live;
    }
    live_counted(live_counted&&) noexcept
    {
        ++live;
    }
    live_counted& operator=(const live_counted&) = default;
    live_counted& operator=(live_counted&&) = default;
    ~live_counted()
    {
        EXPECT_EQ(magic, constructed);
        magic = 0;
        --live;
    }

    unsigned magic{constructed};
};

struct nothrow_counted : live_counted {
    explicit nothrow_counted(int) noexcept {}
};

template <typename Expected, typename Arg, typename = void>
struct can_emplace : std::false_type {};
template <typename Expected, typename Arg>
struct can_emplace<Expected,
                   Arg,
                   std::void_t<decltype(std::declval<Expected&>().emplace(
                       std::declval<Arg>()))>> : std::true_type {};

template <bool Noexcept>
struct with_default_constructor {
    with_default_constructor() noexcept(Noexcept) {}
    std::string s;
};

#if SCN_HAS_EXCEPTIONS
struct throws_on_nonzero_construct {
    throws_on_nonzero_construct(int value)
    {
        if (value != 0) {
            throw std::runtime_error{"throws_on_nonzero_construct"};
        }
    }
};

// Constructing from an int throws, copying throws unless `NothrowCopy`, and
// moving throws unless `NothrowMove`
SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wmissing-noreturn")
template <bool NothrowMove, bool NothrowCopy = false>
struct throwing_counted : live_counted {
    throwing_counted() = default;
    explicit throwing_counted(int) : live_counted()
    {
        throw std::runtime_error{"throwing_counted(int)"};
    }
    throwing_counted(const throwing_counted&) noexcept(NothrowCopy)
        : live_counted()
    {
        if constexpr (!NothrowCopy) {
            throw std::runtime_error{"throwing_counted copy"};
        }
    }
    throwing_counted(throwing_counted&&) noexcept(NothrowMove) : live_counted()
    {
        if constexpr (!NothrowMove) {
            throw std::runtime_error{"throwing_counted move"};
        }
    }
    throwing_counted& operator=(const throwing_counted&) = default;
    throwing_counted& operator=(throwing_counted&&) = default;
};
SCN_CLANG_POP

// Throws when constructed from a nonzero int, and isn't nothrow move
// constructible
struct throwing_from_int : throwing_counted<false> {
    throwing_from_int(int value)
    {
        if (value != 0) {
            throw std::runtime_error{"throwing_from_int"};
        }
    }
};

template <typename T, typename E>
constexpr bool is_assignable_from_error_v =
    std::is_assignable_v<scn::expected<T, E>&, const scn::unexpected<E>&> &&
    std::is_assignable_v<scn::expected<T, E>&, scn::unexpected<E>>;

template <typename T, typename E>
constexpr bool is_fully_assignable_v =
    std::is_copy_assignable_v<scn::expected<T, E>> &&
    std::is_move_assignable_v<scn::expected<T, E>> &&
    is_assignable_from_error_v<T, E>;
#endif
}  // namespace

// Regression test for [[clang::trivial_abi]] being applied to `expected` with
// a non-trivially relocatable value type
TEST(ExpectedTest, PassNonTriviallyRelocatableByValue)
{
    auto result = scn::scan<std::string>("abc", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(take_string_result_by_value(std::move(result)), "abc");

    EXPECT_TRUE(
        take_self_referencing_by_value(scn::expected<self_referencing, int>{}));
    EXPECT_TRUE(take_self_referencing_by_value(
        scn::expected<self_referencing, std::string>{}));
}

TEST(ExpectedTest, EmplaceOverErrorDestroysError)
{
    // Like std::expected, emplace() requires a nothrow constructor
    static_assert(!can_emplace<scn::expected<live_counted, int>, int>::value);
    static_assert(can_emplace<scn::expected<nothrow_counted, int>, int>::value);

    live_counted::live = 0;
    {
        scn::expected<nothrow_counted, live_counted> e{scn::unexpect};
        e.emplace(1);
        EXPECT_TRUE(e.has_value());
        EXPECT_EQ(live_counted::live, 1);
    }
    EXPECT_EQ(live_counted::live, 0);
}

TEST(ExpectedTest, ConvertingConstructorDoesNotLeak)
{
    live_counted::live = 0;
    {
        const scn::expected<int, int> error{scn::unexpect, 1};
        scn::expected<live_counted, long> from_error{error};
        EXPECT_FALSE(from_error.has_value());
        EXPECT_EQ(live_counted::live, 0);

        const scn::expected<int, int> value{1};
        scn::expected<live_counted, long> from_value{value};
        EXPECT_TRUE(from_value.has_value());
        EXPECT_EQ(live_counted::live, 1);
    }
    EXPECT_EQ(live_counted::live, 0);
}

TEST(ExpectedTest, AssignValueAndError)
{
    scn::expected<std::string, int> e{"a"};
    e = scn::unexpected(1);
    ASSERT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), 1);

    const auto error = scn::unexpected(2);
    e = error;
    EXPECT_EQ(e.error(), 2);

    e = std::string{"b"};
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(*e, "b");
}

// std::any is constructible from anything, including the tags, `unexpected`
// and `expected` itself
TEST(ExpectedTest, ConstructAndAssignAny)
{
    using expected_any = scn::expected<std::any, int>;

    static_assert(!std::is_convertible_v<scn::unexpect_t, expected_any>);
    expected_any from_unexpect{scn::unexpect};
    EXPECT_FALSE(from_unexpect.has_value());

    scn::unexpected<int> unex{7};
    expected_any from_unexpected{unex};
    ASSERT_FALSE(from_unexpected.has_value());
    EXPECT_EQ(from_unexpected.error(), 7);

    expected_any error{scn::unexpect, 5};
    expected_any e{std::any{1}};
    e = error;
    ASSERT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), 5);

    e = std::any{1};
    e = unex;
    ASSERT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), 7);

    scn::expected<void, int> void_value{};
    void_value = unex;
    ASSERT_FALSE(void_value.has_value());
    EXPECT_EQ(void_value.error(), 7);
}

#if SCN_HAS_EXCEPTIONS
TEST(ExpectedTest, AssignValueOverErrorRethrows)
{
    scn::expected<throws_on_nonzero_construct, int> e{scn::unexpect, 42};
    EXPECT_THROW(e = 1, std::runtime_error);
    ASSERT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), 42);
}
#endif

TEST(ExpectedTest, AssignmentNoexcept)
{
    static_assert(
        std::is_nothrow_move_assignable_v<scn::expected<std::string, int>>);
    static_assert(
        std::is_nothrow_move_assignable_v<scn::expected<int, std::string>>);
    static_assert(std::is_nothrow_move_assignable_v<
                  scn::expected<std::string, std::string>>);
    static_assert(
        std::is_nothrow_move_assignable_v<scn::expected<void, std::string>>);
    static_assert(
        !std::is_nothrow_copy_assignable_v<scn::expected<std::string, int>>);
    static_assert(std::is_nothrow_assignable_v<scn::expected<std::string, int>&,
                                               scn::unexpected<int>>);
    static_assert(std::is_nothrow_assignable_v<scn::expected<std::string, int>&,
                                               const scn::unexpected<int>&>);
}

TEST(ExpectedTest, ValueOnValue)
{
    scn::expected<int, int> e{42};
    EXPECT_EQ(e.value(), 42);
    EXPECT_EQ(std::as_const(e).value(), 42);
    EXPECT_EQ(std::move(e).value(), 42);
}

#if SCN_HAS_EXCEPTIONS
TEST(ExpectedTest, ValueOnErrorThrows)
{
    scn::expected<int, scn::scan_error> e{scn::unexpected(
        scn::scan_error{scn::scan_error::invalid_scanned_value, "msg"})};

    try {
        (void)e.value();
        FAIL() << "value() did not throw";
    }
    catch (const scn::bad_expected_access<scn::scan_error>& ex) {
        EXPECT_EQ(ex.error().code(), scn::scan_error::invalid_scanned_value);
        EXPECT_STREQ(ex.error().msg(), "msg");
        EXPECT_STREQ(ex.what(), "bad expected access");
    }

    EXPECT_THROW((void)std::as_const(e).value(),
                 scn::bad_expected_access<scn::scan_error>);
    EXPECT_THROW((void)std::move(e).value(),
                 scn::bad_expected_access<scn::scan_error>);
    EXPECT_THROW((void)std::move(std::as_const(e)).value(),
                 scn::bad_expected_access<scn::scan_error>);
    EXPECT_THROW((void)e.value(), scn::bad_expected_access<void>);
    EXPECT_THROW((void)e.value(), std::exception);
}
#else
TEST(ExpectedTest, ValueOnErrorAborts)
{
    scn::expected<int, int> e{scn::unexpect, 1};
    EXPECT_DEATH((void)e.value(), "");
}
#endif

TEST(ExpectedTest, ConvertingConstructorConstraints)
{
    using expected_int = scn::expected<int, int>;
    using expected_bool = scn::expected<bool, int>;

    static_assert(std::is_convertible_v<int, expected_int>);
    static_assert(std::is_convertible_v<short, expected_int>);
    static_assert(!std::is_convertible_v<std::in_place_t, expected_int>);
    static_assert(!std::is_convertible_v<scn::unexpect_t, expected_int>);
    static_assert(!std::is_convertible_v<std::string, expected_int>);

    // `unexpected` constructs the error, not the value
    static_assert(std::is_convertible_v<scn::unexpected<int>, expected_int>);
    static_assert(!std::is_convertible_v<scn::unexpected<std::string>,
                                         scn::expected<int, int>>);

    // An `expected` is not converted to bool through its `operator bool`.
    // Deliberately unlike std::expected, which constructs `expected<bool>`
    // from another `expected` with the converting constructor.
    static_assert(std::is_convertible_v<bool, expected_bool>);
    static_assert(!std::is_constructible_v<expected_bool, expected_int>);
    static_assert(!std::is_constructible_v<expected_bool, const expected_int&>);

    // Copying must not go through the converting constructor
    static_assert(std::is_nothrow_copy_constructible_v<expected_int>);
    static_assert(std::is_nothrow_move_constructible_v<expected_int>);

    expected_int e{1};
    expected_int copy{e};
    EXPECT_EQ(*copy, 1);
}

TEST(ExpectedTest, Triviality)
{
    using expected_int = scn::expected<int, scn::scan_error>;
    using expected_void = scn::expected<void, scn::scan_error>;
    using expected_string = scn::expected<std::string, scn::scan_error>;

    static_assert(std::is_trivially_copyable_v<expected_int>);
    static_assert(std::is_trivially_destructible_v<expected_int>);
    static_assert(std::is_trivially_copyable_v<expected_void>);
    static_assert(std::is_trivially_destructible_v<expected_void>);
    static_assert(!std::is_trivially_copyable_v<expected_string>);
    static_assert(!std::is_trivially_destructible_v<expected_string>);
}

#if SCN_HAS_EXCEPTIONS
TEST(ExpectedTest, ThrowingConvertingConstructor)
{
    live_counted::live = 0;
    {
        using value_throws =
            scn::expected<throwing_counted<true>, live_counted>;
        const scn::expected<int, live_counted> value{1};
        EXPECT_THROW((void)value_throws{value}, std::runtime_error);
        EXPECT_THROW((void)value_throws{(scn::expected<int, live_counted>{1})},
                     std::runtime_error);
        EXPECT_EQ(live_counted::live, 0);

        using error_throws =
            scn::expected<live_counted, throwing_counted<true>>;
        const scn::expected<live_counted, int> error{scn::unexpect, 1};
        EXPECT_THROW((void)error_throws{error}, std::runtime_error);
        EXPECT_THROW((void)error_throws{(
                         scn::expected<live_counted, int>{scn::unexpect, 1})},
                     std::runtime_error);
        EXPECT_EQ(live_counted::live, 0);
    }
    EXPECT_EQ(live_counted::live, 0);
}

TEST(ExpectedTest, ThrowingCopyAndMoveConstructor)
{
    live_counted::live = 0;
    {
        using value_throws =
            scn::expected<throwing_counted<false>, live_counted>;
        value_throws value{};
        EXPECT_THROW((void)value_throws{std::as_const(value)},
                     std::runtime_error);
        EXPECT_THROW((void)value_throws{std::move(value)}, std::runtime_error);
        EXPECT_EQ(live_counted::live, 1);

        using error_throws =
            scn::expected<live_counted, throwing_counted<false>>;
        error_throws error{scn::unexpect};
        EXPECT_THROW((void)error_throws{std::as_const(error)},
                     std::runtime_error);
        EXPECT_THROW((void)error_throws{std::move(error)}, std::runtime_error);
        EXPECT_EQ(live_counted::live, 2);
    }
    EXPECT_EQ(live_counted::live, 0);
}

TEST(ExpectedTest, ThrowingErrorAssignmentKeepsValue)
{
    live_counted::live = 0;
    {
        // The error is constructed into a temporary first
        scn::expected<live_counted, throwing_counted<true>> e{};
        const scn::unexpected<throwing_counted<true>> unex{std::in_place};
        EXPECT_THROW(e = unex, std::runtime_error);
        EXPECT_TRUE(e.has_value());
        EXPECT_EQ(live_counted::live, 2);
    }
    EXPECT_EQ(live_counted::live, 0);
    {
        // The value is moved into a temporary, and moved back
        scn::expected<live_counted, throwing_counted<false>> e{};
        scn::unexpected<throwing_counted<false>> unex{std::in_place};
        EXPECT_THROW(e = std::as_const(unex), std::runtime_error);
        EXPECT_THROW(e = std::move(unex), std::runtime_error);
        EXPECT_TRUE(e.has_value());
        EXPECT_EQ(live_counted::live, 2);

        scn::expected<live_counted, throwing_counted<false>> error{
            scn::unexpect};
        EXPECT_THROW(e = std::as_const(error), std::runtime_error);
        EXPECT_THROW(e = std::move(error), std::runtime_error);
        EXPECT_TRUE(e.has_value());
        EXPECT_EQ(live_counted::live, 3);
    }
    EXPECT_EQ(live_counted::live, 0);
    {
        scn::expected<void, throwing_counted<false>> e{};
        const scn::unexpected<throwing_counted<false>> unex{std::in_place};
        EXPECT_THROW(e = unex, std::runtime_error);
        EXPECT_TRUE(e.has_value());
        EXPECT_EQ(live_counted::live, 1);
    }
    EXPECT_EQ(live_counted::live, 0);
}

TEST(ExpectedTest, ThrowingValueAssignmentKeepsError)
{
    live_counted::live = 0;
    {
        // The error is moved into a temporary, and moved back
        scn::expected<throwing_from_int, live_counted> e{scn::unexpect};
        EXPECT_THROW(e = 1, std::runtime_error);
        EXPECT_FALSE(e.has_value());
        EXPECT_EQ(live_counted::live, 1);
    }
    EXPECT_EQ(live_counted::live, 0);
    {
        // The value is copied into a temporary first
        scn::expected<throwing_counted<true>, live_counted> e{scn::unexpect};
        const scn::expected<throwing_counted<true>, live_counted> value{};
        EXPECT_THROW(e = value, std::runtime_error);
        EXPECT_FALSE(e.has_value());
        EXPECT_EQ(live_counted::live, 2);
    }
    EXPECT_EQ(live_counted::live, 0);
    {
        // The error is moved into a temporary, and moved back
        scn::expected<throwing_counted<false>, live_counted> e{scn::unexpect};
        scn::expected<throwing_counted<false>, live_counted> value{};
        EXPECT_THROW(e = std::as_const(value), std::runtime_error);
        EXPECT_THROW(e = std::move(value), std::runtime_error);
        EXPECT_FALSE(e.has_value());
        EXPECT_EQ(live_counted::live, 2);
    }
    EXPECT_EQ(live_counted::live, 0);
}

// Like std::expected, assigning over the other alternative needs T or E to be
// nothrow move constructible, so that a throw can be rolled back
TEST(ExpectedTest, AssignmentConstraints)
{
    using throws = throwing_counted<false>;
    using throws_on_copy = throwing_counted<true>;

    static_assert(std::is_copy_constructible_v<scn::expected<throws, throws>>);
    static_assert(!std::is_copy_assignable_v<scn::expected<throws, throws>>);
    static_assert(!std::is_move_assignable_v<scn::expected<throws, throws>>);
    static_assert(!is_assignable_from_error_v<throws, throws>);

    static_assert(is_fully_assignable_v<throws, live_counted>);
    static_assert(is_fully_assignable_v<live_counted, throws>);
    static_assert(is_fully_assignable_v<throws_on_copy, throws>);
    static_assert(is_fully_assignable_v<void, throws>);

    // Copying the error doesn't throw, but moving it does. Assigning an
    // `unexpected` rvalue copies the error, as with std::expected.
    using throws_on_move = throwing_counted<false, true>;
    static_assert(
        !std::is_copy_assignable_v<scn::expected<throws, throws_on_move>>);
    static_assert(
        !std::is_move_assignable_v<scn::expected<throws, throws_on_move>>);
    static_assert(is_assignable_from_error_v<throws, throws_on_move>);
}
#endif

TEST(ExpectedTest, DefaultConstructorNoexcept)
{
    static_assert(std::is_nothrow_default_constructible_v<
                  scn::expected<with_default_constructor<true>, int>>);
    static_assert(!std::is_nothrow_default_constructible_v<
                  scn::expected<with_default_constructor<false>, int>>);
    static_assert(std::is_nothrow_default_constructible_v<
                  scn::expected<std::string, int>>);
}

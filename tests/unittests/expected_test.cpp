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

namespace {
// Counts live objects, and has a potentially-throwing constructor
struct counted {
    static inline int live = 0;

    counted()
    {
        ++live;
    }
    counted(int)
    {
        ++live;
    }
    counted(const counted&)
    {
        ++live;
    }
    counted& operator=(const counted&) = default;
    ~counted()
    {
        --live;
    }
};
}  // namespace

TEST(ExpectedTest, EmplaceOverErrorDestroysError)
{
    counted::live = 0;
    {
        scn::expected<counted, counted> e{scn::unexpect};
        e.emplace(1);
        EXPECT_TRUE(e.has_value());
        EXPECT_EQ(counted::live, 1);
    }
    EXPECT_EQ(counted::live, 0);
}

TEST(ExpectedTest, ConvertingConstructorDoesNotLeak)
{
    counted::live = 0;
    {
        const scn::expected<int, int> error{scn::unexpect, 1};
        scn::expected<counted, long> from_error{error};
        EXPECT_FALSE(from_error.has_value());
        EXPECT_EQ(counted::live, 0);

        const scn::expected<int, int> value{1};
        scn::expected<counted, long> from_value{value};
        EXPECT_TRUE(from_value.has_value());
        EXPECT_EQ(counted::live, 1);
    }
    EXPECT_EQ(counted::live, 0);
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
struct throws_on_nonzero_construct {
    throws_on_nonzero_construct() = default;
    throws_on_nonzero_construct(int value)
    {
        if (value != 0) {
            throw std::runtime_error{"throws_on_nonzero_construct"};
        }
    }
};

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

namespace {
template <bool Noexcept>
struct with_default_constructor {
    with_default_constructor() noexcept(Noexcept) {}
    std::string s;
};
}  // namespace

TEST(ExpectedTest, DefaultConstructorNoexcept)
{
    static_assert(std::is_nothrow_default_constructible_v<
                  scn::expected<with_default_constructor<true>, int>>);
    static_assert(!std::is_nothrow_default_constructible_v<
                  scn::expected<with_default_constructor<false>, int>>);
    static_assert(std::is_nothrow_default_constructible_v<
                  scn::expected<std::string, int>>);
}

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

#include "../wrapped_gtest.h"

#include <scn/impl.h>

#include <iterator>
#include <list>
#include <string>
#include <string_view>
#include <vector>

using namespace std::string_literals;
using namespace std::string_view_literals;

using scn::impl::field_unit;

static_assert(scn::detail::is_code_point_start('a'));
static_assert(scn::detail::is_code_point_start('\xc3'));
static_assert(!scn::detail::is_code_point_start('\x80'));
static_assert(!scn::detail::is_code_point_start('\xbf'));
static_assert(scn::detail::is_code_point_start('\xf8'));
static_assert(scn::detail::is_code_point_start('\xff'));

static_assert(scn::detail::is_code_point_start(u'a'));
static_assert(scn::detail::is_code_point_start(char16_t{0xd800}));
static_assert(!scn::detail::is_code_point_start(char16_t{0xdc00}));
static_assert(!scn::detail::is_code_point_start(char16_t{0xdfff}));
static_assert(scn::detail::is_code_point_start(char16_t{0xe000}));

static_assert(scn::detail::is_code_point_start(U'a'));
static_assert(scn::detail::is_code_point_start(char32_t{0xdc00}));

static_assert(scn::detail::is_code_point_start(L'a'));
static_assert(scn::detail::is_code_point_start(static_cast<wchar_t>(0xdc00)) ==
              (sizeof(wchar_t) == 4));

namespace {
template <field_unit Unit, typename CharT>
std::ptrdiff_t bounded_size(std::basic_string_view<CharT> input,
                            std::ptrdiff_t max)
{
    return scn::impl::find_bounded_end<Unit>(input.data(),
                                             input.data() + input.size(), max) -
           input.data();
}
}  // namespace

TEST(BoundedRangeTest, FindBoundedEndCodeUnits)
{
    EXPECT_EQ(bounded_size<field_unit::code_unit>("abc"sv, 2), 2);
    EXPECT_EQ(bounded_size<field_unit::code_unit>("abc"sv, 5), 3);
    EXPECT_EQ(bounded_size<field_unit::code_unit>("\xc3\xa4"sv, 1), 1);
}

TEST(BoundedRangeTest, FindBoundedEndCodePoints)
{
    // a, U+00E4 (2 bytes), b
    const auto input =
        "a\xc3\xa4"
        "b"sv;
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 0), 0);
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 1), 1);
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 2), 3);
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 3), 4);
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 10), 4);
}

TEST(BoundedRangeTest, FindBoundedEndStrayContinuation)
{
    // Before the bound, a stray continuation byte belongs to the code point
    // before it. After a complete last code point, it's left out.
    const auto input =
        "a\x80"
        "b"sv;
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 1), 1);
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 2), 3);
    EXPECT_EQ(bounded_size<field_unit::code_point>("\xc3\xa4\x80"sv, 1), 2);
    EXPECT_EQ(bounded_size<field_unit::code_point>("\x80\x80"
                                                   "a"sv,
                                                   0),
              0);
    EXPECT_EQ(bounded_size<field_unit::code_point>("\x80\x80"
                                                   "ab"sv,
                                                   1),
              3);
}

TEST(BoundedRangeTest, FindBoundedEndTruncatedCodePoint)
{
    // U+20AC is e2 82 ac: the next code point starts before it's complete
    EXPECT_EQ(bounded_size<field_unit::code_point>("\xe2\x82"
                                                   "a"sv,
                                                   1),
              2);
    EXPECT_EQ(bounded_size<field_unit::code_point>("\xe2\x82"sv, 1), 2);
    EXPECT_EQ(bounded_size<field_unit::code_point>("\xe2\x82\xac\x80"sv, 1), 3);
}

TEST(BoundedRangeTest, FindBoundedEndInvalidLead)
{
    // An invalid lead byte is a code point of its own
    EXPECT_EQ(bounded_size<field_unit::code_point>("\xf8"
                                                   "ab"sv,
                                                   1),
              1);
    EXPECT_EQ(bounded_size<field_unit::code_point>("\xff\x80"
                                                   "a"sv,
                                                   1),
              1);
    EXPECT_EQ(bounded_size<field_unit::code_point>("\xff\x80"
                                                   "a"sv,
                                                   2),
              3);
}

TEST(BoundedRangeTest, FindBoundedEndUtf16)
{
    // a, U+1F602 (surrogate pair), b
    const auto input = u"a\U0001F602b"sv;
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 1), 1);
    EXPECT_EQ(bounded_size<field_unit::code_point>(input, 2), 3);
    EXPECT_EQ(bounded_size<field_unit::code_unit>(input, 2), 2);

    // An unpaired low surrogate after a complete last code point is left out
    const auto stray = u"a\xdc00"sv;
    EXPECT_EQ(bounded_size<field_unit::code_point>(stray, 1), 1);
    EXPECT_EQ(bounded_size<field_unit::code_point>(u"\xd83d"
                                                   u"a"sv,
                                                   1),
              1);
}

TEST(BoundedRangeTest, CountFieldUnits)
{
    const auto count = [](field_unit unit, std::string_view input) {
        const auto* first = input.data();
        const auto* last = input.data() + input.size();
        if (unit == field_unit::code_unit) {
            return scn::impl::count_field_units<field_unit::code_unit>(first,
                                                                       last);
        }
        return scn::impl::count_field_units<field_unit::code_point>(first,
                                                                    last);
    };

    EXPECT_EQ(count(field_unit::code_point,
                    "a\xc3\xa4"
                    "b"),
              3);
    EXPECT_EQ(count(field_unit::code_unit,
                    "a\xc3\xa4"
                    "b"),
              4);
    EXPECT_EQ(count(field_unit::code_point,
                    "a\x80"
                    "b"),
              2);
    EXPECT_EQ(count(field_unit::code_point,
                    "\xf8"
                    "ab"),
              3);
    EXPECT_EQ(count(field_unit::code_point, ""), 0);
}

// Forward iterator over a string that records the furthest position read.
// Not in an anonymous namespace, so that clang doesn't warn about the
// operators only used in unevaluated contexts.
class tracking_iterator {
public:
    using value_type = char;
    using reference = char;
    using pointer = const char*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    tracking_iterator() = default;
    tracking_iterator(const char* ptr, const char** furthest)
        : m_ptr(ptr), m_furthest(furthest)
    {
    }

    char operator*() const
    {
        if (*m_furthest < m_ptr) {
            *m_furthest = m_ptr;
        }
        return *m_ptr;
    }

    tracking_iterator& operator++()
    {
        ++m_ptr;
        return *this;
    }
    tracking_iterator operator++(int)
    {
        auto tmp = *this;
        ++m_ptr;
        return tmp;
    }

    friend bool operator==(const tracking_iterator& a,
                           const tracking_iterator& b)
    {
        return a.m_ptr == b.m_ptr;
    }
    friend bool operator!=(const tracking_iterator& a,
                           const tracking_iterator& b)
    {
        return a.m_ptr != b.m_ptr;
    }

    const char* get() const
    {
        return m_ptr;
    }

private:
    const char* m_ptr{nullptr};
    const char** m_furthest{nullptr};
};

namespace {
template <field_unit Unit>
auto make_tracked_range(std::string_view input,
                        const char** furthest,
                        std::ptrdiff_t max)
{
    *furthest = input.data();
    auto range = scn::ranges::subrange{
        tracking_iterator{input.data(), furthest},
        tracking_iterator{input.data() + input.size(), furthest}};
    return scn::impl::make_bounded_range<Unit>(range, max);
}

template <typename Range>
std::string collect(const Range& range)
{
    std::string result;
    for (auto it = range.begin(); it != range.end(); ++it) {
        result.push_back(*it);
    }
    return result;
}
}  // namespace

TEST(BoundedRangeTest, CodeUnitsStopWithoutPeeking)
{
    const auto input = "12345"sv;
    const char* furthest{};
    auto range = make_tracked_range<field_unit::code_unit>(input, &furthest, 3);

    EXPECT_EQ(collect(range), "123");
    EXPECT_EQ(furthest, input.data() + 2);
}

TEST(BoundedRangeTest, CodePointsStopWithoutPeeking)
{
    // a, U+00E4 (2 bytes), b, c
    const auto input =
        "a\xc3\xa4"
        "bc"sv;
    const char* furthest{};
    auto range =
        make_tracked_range<field_unit::code_point>(input, &furthest, 2);

    EXPECT_EQ(collect(range), "a\xc3\xa4");
    EXPECT_EQ(furthest, input.data() + 2);

    auto it = range.begin();
    std::advance(it, 3);
    EXPECT_EQ(it, range.end());
    EXPECT_EQ(it.base().get(), input.data() + 3);
    EXPECT_EQ(it.remaining(), 0);
    EXPECT_EQ(furthest, input.data() + 2);
}

TEST(BoundedRangeTest, AsciiCodePointsStopWithoutPeeking)
{
    const auto input = "abcd"sv;
    const char* furthest{};
    auto range =
        make_tracked_range<field_unit::code_point>(input, &furthest, 2);

    EXPECT_EQ(collect(range), "ab");
    EXPECT_EQ(furthest, input.data() + 1);
}

TEST(BoundedRangeTest, TruncatedCodePointPeeksOneUnit)
{
    // a, the first two bytes of U+20AC, b
    const auto input =
        "a\xe2\x82"
        "b"sv;
    const char* furthest{};
    auto range =
        make_tracked_range<field_unit::code_point>(input, &furthest, 2);

    EXPECT_EQ(collect(range), "a\xe2\x82");
    EXPECT_EQ(furthest, input.data() + 3);
}

TEST(BoundedRangeTest, StrayContinuationAfterBoundIsLeft)
{
    const auto input =
        "a\xc3\xa4\x80"
        "b"sv;
    const char* furthest{};
    auto range =
        make_tracked_range<field_unit::code_point>(input, &furthest, 2);

    EXPECT_EQ(collect(range), "a\xc3\xa4");
    EXPECT_EQ(furthest, input.data() + 2);
}

TEST(BoundedRangeTest, StrayContinuationBeforeBoundIsRead)
{
    const auto input =
        "a\x80"
        "bc"sv;
    const char* furthest{};
    auto range =
        make_tracked_range<field_unit::code_point>(input, &furthest, 2);

    EXPECT_EQ(collect(range),
              "a\x80"
              "b");
    EXPECT_EQ(furthest, input.data() + 2);
}

TEST(BoundedRangeTest, CodePointsStopAtUnderlyingEnd)
{
    const auto input = "a\xc3\xa4"sv;
    const char* furthest{};
    auto range =
        make_tracked_range<field_unit::code_point>(input, &furthest, 5);

    EXPECT_EQ(collect(range), input);
    EXPECT_EQ(furthest, input.data() + 2);
}

TEST(BoundedRangeTest, RemainingAndDecrement)
{
    // a, U+00E4 (2 bytes), b
    const auto input = std::string{
        "a\xc3\xa4"
        "b"};
    const auto list = std::list<char>(input.begin(), input.end());
    auto range = scn::impl::make_bounded_range<field_unit::code_point>(
        scn::ranges::subrange{list.begin(), list.end()}, 2);

    auto it = range.begin();
    EXPECT_EQ(it.remaining(), 2);
    ++it;
    EXPECT_EQ(it.remaining(), 1);
    ++it;
    EXPECT_EQ(it.remaining(), 0);
    EXPECT_NE(it, range.end());
    ++it;
    EXPECT_EQ(it.remaining(), 0);
    EXPECT_EQ(it, range.end());
    EXPECT_EQ(*it.base(), 'b');

    --it;
    EXPECT_EQ(it.remaining(), 0);
    --it;
    EXPECT_EQ(it.remaining(), 1);
    --it;
    EXPECT_EQ(it.remaining(), 2);
    EXPECT_EQ(it, range.begin());
}

TEST(BoundedRangeTest, DecrementRoundTrips)
{
    // Valid, truncated, and stray continuation units, around the bound
    for (auto input : {"a\xc3\xa4"
                       "b"s,
                       "a\xe2\x82"
                       "b"s,
                       "a\x80\xc3\xa4\x80"
                       "b"s,
                       "\xff\x80"
                       "a"s}) {
        const auto list = std::list<char>(input.begin(), input.end());
        auto range = scn::impl::make_bounded_range<field_unit::code_point>(
            scn::ranges::subrange{list.begin(), list.end()}, 2);

        std::vector<std::ptrdiff_t> remaining;
        auto it = range.begin();
        for (; it != range.end(); ++it) {
            remaining.push_back(it.remaining());
        }
        const auto end = it;
        while (it != range.begin()) {
            --it;
            EXPECT_EQ(it.remaining(), remaining.back());
            remaining.pop_back();
            EXPECT_NE(it, range.end());

            auto forward = it;
            while (forward != range.end()) {
                ++forward;
            }
            EXPECT_EQ(forward, end);
            EXPECT_EQ(forward.base(), end.base());
        }
    }
}

namespace {
template <typename CharT>
void expect_iterator_end_matches_find_bounded_end(
    std::basic_string_view<CharT> input)
{
    const auto list = std::list<CharT>(input.begin(), input.end());
    for (std::ptrdiff_t max = 1; max <= 4; ++max) {
        auto range = scn::impl::make_bounded_range<field_unit::code_point>(
            scn::ranges::subrange{list.begin(), list.end()}, max);
        std::ptrdiff_t size = 0;
        for (auto it = range.begin(); it != range.end(); ++it) {
            ++size;
        }
        EXPECT_EQ(size, bounded_size<field_unit::code_point>(input, max))
            << "max: " << max << ", input size: " << input.size();
    }
}
}  // namespace

TEST(BoundedRangeTest, IteratorEndMatchesFindBoundedEnd)
{
    for (auto input : {
             "abc"sv,
             "a\xc3\xa4"
             "b"sv,
             "\xf0\x9f\x98\x82"
             "a"sv,
             "\xe2\x82"
             "ab"sv,
             "\xe2\x82"sv,
             "a\x80\x80"
             "b\x80"sv,
             "\x80"
             "a\x80"sv,
             "\xc3\xa4\x80\xc3\xa4"sv,
             "\xf8\x80"
             "a\xff\x80\x80"sv,
             "\xf0\x9f\x98\x82\x80\x80"sv,
         }) {
        expect_iterator_end_matches_find_bounded_end(input);
    }

    for (auto input : {u"a\U0001F602b"sv,
                       u"\xd83d"
                       u"ab"sv,
                       u"a\xdc00\xdc00"
                       u"b"sv,
                       u"\U0001F602\xdc00"sv}) {
        expect_iterator_end_matches_find_bounded_end(input);
    }

    expect_iterator_end_matches_find_bounded_end(U"a\U0001F602b"sv);
    expect_iterator_end_matches_find_bounded_end(L"a\u00e4\U0001F602b"sv);
}

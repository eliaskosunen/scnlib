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

#include <deque>
#include <list>

// Sized, but can't be assigned to a `const char*`.
// Not in an anonymous namespace, so that clang doesn't warn about the
// operators only used in unevaluated contexts.
struct sized_end_sentinel {
    const char* end;

    friend bool operator==(const char* it, sized_end_sentinel s)
    {
        return it == s.end;
    }
    friend bool operator==(sized_end_sentinel s, const char* it)
    {
        return it == s.end;
    }
    friend bool operator!=(const char* it, sized_end_sentinel s)
    {
        return it != s.end;
    }
    friend bool operator!=(sized_end_sentinel s, const char* it)
    {
        return it != s.end;
    }
    friend std::ptrdiff_t operator-(sized_end_sentinel s, const char* it)
    {
        return s.end - it;
    }
    friend std::ptrdiff_t operator-(const char* it, sized_end_sentinel s)
    {
        return it - s.end;
    }
};

// Bidirectional iterator over a null-terminated string,
// that counts live instances, to catch bitwise copies.
// Not in an anonymous namespace, so that clang doesn't warn about the
// operators only used in unevaluated contexts.
struct instance_counted_iterator {
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

    static inline int live = 0;

    instance_counted_iterator()
    {
        ++live;
    }
    explicit instance_counted_iterator(const char* p) : ptr(p)
    {
        ++live;
    }
    instance_counted_iterator(const instance_counted_iterator& other)
        : ptr(other.ptr)
    {
        ++live;
    }
    instance_counted_iterator& operator=(const instance_counted_iterator&) =
        default;
    ~instance_counted_iterator()
    {
        --live;
    }

    char operator*() const
    {
        return *ptr;
    }
    instance_counted_iterator& operator++()
    {
        ++ptr;
        return *this;
    }
    instance_counted_iterator operator++(int)
    {
        auto copy = *this;
        ++ptr;
        return copy;
    }
    instance_counted_iterator& operator--()
    {
        --ptr;
        return *this;
    }
    instance_counted_iterator operator--(int)
    {
        auto copy = *this;
        --ptr;
        return copy;
    }

    friend bool operator==(const instance_counted_iterator& a,
                           const instance_counted_iterator& b)
    {
        return a.ptr == b.ptr;
    }
    friend bool operator!=(const instance_counted_iterator& a,
                           const instance_counted_iterator& b)
    {
        return a.ptr != b.ptr;
    }
    friend bool operator==(const instance_counted_iterator& a,
                           scn::ranges::default_sentinel_t)
    {
        return *a.ptr == '\0';
    }
    friend bool operator==(scn::ranges::default_sentinel_t,
                           const instance_counted_iterator& a)
    {
        return *a.ptr == '\0';
    }
    friend bool operator!=(const instance_counted_iterator& a,
                           scn::ranges::default_sentinel_t)
    {
        return *a.ptr != '\0';
    }
    friend bool operator!=(scn::ranges::default_sentinel_t,
                           const instance_counted_iterator& a)
    {
        return *a.ptr != '\0';
    }

    const char* ptr{nullptr};
};

using instance_counted_common_iterator =
    scn::ranges::common_iterator<instance_counted_iterator,
                                 scn::ranges::default_sentinel_t>;

namespace {
struct pair_concat_storage_access
    : scn::ranges::detail::pair_concat_iterator_noncopyable_storage<
          instance_counted_iterator,
          instance_counted_iterator> {
    // User-provided, so that this isn't an aggregate,
    // which would make the protected base constructor inaccessible
    pair_concat_storage_access() {}

    using pair_concat_iterator_noncopyable_storage::_is_first;
};
}  // namespace

TEST(RangesUtilTest, DistanceEmpty)
{
    std::string_view sv = "";
    auto dist = scn::ranges::distance(sv.begin(), sv.end());
    EXPECT_EQ(dist, 0);
}

TEST(RangesUtilTest, DistanceNonEmpty)
{
    std::string_view sv = "hello";
    auto dist = scn::ranges::distance(sv.begin(), sv.end());
    EXPECT_EQ(dist, 5);
}

TEST(RangesUtilTest, AdvanceByZero)
{
    std::string_view sv = "test";
    auto it = sv.begin();
    scn::ranges::advance(it, 0);
    EXPECT_EQ(it, sv.begin());
}

TEST(RangesUtilTest, AdvanceByPositive)
{
    std::string_view sv = "test";
    auto it = sv.begin();
    scn::ranges::advance(it, 2);
    EXPECT_EQ(*it, 's');
}

TEST(RangesUtilTest, AdvanceToEnd)
{
    std::string_view sv = "test";
    auto it = sv.begin();
    scn::ranges::advance(it, sv.end());
    EXPECT_EQ(it, sv.end());
}

TEST(RangesUtilTest, AdvanceBounded)
{
    std::string_view sv = "test";
    auto it = sv.begin();
    auto result = scn::ranges::advance(it, 10, sv.end());
    EXPECT_EQ(it, sv.end());
    EXPECT_EQ(result, 6);  // Moved 4, wanted 10
}

TEST(RangesUtilTest, AdvanceNegativeBoundedAtBound)
{
    std::list<char> list{'a', 'b', 'c', 'd'};
    auto it = list.begin();
    auto result = scn::ranges::advance(it, -3, list.begin());
    EXPECT_EQ(it, list.begin());
    EXPECT_EQ(result, -3);
}

TEST(RangesUtilTest, AdvanceNegativeBoundedHitsBound)
{
    std::list<char> list{'a', 'b', 'c', 'd'};
    auto it = std::next(list.begin(), 2);
    auto result = scn::ranges::advance(it, -5, list.begin());
    EXPECT_EQ(it, list.begin());
    EXPECT_EQ(result, -3);
}

TEST(RangesUtilTest, AdvanceNegativeBoundedWithinBound)
{
    std::list<char> list{'a', 'b', 'c', 'd'};
    auto it = std::next(list.begin(), 3);
    auto result = scn::ranges::advance(it, -2, list.begin());
    EXPECT_EQ(*it, 'b');
    EXPECT_EQ(result, 0);
}

TEST(RangesUtilTest, AdvanceToSizedNonAssignableSentinel)
{
    std::string_view sv = "test";
    const char* it = sv.data();
    scn::ranges::advance(it, sized_end_sentinel{sv.data() + sv.size()});
    EXPECT_EQ(it, sv.data() + sv.size());
}

TEST(RangesUtilTest, ScanBufferIterator)
{
    auto src = std::string_view{"abcdefghijklmnopqrstuvwxyz0123456789"};
    auto deque = std::deque<char>(src.begin(), src.end());
    auto buf = scn::detail::make_range_scan_buffer(deque);

    auto first = buf.get().begin();
    auto it = first;
    scn::ranges::advance(it, 30);
    EXPECT_EQ(*it, '4');
    EXPECT_EQ(scn::ranges::distance(first, it), 30);

    scn::ranges::advance(it, -10);
    EXPECT_EQ(*it, 'u');
    EXPECT_EQ(scn::ranges::distance(first, it), 20);

    EXPECT_EQ(scn::ranges::distance(first, buf.get().end()),
              static_cast<std::ptrdiff_t>(src.size()));
}

TEST(RangesUtilTest, NextDefault)
{
    std::string_view sv = "test";
    auto it = scn::ranges::next(sv.begin());
    EXPECT_EQ(*it, 'e');
}

TEST(RangesUtilTest, NextByN)
{
    std::string_view sv = "test";
    auto it = scn::ranges::next(sv.begin(), 2);
    EXPECT_EQ(*it, 's');
}

TEST(RangesUtilTest, NextToBound)
{
    std::string_view sv = "test";
    auto it = scn::ranges::next(sv.begin(), sv.end());
    EXPECT_EQ(it, sv.end());
}

TEST(RangesUtilTest, NextBounded)
{
    std::string_view sv = "test";
    auto it = scn::ranges::next(sv.begin(), 10, sv.end());
    EXPECT_EQ(it, sv.end());
}

TEST(RangesUtilTest, CommonIteratorDecrement)
{
    const char* str = "abc";
    auto it =
        instance_counted_common_iterator{instance_counted_iterator{str + 1}};
    --it;
    EXPECT_EQ(*it, 'a');
}

TEST(RangesUtilTest, CommonIteratorCopyNonTrivialIterator)
{
    instance_counted_iterator::live = 0;
    {
        const char* str = "abc";
        auto it =
            instance_counted_common_iterator{instance_counted_iterator{str}};
        auto copy = it;
        auto other =
            instance_counted_common_iterator{scn::ranges::default_sentinel};
        other = copy;
        other = std::move(it);
        EXPECT_EQ(*other, 'a');
        EXPECT_EQ(instance_counted_iterator::live, 3);
    }
    EXPECT_EQ(instance_counted_iterator::live, 0);
}

TEST(RangesUtilTest, PairConcatIteratorDefaultConstructed)
{
    instance_counted_iterator::live = 0;
    {
        auto storage = pair_concat_storage_access{};
        EXPECT_TRUE(storage._is_first());
        EXPECT_EQ(instance_counted_iterator::live, 1);
    }
    EXPECT_EQ(instance_counted_iterator::live, 0);
}

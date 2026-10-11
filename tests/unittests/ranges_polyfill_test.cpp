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

#include <cstddef>
#include <iterator>
#include <string_view>

#include <scn/scan.h>

#if SCN_STD >= SCN_STD_20 && defined(__cpp_lib_ranges) && \
    __cpp_lib_ranges >= 201911L &&                        \
    !(SCN_STDLIB_GLIBCXX && SCN_CLANG && SCN_CLANG < SCN_COMPILER(16, 0, 0))
#include <ranges>
#endif

// Not in an anonymous namespace, to not warn about unused members
namespace polyfill_test {
struct minimal_input_iterator {
    using iterator_concept = std::input_iterator_tag;
    using value_type = char;
    using difference_type = std::ptrdiff_t;

    char operator*() const
    {
        return *ptr;
    }
    minimal_input_iterator& operator++()
    {
        ++ptr;
        return *this;
    }
    void operator++(int)
    {
        ++ptr;
    }

    const char* ptr{};
};

// Distinct types, so they can have distinct iterator_traits specializations
template <int Id>
struct member_forward_iterator {
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using reference = const char&;
    using pointer = const char*;
    using iterator_category = std::forward_iterator_tag;

    reference operator*() const
    {
        return *ptr;
    }
    member_forward_iterator& operator++()
    {
        ++ptr;
        return *this;
    }
    member_forward_iterator operator++(int)
    {
        auto copy = *this;
        ++ptr;
        return copy;
    }
    friend bool operator==(member_forward_iterator a, member_forward_iterator b)
    {
        return a.ptr == b.ptr;
    }
    friend bool operator!=(member_forward_iterator a, member_forward_iterator b)
    {
        return a.ptr != b.ptr;
    }

    const char* ptr{};
};

// No member category
struct specialized_iterator {
    using value_type = char;
    using difference_type = std::ptrdiff_t;

    char operator*() const
    {
        return *ptr;
    }
    specialized_iterator& operator++()
    {
        ++ptr;
        return *this;
    }
    specialized_iterator operator++(int)
    {
        auto copy = *this;
        ++ptr;
        return copy;
    }
    friend bool operator==(specialized_iterator a, specialized_iterator b)
    {
        return a.ptr == b.ptr;
    }
    friend bool operator!=(specialized_iterator a, specialized_iterator b)
    {
        return a.ptr != b.ptr;
    }

    const char* ptr{};
};

struct unrelated_reference {};

// No category, and iterator_traits is the primary template. The reference type
// has nothing in common with value_type, so not even C++20's iterator_traits
// can deduce an input category for it.
struct fallback_iterator {
    using value_type = char;
    using difference_type = std::ptrdiff_t;

    unrelated_reference operator*() const
    {
        return {};
    }
    unrelated_reference operator[](difference_type) const
    {
        return {};
    }
    fallback_iterator& operator++()
    {
        ++ptr;
        return *this;
    }
    fallback_iterator operator++(int)
    {
        auto copy = *this;
        ++ptr;
        return copy;
    }
    fallback_iterator& operator--()
    {
        --ptr;
        return *this;
    }
    fallback_iterator operator--(int)
    {
        auto copy = *this;
        --ptr;
        return copy;
    }
    fallback_iterator& operator+=(difference_type n)
    {
        ptr += n;
        return *this;
    }
    fallback_iterator& operator-=(difference_type n)
    {
        ptr -= n;
        return *this;
    }
    friend fallback_iterator operator+(fallback_iterator it, difference_type n)
    {
        return it += n;
    }
    friend fallback_iterator operator+(difference_type n, fallback_iterator it)
    {
        return it += n;
    }
    friend fallback_iterator operator-(fallback_iterator it, difference_type n)
    {
        return it -= n;
    }
    friend difference_type operator-(fallback_iterator a, fallback_iterator b)
    {
        return a.ptr - b.ptr;
    }
    friend bool operator==(fallback_iterator a, fallback_iterator b)
    {
        return a.ptr == b.ptr;
    }
    friend bool operator!=(fallback_iterator a, fallback_iterator b)
    {
        return a.ptr != b.ptr;
    }
    friend bool operator<(fallback_iterator a, fallback_iterator b)
    {
        return a.ptr < b.ptr;
    }
    friend bool operator>(fallback_iterator a, fallback_iterator b)
    {
        return a.ptr > b.ptr;
    }
    friend bool operator<=(fallback_iterator a, fallback_iterator b)
    {
        return a.ptr <= b.ptr;
    }
    friend bool operator>=(fallback_iterator a, fallback_iterator b)
    {
        return a.ptr >= b.ptr;
    }

    const char* ptr{};
};

using traits_member_iterator = member_forward_iterator<0>;
using overridden_iterator = member_forward_iterator<1>;
}  // namespace polyfill_test

using namespace polyfill_test;

template <>
struct std::iterator_traits<polyfill_test::specialized_iterator> {
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using reference = char;
    using pointer = const char*;
    using iterator_category = std::forward_iterator_tag;
};

// The specialization overrides the member category
template <>
struct std::iterator_traits<polyfill_test::overridden_iterator> {
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using reference = const char&;
    using pointer = const char*;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::input_iterator_tag;
};

TEST(RangesPolyfillTest, IterConceptFromMember)
{
    static_assert(scn::ranges::input_iterator<minimal_input_iterator>);
    static_assert(!scn::ranges::forward_iterator<minimal_input_iterator>);

    static_assert(scn::ranges::forward_iterator<traits_member_iterator>);
    static_assert(!scn::ranges::bidirectional_iterator<traits_member_iterator>);
}

TEST(RangesPolyfillTest, IterConceptFromIteratorTraits)
{
    static_assert(scn::ranges::forward_iterator<specialized_iterator>);
    static_assert(!scn::ranges::bidirectional_iterator<specialized_iterator>);

    static_assert(scn::ranges::input_iterator<overridden_iterator>);
    static_assert(!scn::ranges::forward_iterator<overridden_iterator>);
}

TEST(RangesPolyfillTest, IterConceptPointers)
{
    static_assert(scn::ranges::random_access_iterator<const char*>);
    static_assert(scn::ranges::random_access_iterator<int*>);
    static_assert(!scn::ranges::input_iterator<void*>);
}

// The fallback needs to know if iterator_traits is the primary template
#if SCN_STDLIB_GLIBCXX || SCN_STDLIB_LIBCPP || SCN_STDLIB_MS_STL
TEST(RangesPolyfillTest, IterConceptRandomAccessFallback)
{
    static_assert(
        std::is_same_v<scn::ranges::detail::iter_concept_t<fallback_iterator>,
                       scn::ranges::random_access_iterator_tag>);
    static_assert(scn::ranges::random_access_iterator<fallback_iterator>);
}
#endif

#if SCN_STD >= SCN_STD_20 && defined(__cpp_lib_ranges) && \
    __cpp_lib_ranges >= 201911L &&                        \
    !(SCN_STDLIB_GLIBCXX && SCN_CLANG && SCN_CLANG < SCN_COMPILER(16, 0, 0))
TEST(RangesPolyfillTest, StdCommonIterator)
{
    using range_type =
        decltype(std::string_view{"12 a"} |
                 std::views::take_while([](char c) { return c != 'a'; }) |
                 std::views::common);
    static_assert(scn::ranges::input_range<range_type>);
    static_assert(scn::ranges::forward_range<range_type>);
}
#endif

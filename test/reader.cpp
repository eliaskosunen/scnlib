// Copyright 2017 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License{");
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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "test.h"

#include <deque>

template <typename CharT>
struct indirect_range {
    using value_type = scn::expected<CharT>;
    using reference = scn::expected<CharT>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    struct iterator {
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;
        using value_type = scn::expected<CharT>;
        using reference = scn::expected<CharT>;
        using pointer = scn::expected<CharT>*;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator() noexcept = default;
        iterator(const indirect_range& r, difference_type i) noexcept
            : range(&r), index(i)
        {
        }

        scn::expected<CharT> operator*() const
        {
            return range->operator[](static_cast<size_type>(index));
        }

        iterator& operator++() noexcept
        {
            ++index;
            return *this;
        }
        iterator operator++(int) noexcept
        {
            auto tmp = *this;
            operator++();
            return tmp;
        }

        iterator& operator--() noexcept
        {
            --index;
            return *this;
        }
        iterator operator--(int) noexcept
        {
            auto tmp = *this;
            operator--();
            return tmp;
        }

        bool operator==(const iterator& o) const noexcept
        {
            if (_is_end() && o._is_end()) {
                return true;
            }
            return range == o.range && index == o.index;
        }
        bool operator!=(const iterator& o) const noexcept
        {
            return !operator==(o);
        }

        bool _is_end() const noexcept
        {
            if (!range) {
                return true;
            }
            if (static_cast<size_t>(index) == range->size()) {
                return true;
            }
            return false;
        }

        const indirect_range* range{nullptr};
        difference_type index{0};
    };
    using const_iterator = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using storage_type = std::vector<scn::expected<CharT>>;

    indirect_range() = default;

    void push_back(scn::expected<CharT> val)
    {
        storage.push_back(val);
    }

    const_iterator begin() const noexcept
    {
        return {*this, 0};
    }
    const_iterator end() const noexcept
    {
        return {*this, static_cast<difference_type>(size())};
    }

    size_type size() const noexcept
    {
        return storage.size();
    }

    scn::expected<CharT> operator[](size_type i) const
    {
        return storage[i];
    }

    storage_type storage;
};
static_assert(SCN_CHECK_CONCEPT(scn::polyfill_2a::bidirectional_iterator<
                                indirect_range<char>::iterator>),
              "indirect_range::iterator is a BidirectionalIterator");
static_assert(SCN_CHECK_CONCEPT(scn::ranges::range<indirect_range<char>>),
              "indirect_range is a Range");
static_assert(!scn::detail::is_direct_impl<indirect_range<char>>::value,
              "indirect_range is not direct");

static indirect_range<char> get_indirect(const std::string& content)
{
    indirect_range<char> src;
    for (auto ch : content) {
        src.push_back({ch});
    }
    src.push_back(scn::error{scn::error::end_of_range, "EOF"});
    return src;
}

TEST_CASE("read_char")
{
    SUBCASE("direct")
    {
        auto range = scn::wrap("42");
        auto ret = scn::read_char(range, false);
        CHECK(ret);
        CHECK(ret.value() == '4');

        ret = scn::read_char(range);
        CHECK(ret.value() == '4');

        CHECK(*range.begin() == '2');
        range.advance();

        ret = scn::read_char(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
    SUBCASE("indirect")
    {
        auto range = scn::wrap(get_indirect("42"));
        auto ret = scn::read_char(range, false);
        CHECK(ret);
        CHECK(ret.value() == '4');

        ret = scn::read_char(range);
        CHECK(ret.value() == '4');

        CHECK((*range.begin()).value() == '2');
        range.advance();

        ret = scn::read_char(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE("read_zero_copy")
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap("123");
        auto ret = scn::read_zero_copy(range, 2);
        CHECK(ret);
        CHECK(ret.value().size() == 2);
        CHECK(ret.value()[0] == '1');
        CHECK(ret.value()[1] == '2');

        CHECK(*range.begin() == '3');
        range.advance();

        ret = scn::read_zero_copy(range, 1);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque());
        auto ret = scn::read_zero_copy(range, 2);
        CHECK(ret);
        CHECK(ret.value().size() == 0);
        CHECK(range.size() == 3);

        range = scn::wrap(get_empty_deque());
        ret = scn::read_zero_copy(range, 2);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE("read_all_zero_copy")
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap("123");
        auto ret = scn::read_all_zero_copy(range);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == '1');
        CHECK(ret.value()[1] == '2');
        CHECK(ret.value()[2] == '3');

        CHECK(range.begin() == range.end());
        ret = scn::read_all_zero_copy(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque());
        auto ret = scn::read_all_zero_copy(range);
        CHECK(ret);
        CHECK(ret.value().size() == 0);
        CHECK(range.size() == 3);

        range = scn::wrap(get_empty_deque());
        ret = scn::read_all_zero_copy(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE("read_into")
{
    SUBCASE("contiguous + direct")
    {
        auto range = scn::wrap("123");
        std::vector<char> data{};
        auto it = std::back_inserter(data);
        auto ret = scn::read_into(range, it, 2);
        CHECK(ret);
        CHECK(data.size() == 2);
        CHECK(data[0] == '1');
        CHECK(data[1] == '2');

        data.clear();
        ret = scn::read_into(range, it, 2);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == '3');

        ret = scn::read_into(range, it, 1);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == '3');
    }

    SUBCASE("direct")
    {
        auto range = scn::wrap(get_deque());
        std::vector<char> data{};
        auto it = std::back_inserter(data);
        auto ret = scn::read_into(range, it, 2);
        CHECK(ret);
        CHECK(data.size() == 2);
        CHECK(data[0] == '1');
        CHECK(data[1] == '2');

        data.clear();
        ret = scn::read_into(range, it, 2);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == '3');

        ret = scn::read_into(range, it, 1);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == '3');
    }

    SUBCASE("indirect")
    {
        auto range = scn::wrap(get_indirect("123"));
        std::vector<char> data{};
        auto it = std::back_inserter(data);
        auto ret = scn::read_into(range, it, 2);
        CHECK(ret);
        CHECK(data.size() == 2);
        CHECK(data[0] == '1');
        CHECK(data[1] == '2');

        data.clear();
        ret = scn::read_into(range, it, 2);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == '3');

        ret = scn::read_into(range, it, 1);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == '3');
    }
}

static bool pred_is_space(char ch)
{
    return ch == ' ';
}

TEST_CASE("read_until_space_zero_copy no final space")
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap("123 456");
        auto ret = scn::read_until_space_zero_copy(range, pred_is_space, false);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == '1');
        CHECK(ret.value()[1] == '2');
        CHECK(ret.value()[2] == '3');

        CHECK(*range.begin() == ' ');
        range.advance();

        ret = scn::read_until_space_zero_copy(range, pred_is_space, false);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == '4');
        CHECK(ret.value()[1] == '5');
        CHECK(ret.value()[2] == '6');

        CHECK(range.begin() == range.end());
    }

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque("123 456"));
        auto ret = scn::read_until_space_zero_copy(range, pred_is_space, false);
        CHECK(ret);
        CHECK(ret.value().size() == 0);

        range.advance(7);
        ret = scn::read_until_space_zero_copy(range, pred_is_space, false);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE("read_until_space_zero_copy keep final space")
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap("123 456");
        auto ret = scn::read_until_space_zero_copy(range, pred_is_space, true);
        CHECK(ret);
        CHECK(ret.value().size() == 4);
        CHECK(ret.value()[0] == '1');
        CHECK(ret.value()[1] == '2');
        CHECK(ret.value()[2] == '3');
        CHECK(ret.value()[3] == ' ');

        ret = scn::read_until_space_zero_copy(range, pred_is_space, true);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == '4');
        CHECK(ret.value()[1] == '5');
        CHECK(ret.value()[2] == '6');

        CHECK(range.begin() == range.end());
    }

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque("123 456"));
        auto ret = scn::read_until_space_zero_copy(range, pred_is_space, true);
        CHECK(ret);
        CHECK(ret.value().size() == 0);

        range.advance(7);
        ret = scn::read_until_space_zero_copy(range, pred_is_space, true);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

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

#include <scn/scn.h>

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
#include <algorithm>
#include <array>
#include <deque>
#include <exception>
#include <ostream>
#include <string>
#include <vector>
SCN_GCC_POP

#ifndef SCN_SKIP_DOCTEST
#include <doctest/doctest.h>
#endif

template <typename T>
struct debug;

template <typename CharT>
std::basic_string<CharT> widen(const std::string&)
{
    return {};
}
template <>
inline std::basic_string<char> widen<char>(const std::string& str)
{
    return str;
}
template <>
inline std::basic_string<wchar_t> widen<wchar_t>(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}

template <typename CharT, typename Input, typename Fmt, typename... T>
auto do_scan(Input&& i, Fmt f, T&... a)
    -> decltype(scn::scan(widen<CharT>(std::forward<Input>(i)),
                          widen<CharT>(f).c_str(),
                          a...))
{
    return scn::scan(widen<CharT>(std::forward<Input>(i)),
                     widen<CharT>(f).c_str(), a...);
}

template <typename CharT>
std::deque<CharT> get_deque(
    const std::basic_string<CharT>& content = widen<CharT>("123"))
{
    std::deque<CharT> src{};
    for (auto ch : content) {
        src.push_back(ch);
    }
    return src;
}
template <typename CharT>
std::deque<CharT> get_empty_deque()
{
    return {};
}

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
        const scn::expected<CharT>* operator->() const
        {
            return &range->storage[static_cast<size_type>(index)];
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
    indirect_range(iterator begin, iterator end)
    {
        for (; begin != end; ++begin) {
            push_back(*begin);
        }
    }

    void push_back(scn::expected<CharT> val)
    {
        storage.push_back(val);
    }

    void set(std::vector<scn::expected<CharT>>&& o)
    {
        storage = SCN_MOVE(o);
    }
    std::vector<scn::expected<CharT>> extract() &&
    {
        return storage;
    }
    std::vector<scn::expected<CharT>>& get()
    {
        return storage;
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

    const scn::expected<CharT>& operator[](size_type i) const
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

template <typename CharT>
indirect_range<CharT> get_indirect(const std::basic_string<CharT>& content)
{
    indirect_range<CharT> src;
    for (auto ch : content) {
        src.push_back({ch});
    }
    src.push_back(scn::error{scn::error::end_of_range, "EOF"});
    return src;
}

template <typename T>
bool consistency_iostream(std::string& source, T& val)
{
    std::istringstream ss{source};
    ss >> val;
    bool res = !(ss.fail() || ss.bad());

    source.clear();
    auto in_avail = ss.rdbuf()->in_avail();
    source.resize(static_cast<size_t>(in_avail));
    ss.rdbuf()->sgetn(&source[0], in_avail);

    return res;
}
template <typename T>
bool consistency_scanf(std::string& source, const std::string& fmt, T& val)
{
    size_t nchar{0};
    auto f = fmt + "%zn";

    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wformat-nonliteral")
    int nargs = std::sscanf(source.c_str(), f.c_str(), &val, &nchar);
    SCN_GCC_COMPAT_POP

    if (nargs == EOF) {
        return false;
    }
    SCN_ENSURE(nchar <= source.size());

    source = source.substr(nchar);
    return nargs == 1;
}

#define DOCTEST_VALUE_PARAMETERIZED_DATA(data, data_array)                     \
    SCN_CLANG_PUSH SCN_CLANG_IGNORE("-Wexit-time-destructors") {}              \
    static std::vector<std::string> _doctest_subcases = [&(data_array)]() {    \
        std::vector<std::string> out;                                          \
        while (out.size() != (data_array).size())                              \
            out.push_back(std::string(#data_array "[") +                       \
                          std::to_string(out.size() + 1) + "]");               \
        return out;                                                            \
    }();                                                                       \
    size_t _doctest_subcase_idx = 0;                                           \
    std::for_each(                                                             \
        (data_array).begin(), (data_array).end(),                              \
        [&](const decltype(data)& in) {                                        \
            DOCTEST_SUBCASE(_doctest_subcases[_doctest_subcase_idx++].c_str()) \
            {                                                                  \
                (data) = in;                                                   \
            }                                                                  \
        }) SCN_CLANG_POP

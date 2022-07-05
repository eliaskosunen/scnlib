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

#pragma once

#include <scn/fwd.h>

#if SCN_USE_IOSTREAMS

#include <scn/detail/args.h>
#include <scn/detail/erased_range.h>
#include <scn/detail/istream_range.h>
#include <scn/detail/ranges.h>
#include <scn/detail/scanner_builtin.h>
#include <scn/util/expected.h>

#include <ios>
#include <streambuf>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename T, typename CharT, typename Enable = void>
        struct is_streamable_impl : std::false_type {};

        template <typename T, typename CharT>
        struct is_streamable_impl<
            T,
            CharT,
            std::enable_if_t<sizeof(SCN_DECLVAL(std::basic_istream<CharT>&)
                                    << std::declval<T>()) != 0>>
            : std::true_type {};

        template <typename CharT>
        struct dummy_context_for_is_streamamble {
            using char_type = CharT;
        };

        template <typename T, typename CharT>
        struct is_streamable
            : std::conditional_t<
                  std::is_convertible_v<
                      std::add_lvalue_reference_t<
                          decltype(arg_mapper<dummy_context_for_is_streamamble<
                                       CharT>>::map(SCN_DECLVAL(T&)))>,
                      unscannable&>,
                  is_streamable_impl<T, CharT>,
                  std::false_type> {};

        template <typename SourceRange>
        class range_streambuf
            : public std::basic_streambuf<ranges::range_value_t<SourceRange>> {
            using base =
                std::basic_streambuf<ranges::range_value_t<SourceRange>>;

        public:
            using range_type = SourceRange;
            using iterator = ranges::iterator_t<SourceRange>;
            using char_type = typename base::char_type;
            using traits_type = typename base::traits_type;
            using int_type = typename base::int_type;

            explicit range_streambuf(range_type range)
                : m_range(range),
                  m_begin(ranges::begin(m_range)),
                  m_begin_prev(m_begin)
            {
            }

            iterator begin() const
                SCN_NOEXCEPT_P(std::is_nothrow_copy_constructible_v<iterator>)
            {
                return m_begin;
            }
            iterator begin_prev() const
                SCN_NOEXCEPT_P(std::is_nothrow_copy_constructible_v<iterator>)
            {
                return m_begin_prev;
            }
            int_type last_char() const SCN_NOEXCEPT
            {
                return m_ch;
            }

        private:
            int_type underflow() override;
            int_type uflow() override;
            std::streamsize showmanyc() override;
            int_type pbackfail(int_type) override;

            range_type m_range;
            iterator m_begin;
            iterator m_begin_prev;
            int_type m_ch{traits_type::eof()};
            bool m_has_put_back{false};
        };

#define SCN_DECLARE_EXTERN_RANGE_STREAMBUF(Range)                              \
    extern template auto range_streambuf<Range>::underflow()->int_type;        \
    extern template auto range_streambuf<Range>::uflow()->int_type;            \
    extern template auto range_streambuf<Range>::showmanyc()->std::streamsize; \
    extern template auto range_streambuf<Range>::pbackfail(int_type)->int_type;

        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(
            scanner_scan_contexts::sv::subrange_type)
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(
            scanner_scan_contexts::wsv::subrange_type)
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(istreambuf_subrange)
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(wistreambuf_subrange)
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(erased_subrange)
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(werased_subrange)

#undef SCN_DECLARE_EXTERN_RANGE_STREAMBUF
    }  // namespace detail

    template <typename CharT>
    struct basic_istream_scanner
        : scanner<std::basic_string_view<CharT>, CharT> {
        template <typename T, typename Context>
        scan_expected<typename Context::iterator> scan(T& val,
                                                       Context& ctx) const
        {
            detail::range_streambuf<typename Context::subrange_type> streambuf(
                ctx.range());
            using traits = typename decltype(streambuf)::traits_type;
            std::basic_istream<CharT> stream(std::addressof(streambuf));

            if (!(stream >> val)) {
                if (stream.eof()) {
                    return unexpected_scan_error(scan_error::end_of_range,
                                                 "EOF");
                }
                if (stream.bad()) {
                    return unexpected_scan_error(
                        scan_error::bad_source_error,
                        "Bad std::istream after reading");
                }
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Failed to read with std::istream");
            }

            if (traits::eq_int_type(streambuf.last_char(), traits::eof())) {
                return streambuf.begin();
            }
            return streambuf.begin_prev();
        }
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_USE_IOSTREAMS

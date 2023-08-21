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

#include <scn/detail/scanner.h>
#include <scn/impl/algorithms/read.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        struct reader_error_handler {
            constexpr void on_error(const char* msg)
            {
                SCN_UNLIKELY_ATTR
                m_msg = msg;
            }
            explicit constexpr operator bool() const
            {
                return m_msg == nullptr;
            }

            const char* m_msg{nullptr};
        };

        template <typename SourceRange>
        scan_expected<simple_borrowed_iterator_t<SourceRange>>
        skip_classic_whitespace(SourceRange&& range,
                                bool allow_exhaustion = false)
        {
            if (!allow_exhaustion) {
                return read_while_classic_space(range).and_then(
                    [&](auto it) -> scan_expected<decltype(it)> {
                        if (auto e = eof_check(
                                ranges::subrange{it, ranges::end(range)});
                            SCN_UNLIKELY(!e)) {
                            return unexpected(e);
                        }

                        return it;
                    });
            }

            return read_while_classic_space(SCN_FWD(range));
        }

        template <typename SourceRange>
        scan_expected<simple_borrowed_iterator_t<SourceRange>>
        skip_localized_whitespace(SourceRange&& range,
                                  detail::locale_ref loc,
                                  bool allow_exhaustion = false)
        {
            if (!allow_exhaustion) {
                return read_while_localized_mask(range, loc,
                                                 std::ctype_base::space)
                    .and_then([&](auto it) -> scan_expected<decltype(it)> {
                        if (auto e = eof_check(
                                ranges::subrange{it, ranges::end(range)});
                            SCN_UNLIKELY(!e)) {
                            return unexpected(e);
                        }
                        return it;
                    });
            }

            return read_while_localized_mask(SCN_FWD(range), loc,
                                             std::ctype_base::space);
        }

#if 0
        template <typename T, typename CharT, typename Enable = void>
        class reader;
#endif

        template <typename Derived, typename CharT>
        class reader_base {
        public:
            using char_type = CharT;

            constexpr reader_base() = default;

            bool skip_ws_before_read() const
            {
                return true;
            }

            scan_error check_specs(
                const detail::basic_format_specs<char_type>& specs)
            {
                reader_error_handler eh{};
                get_derived().check_specs_impl(specs, eh);
                if (SCN_UNLIKELY(!eh)) {
                    return {scan_error::invalid_format_string, eh.m_msg};
                }
                return {};
            }

        private:
            Derived& get_derived()
            {
                return static_cast<Derived&>(*this);
            }
            const Derived& get_derived() const
            {
                return static_cast<const Derived&>(*this);
            }
        };

        template <typename CharT>
        class reader_impl_for_monostate {
        public:
            constexpr reader_impl_for_monostate() = default;

            bool skip_ws_before_read() const
            {
                return true;
            }

            static scan_error check_specs(
                const detail::basic_format_specs<CharT>&)
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_default(Range&&, detail::monostate&, detail::locale_ref)
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>> read_specs(
                Range&&,
                const detail::basic_format_specs<CharT>&,
                detail::monostate&,
                detail::locale_ref)
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn

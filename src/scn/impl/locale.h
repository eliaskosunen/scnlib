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

#include <scn/detail/locale_ref.h>

#include <cstring>
#include <clocale>
#include <locale>
#include <tuple>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        extern template locale_ref::locale_ref(const std::locale&);
        extern template auto locale_ref::get() const -> std::locale;
    }  // namespace detail

    namespace impl {
        template <typename Facet>
        const Facet& get_facet(detail::locale_ref loc)
        {
            auto stdloc = loc.get<std::locale>();
            SCN_EXPECT(std::has_facet<Facet>(stdloc));
            return std::use_facet<Facet>(stdloc);
        }

        template <typename Facet>
        const Facet& get_or_add_facet(std::locale& stdloc)
        {
            if (std::has_facet<Facet>(stdloc)) {
                return std::use_facet<Facet>(stdloc);
            }
            stdloc = std::locale(stdloc, new Facet{});
            return std::use_facet<Facet>(stdloc);
        }

        class clocale_restorer {
        public:
            clocale_restorer(int cat) : m_category(cat)
            {
                const auto loc = std::setlocale(cat, nullptr);
                std::strcpy(m_locbuf, loc);
            }
            ~clocale_restorer()
            {
                // Restore locale to what it was before
                std::setlocale(m_category, m_locbuf);
            }

            clocale_restorer(const clocale_restorer&) = delete;
            clocale_restorer(clocale_restorer&&) = delete;
            clocale_restorer& operator=(const clocale_restorer&) = delete;
            clocale_restorer& operator=(clocale_restorer&&) = delete;

        private:
            // For whatever reason, this cannot be stored in the heap if
            // setlocale hasn't been called before, or msan errors with
            // 'use-of-unitialized-value' when resetting the locale
            // back. POSIX specifies that the content of loc may not be
            // static, so we need to save it ourselves
            char m_locbuf[64] = {0};

            int m_category;
        };

        template <typename CharT>
        struct localized_number_formatting_options {
            localized_number_formatting_options() = default;

            localized_number_formatting_options(detail::locale_ref loc)
            {
                const auto& numpunct = get_facet<std::numpunct<CharT>>(loc);
                grouping = numpunct.grouping();
                thousands_sep = grouping.length() != 0
                                    ? numpunct.thousands_sep()
                                    : CharT{0};
                decimal_point = numpunct.decimal_point();
            }

            std::string grouping{};
            CharT thousands_sep{0};
            CharT decimal_point{CharT{'.'}};
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn

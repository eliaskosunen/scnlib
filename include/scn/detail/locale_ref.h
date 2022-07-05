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

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        class locale_ref {
        public:
            constexpr locale_ref() = default;

            template <typename Locale>
            explicit locale_ref(const Locale& loc);

            constexpr explicit operator bool() const SCN_NOEXCEPT
            {
                return m_locale != nullptr;
            }

            template <typename Locale>
            Locale get() const;

        private:
            const void* m_locale{nullptr};
        };
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

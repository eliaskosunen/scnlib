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

#include <scn/impl/reader/common.h>
#include <string>
#include <string_view>
#include "scn/impl/algorithms/common.h"
#include "scn/impl/algorithms/read_nocopy.h"
#include "scn/impl/util/text_width.h"
#include "scn/util/expected.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename SourceCharT, typename DestCharT>
        scan_error transcode_impl(std::basic_string_view<SourceCharT> src,
                                  std::basic_string<DestCharT>& dst)
        {
            dst.clear();
            if (!transcode_to_string(src, dst)) {
                SCN_UNLIKELY_ATTR
                return scan_error(scan_error::invalid_encoding,
                                  "Failed to transcode string value");
            }
            return {};
        }

        template <typename SourceCharT, typename DestCharT>
        scan_error transcode_if_necessary(
            const contiguous_range_factory<SourceCharT>& source,
            std::basic_string<DestCharT>& dest)
        {
            if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
                dest.assign(source.view());
            }
            else {
                return transcode_impl(source.view(), dest);
            }

            return {};
        }

        template <typename SourceCharT, typename DestCharT>
        scan_error transcode_if_necessary(
            contiguous_range_factory<SourceCharT>&& source,
            std::basic_string<DestCharT>& dest)
        {
            if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
                dest.assign(SCN_MOVE(source.get_allocated_string()));
            }
            else {
                return transcode_impl(source.view(), dest);
            }

            return {};
        }

        template <typename SourceCharT, typename DestCharT>
        scan_error transcode_if_necessary(
            string_view_wrapper<SourceCharT> source,
            std::basic_string<DestCharT>& dest)
        {
            if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
                dest.assign(source.view());
            }
            else {
                return transcode_impl(source.view(), dest);
            }

            return {};
        }

        template <typename Range, typename Iterator, typename ValueCharT>
        auto read_string_impl(Range& range,
                              scan_expected<Iterator>&& result,
                              std::basic_string<ValueCharT>& value)
        {
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }

            auto src = make_contiguous_buffer(
                ranges::subrange{ranges::begin(range), *result});
            if (auto e = transcode_if_necessary(SCN_MOVE(src), value);
                SCN_UNLIKELY(!e)) {
                return unexpected(e);
            }

            return *result;
        }

        template <typename Range, typename Iterator, typename ValueCharT>
        auto read_string_view_impl(Range& range,
                                   scan_expected<Iterator>&& result,
                                   std::basic_string_view<ValueCharT>& value)
        {
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }

            auto src = make_contiguous_buffer(
                ranges::subrange{ranges::begin(range), *result});
            if (src.stores_allocated_string()) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Cannot read a string_view from this source range (not "
                    "contiguous)");
            }
            if constexpr (!std::is_same_v<typename decltype(src)::char_type,
                                          ValueCharT>) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Cannot read a string_view from "
                                             "this source range (would require "
                                             "transcoding)");
            }

            return *result;
        }

        template <typename SourceCharT>
        class word_reader_impl {
        public:
            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read(
                Range&& range,
                std::basic_string<ValueCharT>& value)
            {
                return read_string_impl(range, read_until_classic_space(range),
                                        value);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read(
                Range&& range,
                std::basic_string_view<ValueCharT>& value)
            {
                return read_string_view_impl(
                    range, read_until_classic_space(range), value);
            }
        };

        template <typename SourceCharT>
        class character_reader_impl {
        public:
            template <typename View, typename ValueCharT>
            auto read(take_width_view<View>& range,
                      std::basic_string<ValueCharT>& value)
                -> scan_expected<ranges::iterator_t<take_width_view<View>&>>
            {
                return read_string_impl(range, read_all(range), value);
            }

            template <typename View, typename ValueCharT>
            auto read(take_width_view<View>& range,
                      std::basic_string_view<ValueCharT>& value)
                -> scan_expected<ranges::iterator_t<take_width_view<View>&>>
            {
                return read_string_view_impl(range, read_all(range), value);
            }
        };

        template <typename SourceCharT>
        class string_reader
            : public reader_base<string_reader<SourceCharT>, SourceCharT> {
        public:
            constexpr string_reader() = default;

            static void check_specs_impl(
                const detail::basic_format_specs<SourceCharT>& specs,
                reader_error_handler& eh)
            {
                detail::check_string_type_specs(specs, eh);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read(
                Range&& range,
                std::basic_string<ValueCharT>& value)
            {
                return word_reader_impl<SourceCharT>{}.read(SCN_FWD(range),
                                                            value);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read(
                Range&& range,
                std::basic_string_view<ValueCharT>& value)
            {
                return word_reader_impl<SourceCharT>{}.read(SCN_FWD(range),
                                                            value);
            }
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn

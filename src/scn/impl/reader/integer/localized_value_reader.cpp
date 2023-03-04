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

#include <scn/impl/reader/integer/localized_value_reader.h>

#include <sstream>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        void int_localized_value_reader<CharT>::make_istream(
            std::basic_istream<CharT>& ss) const
        {
            switch (m_base) {
                case 8:
                    ss.setf(std::ios_base::oct, std::ios_base::basefield);
                    break;
                case 10:
                    ss.setf(std::ios_base::dec, std::ios_base::basefield);
                    break;
                case 16:
                    ss.setf(std::ios_base::hex, std::ios_base::basefield);
                    break;
                case 0:
                    ss.setf(std::ios_base::fmtflags(0),
                            std::ios_base::basefield);
                    break;
                default:
                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
            }
        }

        namespace {
            template <typename T, typename CharT>
            scan_error reject_minus_sign_if_unsigned(
                std::basic_string_view<CharT> source)
            {
                if constexpr (std::is_unsigned_v<T>) {
                    if (source.front() == CharT{'-'}) {
                        return {scan_error::invalid_scanned_value,
                                "Unexpected sign '-' when scanning an "
                                "unsigned integer"};
                    }
                }
                SCN_UNUSED(source);
                return {};
            }

            template <typename CharT, typename T, typename Facet>
            auto do_get_facet(const Facet& facet,
                              std::ios_base& stream,
                              std::ios_base::iostate& err,
                              std::basic_string_view<CharT> source,
                              T& value)
            {
                return facet.get(ranges::begin(source), ranges::end(source),
                                 stream, err, value);
            }

            template <typename T>
            scan_error check_range_supported(T value,
                                             std::ios_base::iostate err)
            {
                if ((err & std::ios_base::failbit) == 0) {
                    return {};
                }

                if (value == std::numeric_limits<T>::max()) {
                    return scan_error{scan_error::value_out_of_range,
                                      "Out of range: integer overflow"};
                }
                if constexpr (std::is_signed_v<T>) {
                    if (value == std::numeric_limits<T>::min()) {
                        return scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer underflow"};
                    }
                }
                return scan_error{scan_error::invalid_scanned_value,
                                  "Failed to scan int"};
            }

            template <typename RequestedT, typename ScannedT>
            scan_error check_range_unsupported(ScannedT value,
                                               std::ios_base::iostate err)
            {
                if (value > static_cast<ScannedT>(
                                std::numeric_limits<RequestedT>::max())) {
                    return scan_error{scan_error::value_out_of_range,
                                      "Out of range: integer overflow"};
                }
                if constexpr (std::is_signed_v<RequestedT>) {
                    if (value < static_cast<ScannedT>(
                                    std::numeric_limits<RequestedT>::min())) {
                        return scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer underflow"};
                    }
                }

                if ((err & std::ios_base::failbit) == 0) {
                    return {};
                }

                return scan_error{scan_error::invalid_scanned_value,
                                  "Failed to scan int"};
            }

            template <typename T, typename CharT, typename Facet>
            scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>
            do_get_supported_impl(const Facet& facet,
                                  std::ios_base& stream,
                                  std::ios_base::iostate& err,
                                  std::basic_string_view<CharT> source,
                                  T& value)
            {
                if (auto e = reject_minus_sign_if_unsigned<T>(source); !e) {
                    return unexpected(e);
                }

                auto it = do_get_facet(facet, stream, err, source, value);
                if (auto e = check_range_supported(value, err); !e) {
                    return unexpected(e);
                }
                return {it};
            }

            template <typename T, typename CharT, typename Facet>
            scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>
            do_get_unsupported_impl(const Facet& facet,
                                    std::ios_base& stream,
                                    std::ios_base::iostate& err,
                                    std::basic_string_view<CharT> source,
                                    T& value)
            {
                if (auto e = reject_minus_sign_if_unsigned<T>(source); !e) {
                    return unexpected(e);
                }

                using scanned_type = std::conditional_t<std::is_signed_v<T>,
                                                        long long, unsigned long long>;
                scanned_type tmp{};
                auto it = do_get_facet(facet, stream, err, source, tmp);
                if (auto e = check_range_unsupported<T>(tmp, err); !e) {
                    return unexpected(e);
                }
                value = static_cast<T>(tmp);
                return {it};
            }

#define SCN_DO_GET_SUPPORTED(T)                                \
    template <typename... Args>                                \
    auto do_get(T& value, Args&&... args)                      \
    {                                                          \
        return do_get_supported_impl(SCN_FWD(args)..., value); \
    }
#define SCN_DO_GET_UNSUPPORTED(T)                                \
    template <typename... Args>                                  \
    auto do_get(T& value, Args&&... args)                        \
    {                                                            \
        return do_get_unsupported_impl(SCN_FWD(args)..., value); \
    }

            SCN_DO_GET_UNSUPPORTED(signed char)
            SCN_DO_GET_UNSUPPORTED(short)
            SCN_DO_GET_UNSUPPORTED(int)
            SCN_DO_GET_SUPPORTED(long)
            SCN_DO_GET_SUPPORTED(long long)

            SCN_DO_GET_UNSUPPORTED(unsigned char)
            SCN_DO_GET_SUPPORTED(unsigned short)
            SCN_DO_GET_SUPPORTED(unsigned int)
            SCN_DO_GET_SUPPORTED(unsigned long)
            SCN_DO_GET_SUPPORTED(unsigned long long)

#undef SCN_DO_GET_UNSUPPORTED
#undef SCN_DO_GET_SUPPORTED
        }  // namespace

        template <typename CharT>
        template <typename T>
        auto int_localized_value_reader<CharT>::read(string_view_type source,
                                                     T& value)
            -> scan_expected<ranges::iterator_t<string_view_type>>
        {
            std::basic_istringstream<CharT> s{};
            make_istream(s);

            auto stdloc = m_locale.get<std::locale>();
            const auto& facet = get_or_add_facet<
                std::num_get<CharT, ranges::iterator_t<string_view_type>>>(
                stdloc);

            std::ios_base::iostate err = std::ios_base::goodbit;
            return do_get(value, facet, s, err, source);
        }

#define SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, IntT)    \
    template auto int_localized_value_reader<CharT>::read(string_view_type, \
                                                          IntT&)            \
        ->scan_expected<ranges::iterator_t<string_view_type>>;

#define SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE(CharT)                  \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, signed char)    \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, short)          \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, int)            \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, long)           \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, long long)      \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned char)  \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned short) \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned int)   \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned long)  \
    SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL(CharT,                 \
                                                        unsigned long long)

        SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE(char)
        SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE(wchar_t)

#undef SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE
#undef SCN_DEFINE_INT_LOCALIZED_VALUE_READER_TEMPLATE_IMPL
    }  // namespace impl

    SCN_END_NAMESPACE

}  // namespace scn

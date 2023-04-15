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

#include <scn/impl/reader/float/value_reader.h>

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wold-style-cast")
SCN_GCC_IGNORE("-Wnoexcept")
SCN_GCC_IGNORE("-Wundef")
SCN_GCC_IGNORE("-Wsign-conversion")

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wold-style-cast")
SCN_CLANG_IGNORE("-Wdeprecated")
SCN_CLANG_IGNORE("-Wcomma")
SCN_CLANG_IGNORE("-Wundef")
SCN_CLANG_IGNORE("-Wdocumentation-unknown-command")

#if SCN_CLANG >= SCN_COMPILER(8, 0, 0)
SCN_CLANG_IGNORE("-Wextra-semi-stmt")
#endif

#if SCN_CLANG >= SCN_COMPILER(13, 0, 0)
SCN_CLANG_IGNORE("-Wreserved-identifier")
#endif

SCN_GCC_COMPAT_IGNORE("-Wundef")

#define FASTFLOAT_SKIP_WHITE_SPACE    0
#define FASTFLOAT_ALLOWS_LEADING_PLUS 1

#include <fast_float/fast_float.h>

SCN_CLANG_POP
SCN_GCC_POP

#include <limits>
#include <sstream>

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace {
            constexpr bool is_hexfloat(std::string_view str) SCN_NOEXCEPT
            {
                if (str.size() < 3) {
                    return false;
                }
                if (str[0] == '-') {
                    str = str.substr(1);
                }
                return str[0] == '0' && (str[1] == 'x' || str[1] == 'X');
            }

            bool is_input_inf(std::string_view str)
            {
                if (str.empty()) {
                    return false;
                }
                if (str[0] == '-') {
                    str = str.substr(1);
                }

                if (str.size() >= 3) {
                    if ((str[0] == 'i' || str[0] == 'I') &&
                        (str[1] == 'n' || str[1] == 'N') &&
                        (str[2] == 'f' || str[2] == 'F')) {
                        return true;
                    }
                }
                return false;
            }

            bool _skip_zeroes(std::string_view str,
                              std::string_view::iterator& it)
            {
                for (; it != str.end(); ++it) {
                    if (*it == '0') {
                        continue;
                    }
                    if (*it == '.' || *it == 'e' || *it == 'E' || *it == 'p' ||
                        *it == 'P') {
                        break;
                    }
                    return false;
                }
                return true;
            }

            bool is_input_hexzero(std::string_view str)
            {
                if (str[0] == '-') {
                    str = str.substr(3);
                }
                else {
                    str = str.substr(2);
                }

                auto it = str.begin();
                if (!_skip_zeroes(str, it)) {
                    return false;
                }

                if (it == str.end() || *it != '.') {
                    return true;
                }
                ++it;
                if (it == str.end()) {
                    return true;
                }
                if (!_skip_zeroes(str, it)) {
                    return false;
                }
                return true;
            }

            bool is_input_zero(std::string_view str)
            {
                if (is_hexfloat(str)) {
                    return is_input_hexzero(str);
                }

                if (str.empty()) {
                    return false;
                }
                if (str[0] == '-') {
                    str = str.substr(1);
                }

                auto it = str.begin();
                if (!_skip_zeroes(str, it)) {
                    return false;
                }
                if (it == str.end() || *it != '.') {
                    return true;
                }
                ++it;

                if (!_skip_zeroes(str, it)) {
                    return false;
                }
                return true;
            }

            SCN_GCC_COMPAT_PUSH
            SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")
            constexpr bool is_float_zero(float f)
            {
                return f == 0.0f || f == -0.0f;
            }
            constexpr bool is_float_zero(double d)
            {
                return d == 0.0 || d == -0.0;
            }
            SCN_MAYBE_UNUSED constexpr bool is_float_zero(long double ld)
            {
                return ld == 0.0L || ld == -0.0L;
            }
            SCN_GCC_COMPAT_POP

            template <typename CharT>
            struct float_preparer {
                float_preparer(std::basic_string_view<CharT> i) : input(i) {}

                void prepare(bool use_thsep, CharT thsep, CharT decimal_point)
                {
                    for (size_t i = 0; i < input.size(); ++i) {
                        if (is_ascii_space(input[i])) {
                            break;
                        }

                        if (input[i] == decimal_point) {
                            if (decimal_point_input_index != -1) {
                                break;
                            }
                            output.push_back(CharT{'.'});
                            indices.push_back(static_cast<char>(i));
                            decimal_point_input_index =
                                static_cast<std::ptrdiff_t>(i);
                            continue;
                        }

                        if (use_thsep && input[i] == thsep) {
                            if (decimal_point_input_index == -1) {
                                thsep_indices.push_back(static_cast<char>(i));
                                continue;
                            }
                            break;
                        }

                        output.push_back(input[i]);
                        indices.push_back(static_cast<char>(i));
                    }
                }

                using iterator =
                    typename std::basic_string_view<CharT>::iterator;

                iterator get_iterator(iterator output_it)
                {
                    auto sv = std::basic_string_view<CharT>{output};
                    auto diff = ranges::distance(detail::to_address(sv.begin()),
                                                 detail::to_address(output_it));
                    auto idx = indices[static_cast<size_t>(diff) - 1];
                    return input.begin() + static_cast<std::ptrdiff_t>(idx) + 1;
                }

                void transform_thsep_indices()
                {
                    SCN_EXPECT(decimal_point_input_index != -1);

                    auto last_thsep_index = decimal_point_input_index;
                    for (auto thsep_it = thsep_indices.rbegin();
                         thsep_it != thsep_indices.rend(); ++thsep_it) {
                        auto tmp = *thsep_it;
                        *thsep_it =
                            static_cast<char>(last_thsep_index - tmp - 1);
                        last_thsep_index = static_cast<std::ptrdiff_t>(tmp);
                    }
                }

                scan_error check_grouping(std::string_view grouping)
                {
                    transform_thsep_indices();

                    auto thsep_it = thsep_indices.rbegin();
                    for (auto grouping_it = grouping.begin();
                         grouping_it != grouping.end() &&
                         thsep_it != thsep_indices.rend();
                         ++grouping_it, ++thsep_it) {
                        if (*thsep_it != *grouping_it) {
                            return {scan_error::invalid_scanned_value,
                                    "Invalid thousands separator grouping"};
                        }
                    }

                    for (; thsep_it != thsep_indices.rend(); ++thsep_it) {
                        if (*thsep_it != grouping.back()) {
                            return {scan_error::invalid_scanned_value,
                                    "Invalid thousands separator grouping"};
                        }
                    }

                    return {};
                }

                scan_expected<iterator> check_grouping_and_get_iterator(
                    std::string_view grouping,
                    iterator output_it)
                {
                    if (decimal_point_input_index != -1) {
                        if (auto e = check_grouping(grouping); !e) {
                            return unexpected(e);
                        }
                    }

                    return get_iterator(output_it);
                }

                std::basic_string<CharT> output;
                std::string indices, thsep_indices;
                std::basic_string_view<CharT> input;
                std::ptrdiff_t decimal_point_input_index{-1};
            };
        }  // namespace

        ////////////////////////////////////////////////////////////////////////
        // C standard library based implementation
        // Fallback for all CharT and FloatT
        ////////////////////////////////////////////////////////////////////////

        namespace {
            struct float_classic_value_reader_cstd_impl_base {
                const float_value_reader_base& reader;
            };

            template <typename T>
            class cstd_reader_impl
                : public float_classic_value_reader_cstd_impl_base {
            public:
                explicit cstd_reader_impl(const float_value_reader_base& r)
                    : float_classic_value_reader_cstd_impl_base{r}
                {
                }

                scan_expected<ranges::iterator_t<std::string_view>> operator()(
                    std::string_view source,
                    T& value)
                {
                    clocale_restorer lr{LC_NUMERIC};
                    std::setlocale(LC_NUMERIC, "C");

                    T tmp{};
                    return impl(get_null_terminated_source(source), tmp)
                        .transform([&](size_t chars_read) SCN_NOEXCEPT {
                            value = tmp;
                            return source.begin() + chars_read;
                        })
                        .transform_error([&](scan_error err) SCN_NOEXCEPT {
                            if (err.code() == scan_error::value_out_of_range) {
                                value = tmp;
                            }
                            return err;
                        });
                }

            private:
                const char* get_null_terminated_source(std::string_view source)
                {
                    if (source.size() >= 16) {
                        auto first_space =
                            find_classic_space_narrow_fast(source);
                        ntcs_buffer.assign(source.begin(), first_space);
                    }
                    else {
                        ntcs_buffer.assign(source);
                    }
                    return ntcs_buffer.c_str();
                }

                scan_expected<std::size_t> impl(const char* src,
                                                T& tmp_value) const
                {
                    char* end{};
                    errno = 0;
                    tmp_value = cstd_strtod(src, &end);
                    const auto chars_read = end - src;
                    errno = 0;

                    if (auto e = check_error(src, chars_read, tmp_value);
                        SCN_UNLIKELY(!e)) {
                        return unexpected(e);
                    }

                    return static_cast<size_t>(chars_read);
                }

                SCN_NODISCARD scan_error check_error(std::string_view source,
                                                     std::ptrdiff_t chars_read,
                                                     T& value) const
                {
                    // No conversion
                    if (is_float_zero(value) && chars_read == 0) {
                        SCN_UNLIKELY_ATTR
                        return {scan_error::invalid_scanned_value,
                                "strtod failed: no conversion"};
                    }

                    // Unexpected hex float
                    if (is_hexfloat(source) &&
                        (reader.m_options &
                         float_value_reader_base::allow_hex) == 0) {
                        SCN_UNLIKELY_ATTR
                        return {scan_error::invalid_scanned_value,
                                "Parsed a hex float, which was "
                                "not allowed by the format string"};
                    }

                    // Musl libc doesn't set errno to ERANGE on range error,
                    // so we can't rely on that

                    // Underflow:
                    // returned 0, and input is not 0
                    if (is_float_zero(value) &&
                        !is_input_zero(source.substr(
                            0, static_cast<size_t>(chars_read)))) {
                        SCN_UNLIKELY_ATTR
                        return {scan_error::value_out_of_range,
                                "strtod failed: float underflow"};
                    }

                    // Overflow:
                    // returned inf (HUGE_VALUE), and input is not "inf"
                    if (std::isinf(value) &&
                        !is_input_inf(source.substr(
                            0, static_cast<size_t>(chars_read)))) {
                        SCN_UNLIKELY_ATTR
                        return {scan_error::value_out_of_range,
                                "strtod failed: float overflow"};
                    }

                    return {};
                }

                T cstd_strtod(const char* str, char** str_end) const
                {
                    if constexpr (std::is_same_v<T, float>) {
                        return std::strtof(str, str_end);
                    }
                    else if constexpr (std::is_same_v<T, double>) {
                        return std::strtod(str, str_end);
                    }
                    else if constexpr (std::is_same_v<T, long double>) {
                        return std::strtold(str, str_end);
                    }
                }

                std::string ntcs_buffer{};
            };
        }  // namespace

        ////////////////////////////////////////////////////////////////////////
        // std::from_chars based implementation
        // Only for CharT=char
        ////////////////////////////////////////////////////////////////////////

#if SCN_HAS_FLOAT_CHARCONV
        namespace {
            template <typename = void>
            struct float_classic_value_reader_from_chars_impl_base {
                scan_expected<std::chars_format> get_flags(
                    std::string_view& source,
                    bool& has_negative_sign) const
                {
                    if ((reader.m_options &
                         float_value_reader_base::allow_hex) != 0 &&
                        is_hexfloat(source)) {
                        if (source[0] == '-') {
                            has_negative_sign = true;
                            source = source.substr(3);
                        }
                        else if (source[0] == '+') {
                            source = source.substr(3);
                        }
                        else {
                            source = source.substr(2);
                        }
                        return {std::chars_format::hex};
                    }

                    std::chars_format flags{};
                    if ((reader.m_options &
                         float_value_reader_base::allow_fixed) != 0) {
                        flags |= std::chars_format::fixed;
                    }
                    if ((reader.m_options &
                         float_value_reader_base::allow_scientific) != 0) {
                        flags |= std::chars_format::scientific;
                    }

                    if (SCN_UNLIKELY(flags == std::chars_format{})) {
                        SCN_EXPECT((reader.m_options &
                                    float_value_reader_base::allow_hex) != 0);
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "from_chars failed: Expected a hexfloat");
                    }

                    return {flags};
                }

                const float_value_reader_base& reader;
            };

            template <typename T>
            class from_chars_reader_impl
                : public float_classic_value_reader_from_chars_impl_base<> {
            public:
                explicit from_chars_reader_impl(
                    const float_value_reader_base& r)
                    : float_classic_value_reader_from_chars_impl_base{r}
                {
                }

                scan_expected<ranges::iterator_t<std::string_view>> operator()(
                    std::string_view source,
                    T& value) const
                {
                    const auto original_source = source;
                    bool has_negative_sign = false;
                    const auto flags = get_flags(source, has_negative_sign);
                    if (SCN_UNLIKELY(!flags)) {
                        return unexpected(flags.error());
                    }

                    T tmp{};
                    const auto result = std::from_chars(
                        source.data(), source.data() + source.size(), tmp,
                        *flags);

                    if (SCN_UNLIKELY(result.ec ==
                                     std::errc::invalid_argument)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "from_chars failed: invalid_argument");
                    }
                    if (result.ec == std::errc::result_out_of_range) {
                        // Out of range, may be subnormal -> fall back to strtod
                        // On gcc, std::from_chars doesn't parse subnormals
                        return cstd_reader_impl<T>{reader}(original_source,
                                                           value);
                    }

                    if (has_negative_sign) {
                        value = -tmp;
                    }
                    else {
                        value = tmp;
                    }
                    return {source.begin() + (result.ptr - source.data())};
                }
            };
        }  // namespace
#endif

        ////////////////////////////////////////////////////////////////////////
        // fast_float based implementation
        // Only for CharT=char AND FloatT=(float OR double)
        ////////////////////////////////////////////////////////////////////////

        namespace {
            struct float_classic_value_reader_fast_float_impl_base {
                fast_float::chars_format get_flags() const
                {
                    unsigned format_flags{};
                    if ((reader.m_options &
                         float_value_reader_base::allow_fixed) != 0) {
                        format_flags |= fast_float::fixed;
                    }
                    if ((reader.m_options &
                         float_value_reader_base::allow_scientific) != 0) {
                        format_flags |= fast_float::scientific;
                    }

                    return static_cast<fast_float::chars_format>(format_flags);
                }

                const float_value_reader_base& reader;
            };

#if SCN_HAS_FLOAT_CHARCONV
            template <typename Float, typename = void>
            struct has_charconv_for : std::false_type {};

            template <typename Float>
            struct has_charconv_for<
                Float,
                std::void_t<decltype(std::from_chars(SCN_DECLVAL(const char*),
                                                     SCN_DECLVAL(const char*),
                                                     SCN_DECLVAL(Float&)))>>
                : std::true_type {};
#endif

            template <typename T>
            auto fast_float_fallback(const float_value_reader_base& reader,
                                     std::string_view source,
                                     T& value)
            {
#if SCN_HAS_FLOAT_CHARCONV
                constexpr bool cond =
#if SCN_STDLIB_GLIBCXX
                    !std::is_same_v<T, long double> &&
#endif
                    has_charconv_for<T>::value;

                if constexpr (cond) {
                    return from_chars_reader_impl<T>{reader}(source, value);
                }
                else {
                    return cstd_reader_impl<T>{reader}(source, value);
                }
#else
                return cstd_reader_impl<T>{reader}(source, value);
#endif
            }

            template <typename T>
            class fast_float_reader_impl
                : float_classic_value_reader_fast_float_impl_base {
            public:
                explicit fast_float_reader_impl(
                    const float_value_reader_base& r)
                    : float_classic_value_reader_fast_float_impl_base{r}
                {
                }

                scan_expected<ranges::iterator_t<std::string_view>> operator()(
                    std::string_view source,
                    T& value) const
                {
                    if ((reader.m_options &
                         float_value_reader_base::allow_hex) != 0) {
                        if (is_hexfloat(source)) {
                            // fast_float doesn't support hexfloats
                            return fast_float_fallback(reader, source, value);
                        }
                        if ((reader.m_options ^
                             float_value_reader_base::allow_hex) == 0) {
                            // only hex floats allowed
                            SCN_UNLIKELY_ATTR
                            return unexpected_scan_error(
                                scan_error::invalid_scanned_value,
                                "float parsing failed: expected hexfloat");
                        }
                    }

                    const auto flags = get_flags();
                    T tmp{};
                    const auto result = fast_float::from_chars(
                        source.data(), source.data() + source.size(), tmp,
                        flags);

                    if (SCN_UNLIKELY(result.ec ==
                                     std::errc::invalid_argument)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "fast_float failed: invalid_argument");
                    }
                    if (SCN_UNLIKELY(result.ec ==
                                     std::errc::result_out_of_range)) {
                        value = tmp;
                        return unexpected_scan_error(
                            scan_error::value_out_of_range,
                            "fast_float failed: result_out_of_range");
                    }

                    value = tmp;
                    return {source.begin() + (result.ptr - source.data())};
                }
            };
        }  // namespace

        // classic reader dispatch

        namespace {
            template <typename T>
            auto do_read_without_thsep(const float_value_reader_base& reader,
                                       std::string_view source,
                                       T& value)
                -> scan_expected<ranges::iterator_t<std::string_view>>
            {
                if constexpr (std::is_same_v<T, long double>) {
                    if constexpr (sizeof(double) == sizeof(long double)) {
                        // If long double is an alias to double (true on
                        // Windows), use fast_float with double
                        double tmp{};
                        auto ret =
                            fast_float_reader_impl<double>{reader}(source, tmp);
                        value = tmp;
                        return ret;
                    }
                    else {
                        // long doubles aren't supported by fast_float ->
                        // fall back to from_chars or cstd_impl
                        return fast_float_fallback(reader, source, value);
                    }
                }
                else {
                    // Default to fast_float
                    return fast_float_reader_impl<T>{reader}(source, value);
                }
            }

            template <typename T>
            auto do_read_with_thsep(const float_value_reader_base& reader,
                                    std::string_view source,
                                    T& value)
                -> scan_expected<ranges::iterator_t<std::string_view>>
            {
                float_preparer<char> prepare{source};
                prepare.prepare(true, ',', '.');

                auto reader_input = std::string_view{prepare.output};
                return do_read_without_thsep(reader, reader_input, value)
                    .and_then([&](auto it) {
                        return prepare.check_grouping_and_get_iterator("\3",
                                                                       it);
                    });
            }

            template <typename T>
            auto do_read(const float_value_reader_base& reader,
                         std::string_view source,
                         T& value)
                -> scan_expected<ranges::iterator_t<std::string_view>>
            {
                if ((reader.m_options & float_value_reader_base::allow_thsep) !=
                    0) {
                    return do_read_with_thsep(reader, source, value);
                }
                return do_read_without_thsep(reader, source, value);
            }

            scan_expected<std::string_view> make_utf8_string(
                std::wstring_view source,
                span<char> buffer)
            {
                return validate_and_count_transcoded_code_units<char>(source)
                    .transform([source, &buffer](std::size_t utf8_cu_count) {
                        buffer = buffer.first(utf8_cu_count);
                        const auto n = transcode_valid(source, buffer);
                        SCN_ENSURE(n == buffer.size());

                        return std::string_view{buffer.data(), buffer.size()};
                    });
            }

            ranges::iterator_t<std::wstring_view> get_corresponding_iterator(
                std::wstring_view wide_input,
                std::string_view utf8_input,
                std::size_t utf8_count)
            {
                auto n = count_valid_transcoded_code_units<wchar_t>(
                    std::string_view{utf8_input.data(), utf8_count});
                return wide_input.begin() + n;
            }

            template <typename T>
            auto do_read(const float_value_reader_base& reader,
                         std::wstring_view source,
                         T& value)
                -> scan_expected<ranges::iterator_t<std::wstring_view>>
            {
                // With wide strings, transcode to utf8, and use narrow impl

                auto limited_source = source.substr(0, 64);
                if constexpr (sizeof(wchar_t) == 2) {
                    while (utf16_code_point_length_by_starting_code_unit(
                               limited_source.back()) == 0) {
                        limited_source =
                            limited_source.substr(0, limited_source.size() - 1);
                    }
                }

                std::array<char, 256> buffer{};
                auto utf8_input = make_utf8_string(
                    limited_source, {buffer.data(), buffer.size()});
                if (SCN_UNLIKELY(!utf8_input)) {
                    return unexpected(utf8_input.error());
                }

                return do_read(reader, *utf8_input, value)
                    .transform([&source, utf8_input](auto it) {
                        return get_corresponding_iterator(
                            source, *utf8_input,
                            static_cast<std::size_t>(ranges::distance(
                                detail::to_address(utf8_input->begin()),
                                detail::to_address(it))));
                    });
            }
        }  // namespace

        template <typename CharT>
        template <typename T>
        auto float_classic_value_reader<CharT>::read(string_view_type source,
                                                     T& value)
            -> scan_expected<ranges::iterator_t<string_view_type>>
        {
            return do_read(*this, source, value);
        }

        template auto float_classic_value_reader<char>::read(string_view_type,
                                                             float&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_classic_value_reader<char>::read(string_view_type,
                                                             double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_classic_value_reader<char>::read(string_view_type,
                                                             long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;

        template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            float&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            double&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;

        ////////////////////////////////////////////////////////////////////////
        // localized implementation:
        //   change localized characters, delegate to classic
        ////////////////////////////////////////////////////////////////////////

        template <typename CharT>
        template <typename T>
        auto float_localized_value_reader<CharT>::read(string_view_type source,
                                                       T& value)
            -> scan_expected<ranges::iterator_t<string_view_type>>
        {
            auto stdloc = m_locale.get<std::locale>();
            const auto& numpunct =
                get_or_add_facet<std::numpunct<CharT>>(stdloc);

            const auto grouping = numpunct.grouping();
            const bool use_thsep =
                !grouping.empty() && (m_options & allow_thsep) != 0;
            const CharT decimal_point = numpunct.decimal_point();
            const CharT thsep = numpunct.thousands_sep();

            auto prepare = float_preparer<CharT>{source};
            prepare.prepare(use_thsep, thsep, decimal_point);

            auto reader = float_classic_value_reader<CharT>{
                static_cast<uint8_t>(m_options & ~allow_thsep)};
            auto reader_input = std::basic_string_view<CharT>{prepare.output};
            SCN_UNUSED(reader_input);

            return reader.read(reader_input, value).and_then([&](auto it) {
                return prepare.check_grouping_and_get_iterator(grouping, it);
            });
        }

        template auto float_localized_value_reader<char>::read(string_view_type,
                                                               float&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_localized_value_reader<char>::read(string_view_type,
                                                               double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_localized_value_reader<char>::read(string_view_type,
                                                               long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;

        template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            float&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            double&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn

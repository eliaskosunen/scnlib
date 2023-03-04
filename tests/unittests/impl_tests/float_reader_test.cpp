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

#include "../test_common.h"

#include <scn/detail/istream_range.h>
#include <scn/detail/locale_ref.h>
#include <scn/impl/reader/float/reader.h>
#include <scn/util/optional.h>

#include <cmath>

template <typename CharT, typename FloatT>
class float_reader_interface : public value_reader_interface<CharT> {
public:
    virtual scn::scan_expected<typename std::basic_string_view<CharT>::iterator>
    read(std::basic_string_view<CharT> source, FloatT& value) = 0;
};

template <typename CharT, typename FloatT>
class classic_reader_interface : public float_reader_interface<CharT, FloatT> {
public:
    classic_reader_interface() = default;

    void make_value_reader() override
    {
        m_reader =
            std::make_unique<scn::impl::float_classic_value_reader<CharT>>();
    }
    void make_value_reader(uint8_t flags, uint8_t = 0) override
    {
        m_reader =
            std::make_unique<scn::impl::float_classic_value_reader<CharT>>(
                flags);
    }
    void make_value_reader_from_specs(
        const scn::detail::basic_format_specs<CharT>& specs) override
    {
        auto reader = scn::impl::float_classic_value_reader<CharT>{
            scn::impl::float_reader<FloatT, CharT>::get_presentation_flags(
                specs)};
        m_reader =
            std::make_unique<scn::impl::float_classic_value_reader<CharT>>(
                reader);
    }
    scn::scan_expected<typename std::basic_string_view<CharT>::iterator> read(
        std::basic_string_view<CharT> source,
        FloatT& value) override
    {
        SCN_EXPECT(m_reader);
        return m_reader->read(source, value);
    }

    [[nodiscard]] bool is_localized() const override
    {
        return false;
    }

private:
    std::unique_ptr<scn::impl::float_classic_value_reader<CharT>> m_reader;
};

template <typename CharT, typename FloatT>
class localized_reader_interface
    : public float_reader_interface<CharT, FloatT> {
public:
    localized_reader_interface() = default;

    void make_value_reader() override
    {
        m_reader =
            std::make_unique<scn::impl::float_localized_value_reader<CharT>>(
                scn::detail::locale_ref{});
    }
    void make_value_reader(uint8_t flags, uint8_t = 0) override
    {
        m_reader =
            std::make_unique<scn::impl::float_localized_value_reader<CharT>>(
                flags, scn::detail::locale_ref{});
    }
    void make_value_reader_from_specs(
        const scn::detail::basic_format_specs<CharT>& specs) override
    {
        auto reader = scn::impl::float_localized_value_reader<CharT>{
            scn::impl::float_reader<FloatT, CharT>::get_presentation_flags(
                specs),
            scn::detail::locale_ref{}};
        m_reader =
            std::make_unique<scn::impl::float_localized_value_reader<CharT>>(
                reader);
    }
    scn::scan_expected<typename std::basic_string_view<CharT>::iterator> read(
        std::basic_string_view<CharT> source,
        FloatT& value) override
    {
        SCN_EXPECT(m_reader);
        return m_reader->read(source, value);
    }

    [[nodiscard]] bool is_localized() const override
    {
        return true;
    }

private:
    std::unique_ptr<scn::impl::float_localized_value_reader<CharT>> m_reader;
};

template <template <class, class> class Interface,
          typename CharT,
          typename FloatT>
struct test_type_pack {
    using interface_type = Interface<CharT, FloatT>;
    using char_type = CharT;
    using float_type = FloatT;
};

template <typename T>
[[nodiscard]] testing::AssertionResult check_floating_eq(T a, T b)
{
    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")
    if (a == b) {
        return testing::AssertionSuccess();
    }
    SCN_GCC_COMPAT_POP
    return testing::AssertionFailure()
           << "Floats not equal: " << a << " and " << b;
}

using namespace std::string_view_literals;

template <typename T>
class FloatValueReaderTest : public testing::Test {
protected:
    using interface_type = typename T::interface_type;
    using char_type = typename T::char_type;
    using float_type = typename T::float_type;
    using string_type = std::basic_string<char_type>;
    using string_view_type = std::basic_string_view<char_type>;

    static constexpr bool is_wide()
    {
        return std::is_same_v<char_type, wchar_t>;
    }
    static constexpr bool is_f32()
    {
        return sizeof(float_type) == sizeof(float);
    }
    static constexpr bool is_f64()
    {
        // double on Linux, double and long double on Windows
        return sizeof(float_type) == sizeof(double);
    }
    static constexpr bool is_double()
    {
        return std::is_same_v<float_type, double>;
    }
    static constexpr bool is_long_double()
    {
        return std::is_same_v<float_type, long double>;
    }
    static constexpr bool is_double_64()
    {
        return is_f64() && is_double();
    }
    static constexpr bool is_long_double_64()
    {
        return is_f64() && is_long_double();
    }
    static constexpr bool is_f80()
    {
        // long double on Linux
        return std::numeric_limits<float_type>::digits == 64;
    }
    static constexpr bool is_f128()
    {
        return !is_f80() && sizeof(float_type) == 16;
    }

    static_assert(std::numeric_limits<float_type>::is_iec559);
    static_assert(is_f32() || is_double() || is_long_double());
    static_assert(is_f32() || is_f64() || is_f80() || is_f128());
    static_assert(is_f32() || is_double_64() || is_long_double_64() ||
                  is_f80() || is_f128());

    static constexpr const char* get_length_flag()
    {
        if constexpr (std::is_same_v<float_type, long double>) {
            return "L";
        }
        else {
            return "";
        }
    }

    static std::string format_float(float_type val,
                                    const std::string& flags_before_len,
                                    const std::string& flags_after_len)
    {
        std::string buf(256, '\0');
        auto casted_val = [&]() {
            if constexpr (std::is_same_v<float_type, float>) {
                return static_cast<double>(val);
            }
            else {
                return val;
            }
        };
        SCN_GCC_COMPAT_PUSH
        SCN_GCC_COMPAT_IGNORE("-Wformat-nonliteral")
        const auto fmt = std::string{"%"} + flags_before_len +
                         get_length_flag() + flags_after_len;
        const auto ret =
            std::snprintf(buf.data(), buf.size(), fmt.c_str(), casted_val());
        SCN_GCC_COMPAT_POP
        SCN_EXPECT(ret > 0);
        buf = buf.substr(0, buf.find('\0'));
        return buf;
    }

    static auto get_pi()
    {
        if constexpr (is_f32()) {
            return std::make_pair(3.14f, "3.14"sv);
        }
        else if constexpr (is_double()) {
            return std::make_pair(3.14, "3.14"sv);
        }
        else if constexpr (is_long_double()) {
            return std::make_pair(3.14l, "3.14"sv);
        }
    }
    static auto get_neg()
    {
        if constexpr (is_f32()) {
            return std::make_pair(-123.456f, "-123.456"sv);
        }
        else if constexpr (is_double()) {
            return std::make_pair(-123.456, "-123.456"sv);
        }
        else if constexpr (is_long_double()) {
            return std::make_pair(-123.456l, "-123.456"sv);
        }
    }

#ifdef __x86_64__
#define SCN_IS_X86 1
#elif defined(_M_X64)
#define SCN_IS_X86 1
#elif defined(i386)
#define SCN_IS_X86 1
#elif defined(__i386)
#define SCN_IS_X86 1
#elif defined(__i386__)
#define SCN_IS_X86 1
#elif defined(_M_IX86)
#define SCN_IS_X86 1
#else
#define SCN_IS_X86 0
#endif

    static auto get_subnormal()
    {
        if constexpr (is_f32()) {
            return std::make_pair(2e-40f, "2e-40"sv);
        }
        else if constexpr (is_double_64()) {
            return std::make_pair(5e-320, "5e-320"sv);
        }
        else if constexpr (is_long_double_64()) {
            return std::make_pair(5e-320l, "5e-320"sv);
        }
        else if constexpr (is_f80()) {
            return std::make_pair(3e-4940l, "3e-4940"sv);
        }
        // GCC warns about out-of-range literal here on x86.
        // The value is out of range for f80, but not for f128.
        // This branch is not taken on x86, where long double is never f128.
        //
        // For some reason, -Woverflow can't also be ignored
#if !(SCN_GCC && SCN_IS_X86)
        else if constexpr (is_f128()) {
            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wliteral-range")

            return std::make_pair(5e-4960l, "5e-4960"sv);

            SCN_CLANG_POP
        }
#endif
    }
    static auto get_subnormal_hex()
    {
        if constexpr (is_f32()) {
            return std::make_pair(0x1.2p-130f, "0x1.2p-130"sv);
        }
        else if constexpr (is_double_64()) {
            return std::make_pair(0x1.2p-1050, "0x1.2p-1050"sv);
        }
        else if constexpr (is_long_double_64()) {
            return std::make_pair(0x1.2p-1050l, "0x1.2p-1050"sv);
        }
        else if constexpr (is_f80()) {
            return std::make_pair(0x1.2p-16400l, "0x1.2p-16400"sv);
        }
#if !(SCN_GCC && SCN_IS_X86)
        else if constexpr (is_f128()) {
            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wliteral-range")

            return std::make_pair(0x1.2p-16450l, "0x1.2p-16450"sv);

            SCN_CLANG_POP
        }
#endif
    }

    static auto get_subnormal_max()
    {
        if constexpr (is_f32()) {
            return std::make_pair(1e-38f, "1e-38"sv);
        }
        else if constexpr (is_double_64()) {
            return std::make_pair(2e-308, "2e-308"sv);
        }
        else if constexpr (is_long_double_64()) {
            return std::make_pair(2e-308l, "2e-308"sv);
        }
        else if constexpr (is_f80() || is_f128()) {
            return std::make_pair(3.2e-4932l, "3.2e-4932"sv);
        }
    }
    static auto get_subnormal_max_hex()
    {
        if constexpr (is_f32()) {
            return std::make_pair(0x1.fp-127f, "0x1.fp-127"sv);
        }
        else if constexpr (is_double_64()) {
            return std::make_pair(0x1.fp-1023, "0x1.fp-1023"sv);
        }
        else if constexpr (is_long_double_64()) {
            return std::make_pair(0x1.fp-1023l, "0x1.fp-1023"sv);
        }
        else if constexpr (is_f80() || is_f128()) {
            return std::make_pair(0x1.fp-16383l, "0x1.fp-16383"sv);
        }
    }

    static auto get_normal_min()
    {
        auto val = std::numeric_limits<float_type>::min();
        return std::make_pair(val, format_float(val, ".24", "e"));
    }
    static auto get_normal_min_hex()
    {
        auto val = std::numeric_limits<float_type>::min();
        return std::make_pair(val, format_float(val, "", "a"));
    }

    static auto get_underflow()
    {
        if constexpr (is_f32()) {
            return "1.0e-45"sv;
        }
        else if constexpr (is_f64()) {
            return "4.0e-324"sv;
        }
        else if constexpr (is_f80()) {
            return "3.0e-4951"sv;
        }
        else if constexpr (is_f128()) {
            return "6.0e-4966"sv;
        }
    }
    static auto get_underflow_hex()
    {
        if constexpr (is_f32()) {
            return "0x1.fffffep-150"sv;
        }
        else if constexpr (is_f64()) {
            return "0x1.fffffffffffffp-1075"sv;
        }
        else if constexpr (is_f80()) {
            return "0x1.fffffffffffffffep-16447"sv;
        }
        else if constexpr (is_f128()) {
            return "0x1.fffffffffffffffffffep-16497"sv;
        }
    }

    static auto get_maximum()
    {
        auto val = std::numeric_limits<float_type>::max();
        return std::make_pair(val, format_float(val, ".24", "e"));
    }
    static auto get_maximum_hex()
    {
        auto val = std::numeric_limits<float_type>::max();
        return std::make_pair(val, format_float(val, ".16", "a"));
    }

    static auto get_overflow()
    {
        if constexpr (is_f32()) {
            return "4.0e38"sv;
        }
        else if constexpr (is_f64()) {
            return "2.0e308"sv;
        }
        else if constexpr (is_f80() || is_f128()) {
            return "2.0e4932"sv;
        }
    }
    static auto get_overflow_hex()
    {
        if constexpr (is_f32()) {
            return "0x1p+128"sv;
        }
        else if constexpr (is_f64()) {
            return "0x1p+1024"sv;
        }
        else if constexpr (is_f80() || is_f128()) {
            return "0x1p+16384"sv;
        }
    }

    template <typename Source>
    void set_source(Source&& s)
    {
        if constexpr (!is_wide()) {
            widened_source = std::string{SCN_FWD(s)};
        }
        else if constexpr (std::is_same_v<
                               scn::detail::remove_cvref_t<
                                   scn::ranges::range_value_t<Source>>,
                               wchar_t>) {
            widened_source = std::wstring{SCN_FWD(s)};
        }
        else {
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")
            auto sv = std::string_view{s};
            widened_source = std::wstring(sv.size(), L'\0');
            std::copy(sv.begin(), sv.end(), widened_source->begin());
            SCN_GCC_POP
        }
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_generic_success(
        const Result& result) const
    {
        if (!result) {
            return testing::AssertionFailure()
                   << "Result not good: code " << result.error().code();
        }
        SCN_EXPECT(this->widened_source);
        if (scn::detail::to_address(result.value()) !=
            scn::detail::to_address(this->widened_source->end())) {
            return testing::AssertionFailure()
                   << "Result range not correct: diff "
                   << std::distance(
                          scn::detail::to_address(result.value()),
                          scn::detail::to_address(this->widened_source->end()));
        }
        return testing::AssertionSuccess();
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_value_success(
        const Result& result,
        float_type val,
        float_type expected) const
    {
        if (auto a = check_generic_success(result); !a) {
            return a;
        }
        if (auto a = check_floating_eq(val, expected); !a) {
            return a;
        }
        return testing::AssertionSuccess();
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_failure_with_code(
        const Result& result,
        float_type val,
        enum scn::scan_error::code c) const
    {
        if (result) {
            return testing::AssertionFailure()
                   << "Result good, expected failure";
        }
        if (result.error().code() != c) {
            return testing::AssertionFailure()
                   << "Result failed with wrong error code: "
                   << result.error().code() << ", expected " << c;
        }
        if (auto a = check_floating_eq(val, static_cast<float_type>(0.0)); !a) {
            return a;
        }
        return testing::AssertionSuccess();
    }

    template <typename Source, typename... ReaderArgs>
    auto simple_test(Source&& source, ReaderArgs&&... args)
    {
        this->set_source(SCN_FWD(source));
        this->interface.make_value_reader(SCN_FWD(args)...);

        float_type val{};
        auto result = this->interface.read(widened_source.value(), val);
        return std::make_pair(result, val);
    }
    template <typename Source>
    auto simple_specs_test(
        Source&& source,
        const scn::detail::basic_format_specs<char_type>& specs)
    {
        this->set_source(SCN_FWD(source));
        this->interface.make_value_reader_from_specs(specs);

        float_type val{};
        auto result = this->interface.read(widened_source.value(), val);
        return std::make_pair(result, val);
    }

    template <typename Source, typename... ReaderArgs>
    auto simple_success_test(Source&& source, ReaderArgs&&... args)
    {
        this->set_source(SCN_FWD(source));
        this->interface.make_value_reader(SCN_FWD(args)...);

        float_type val{};
        auto result = this->interface.read(widened_source.value(), val);
        return std::make_tuple(this->check_generic_success(result), result,
                               val);
    }
    template <typename Source>
    auto simple_success_specs_test(
        Source&& source,
        const scn::detail::basic_format_specs<char_type>& specs)
    {
        this->set_source(SCN_FWD(source));
        this->interface.make_value_reader_from_specs(specs);

        float_type val{};
        auto result = this->interface.read(widened_source.value(), val);
        return std::make_tuple(this->check_generic_success(result), result,
                               val);
    }

    template <typename Source>
    [[nodiscard]] testing::AssertionResult simple_default_test(
        Source&& source,
        float_type expected_output)
    {
        auto [result, val] = simple_test(SCN_FWD(source));
        return check_value_success(result, val, expected_output);
    }

    scn::detail::basic_format_specs<char_type>
    make_format_specs_with_presentation(
        scn::detail::presentation_type type) const
    {
        scn::detail::basic_format_specs<char_type> specs{};
        specs.type = type;
        return specs;
    }

    interface_type interface;

    scn::optional<string_type> widened_source;
};

using type_list = ::testing::Types<
    test_type_pack<classic_reader_interface, char, float>,
    test_type_pack<classic_reader_interface, char, double>,
    test_type_pack<classic_reader_interface, char, long double>,
    test_type_pack<classic_reader_interface, wchar_t, float>,
    test_type_pack<classic_reader_interface, wchar_t, double>,
    test_type_pack<classic_reader_interface, wchar_t, long double>,
    test_type_pack<localized_reader_interface, char, float>,
    test_type_pack<localized_reader_interface, char, double>,
    test_type_pack<localized_reader_interface, char, long double>,
    test_type_pack<localized_reader_interface, wchar_t, float>,
    test_type_pack<localized_reader_interface, wchar_t, double>,
    test_type_pack<localized_reader_interface, wchar_t, long double>>;

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

TYPED_TEST_SUITE(FloatValueReaderTest, type_list);

SCN_CLANG_POP

TYPED_TEST(FloatValueReaderTest, Basic)
{
    const auto [val, src] = this->get_pi();
    EXPECT_TRUE(this->simple_default_test(src, val));
}

TYPED_TEST(FloatValueReaderTest, Negative)
{
    const auto [val, src] = this->get_neg();
    EXPECT_TRUE(this->simple_default_test(src, val));
}

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wdouble-promotion")

TYPED_TEST(FloatValueReaderTest, Scientific)
{
    EXPECT_TRUE(this->simple_default_test("4.20e1", 42.0));
}

TYPED_TEST(FloatValueReaderTest, Hex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    EXPECT_TRUE(this->simple_default_test("0x1.2ap3", 0x1.2ap3));
}
TYPED_TEST(FloatValueReaderTest, NegativeHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    EXPECT_TRUE(this->simple_default_test("-0x1.2ap3", -0x1.2ap3));
}

SCN_CLANG_POP  // -Wdouble-promotion

TYPED_TEST(FloatValueReaderTest, InfinityWithInf)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't support infinities";
    }

    auto [a, _, val] = this->simple_success_test("inf");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isinf(val));
    EXPECT_FALSE(std::signbit(val));
}
TYPED_TEST(FloatValueReaderTest, InfinityWithNegInfinity)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't support infinities";
    }

    auto [a, _, val] = this->simple_success_test("-infinity");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isinf(val));
    EXPECT_TRUE(std::signbit(val));
}

TYPED_TEST(FloatValueReaderTest, NaN)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't support NaNs";
    }

    auto [a, _, val] = this->simple_success_test("nan");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnan(val));
    EXPECT_FALSE(std::signbit(val));
}
TYPED_TEST(FloatValueReaderTest, NaNWithPayload)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't support NaNs";
    }

    auto [a, _, val] = this->simple_success_test("nan(123_abc)");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnan(val));
    EXPECT_FALSE(std::signbit(val));
}

TYPED_TEST(FloatValueReaderTest, Overflow)
{
    auto [result, val] =
        this->simple_test("9999999999999.9999e999999999999999");
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::value_out_of_range));
}

TYPED_TEST(FloatValueReaderTest, Subnormal)
{
    const auto [orig_val, source] = this->get_subnormal();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, SubnormalFromHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    const auto [orig_val, source] = this->get_subnormal();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, LargeSubnormal)
{
    const auto [orig_val, source] = this->get_subnormal_max();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, LargeSubnormalFromHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    const auto [orig_val, source] = this->get_subnormal_max_hex();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, MinimumNormal)
{
    const auto [orig_val, source] = this->get_normal_min();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, MinimumNormalFromHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    const auto [orig_val, source] = this->get_normal_min_hex();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, BarelyUnderflow)
{
    auto [a, _, val] = this->simple_success_test(this->get_underflow());
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(
        val,
        std::numeric_limits<typename TestFixture::float_type>::denorm_min()));
}
TYPED_TEST(FloatValueReaderTest, BarelyUnderflowFromHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    auto [a, _, val] = this->simple_success_test(this->get_underflow_hex());
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(
        val,
        std::numeric_limits<typename TestFixture::float_type>::denorm_min()));
}

TYPED_TEST(FloatValueReaderTest, Maximum)
{
    const auto [orig_val, source] = this->get_maximum();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isinf(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, MaximumFromHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    const auto [orig_val, source] = this->get_maximum_hex();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isinf(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, BarelyOverflow)
{
    auto [result, val] = this->simple_test(this->get_overflow());
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::value_out_of_range));
}
TYPED_TEST(FloatValueReaderTest, BarelyOverflowFromHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    auto [result, val] = this->simple_test(this->get_overflow_hex());
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::value_out_of_range));
}

TYPED_TEST(FloatValueReaderTest, PresentationScientificValueScientific)
{
    auto [a, _, val] = this->simple_success_specs_test(
        "12.3e4", this->make_format_specs_with_presentation(
                      scn::detail::presentation_type::float_scientific));
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(12.3e4)));
}
TYPED_TEST(FloatValueReaderTest, PresentationScientificValueFixed)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't specifying a float format";
    }
    if constexpr (std::is_same_v<typename TestFixture::float_type,
                                 long double>) {
        // FIXME
        return SUCCEED() << "This test is buggy with long doubles";
    }

    auto [result, val] = this->simple_specs_test(
        "12.3", this->make_format_specs_with_presentation(
                    scn::detail::presentation_type::float_scientific));
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::invalid_scanned_value));
}
TYPED_TEST(FloatValueReaderTest, PresentationScientificValueHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't specifying a float format";
    }

    auto [result, val] = this->simple_specs_test(
        "0x1.fp3", this->make_format_specs_with_presentation(
                       scn::detail::presentation_type::float_scientific));
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::invalid_scanned_value));
}

TYPED_TEST(FloatValueReaderTest, PresentationFixedValueScientific)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't specifying a float format";
    }
    if constexpr (std::is_same_v<typename TestFixture::float_type,
                                 long double>) {
        // FIXME
        return SUCCEED() << "This test is buggy with long doubles";
    }

    auto [result, val] = this->simple_specs_test(
        "12.3e4", this->make_format_specs_with_presentation(
                      scn::detail::presentation_type::float_fixed));
    ASSERT_TRUE(result);
    EXPECT_EQ(scn::detail::to_address(result.value()),
              this->widened_source->data() + 4);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(12.3l)));
}
TYPED_TEST(FloatValueReaderTest, PresentationFixedValueFixed)
{
    const auto [orig_val, src] = this->get_pi();
    auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation(
                 scn::detail::presentation_type::float_fixed));

    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, PresentationFixedValueHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't specifying a float format";
    }
    if constexpr (std::is_same_v<typename TestFixture::float_type,
                                 long double>) {
        // FIXME
        return SUCCEED() << "This test is buggy with long doubles";
    }

    auto [result, val] = this->simple_specs_test(
        "0x1.fp3", this->make_format_specs_with_presentation(
                       scn::detail::presentation_type::float_fixed));
    ASSERT_TRUE(result);
    EXPECT_EQ(scn::detail::to_address(result.value()),
              this->widened_source->data() + 1);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(0.0)));
}

TYPED_TEST(FloatValueReaderTest, PresentationHexValueScientific)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't specifying a float format";
    }
    if constexpr (std::is_same_v<typename TestFixture::float_type,
                                 long double>) {
        // FIXME
        return SUCCEED() << "This test is buggy with long doubles";
    }

    auto [result, val] = this->simple_specs_test(
        "12.3e4", this->make_format_specs_with_presentation(
                      scn::detail::presentation_type::float_hex));
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::invalid_scanned_value));
}
TYPED_TEST(FloatValueReaderTest, PresentationHexValueFixed)
{
    if (this->interface.is_localized()) {
        return SUCCEED() << "std::num_get doesn't specifying a float format";
    }
    if constexpr (std::is_same_v<typename TestFixture::float_type,
                                 long double>) {
        // FIXME
        return SUCCEED() << "This test is buggy with long doubles";
    }

    auto [result, val] = this->simple_specs_test(
        "12.3", this->make_format_specs_with_presentation(
                    scn::detail::presentation_type::float_hex));
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::invalid_scanned_value));
}
TYPED_TEST(FloatValueReaderTest, PresentationHexValueHex)
{
    if (this->interface.is_localized()) {
        return SUCCEED()
               << "std::num_get doesn't universally support hexfloats";
    }

    auto [a, _, val] = this->simple_success_specs_test(
        "0x1.fp3", this->make_format_specs_with_presentation(
                       scn::detail::presentation_type::float_hex));
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(0x1.fp3)));
}

// Copyright 2017-2019 Elias Kosunen
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

#ifndef SCN_DETAIL_UTIL_H
#define SCN_DETAIL_UTIL_H

#include "config.h"

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename... Ts>
        struct make_void {
            using type = void;
        };
        template <typename... Ts>
        using void_t = typename make_void<Ts...>::type;

        template <typename... T>
        void valid_expr(T&&...);

        template <typename T>
        struct remove_cvref {
            using type = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;
        };
        template <typename T>
        using remove_cvref_t = typename remove_cvref<T>::type;

        // Stolen from range-v3
        template <typename T>
        struct static_const {
            static constexpr T value{};
        };
        template <typename T>
        constexpr T static_const<T>::value;

        template <size_t I>
        struct priority_tag : priority_tag<I - 1> {
        };
        template <>
        struct priority_tag<0> {
        };

        template <typename T>
        struct dependent_false : std::false_type {
        };

        // Stolen from NanoRange
        struct nonesuch {
            nonesuch() = delete;
            nonesuch(nonesuch const&) = delete;
            nonesuch& operator=(const nonesuch&) = delete;
            ~nonesuch() = delete;
        };

        template <typename Void,
                  template <class...>
                  class Trait,
                  typename... Args>
        struct _test {
            using type = nonesuch;
        };

        template <template <class...> class Trait, typename... Args>
        struct _test<void_t<Trait<Args...>>, Trait, Args...> {
            using type = Trait<Args...>;
        };

        template <template <class...> class Trait, typename... Args>
        using _test_t = typename _test<void, Trait, Args...>::type;

        template <typename Void,
                  template <class...>
                  class AliasT,
                  typename... Args>
        struct exists_helper : std::false_type {
        };

        template <template <class...> class AliasT, typename... Args>
        struct exists_helper<void_t<AliasT<Args...>>, AliasT, Args...>
            : std::true_type {
        };

        template <template <class...> class AliasT, typename... Args>
        struct exists : exists_helper<void, AliasT, Args...> {
        };

        template <typename R,
                  typename... Args,
                  typename = decltype(&R::template _test_requires<Args...>)>
        auto test_requires(R&) -> void;

        template <typename R, typename... Args>
        using test_requires_t =
            decltype(test_requires<R, Args...>(std::declval<R&>()));

        template <typename R, typename... Args>
        struct _requires : exists<test_requires_t, R, Args...> {
        };

        template <bool Expr>
        using requires_expr = typename std::enable_if<Expr, int>::type;

        template <typename...>
        struct get_common_type;

        template <typename T, typename U>
        struct _copy_cv {
            using type = U;
        };
        template <typename T, typename U>
        struct _copy_cv<const T, U> {
            using type = typename std::add_const<U>::type;
        };
        template <typename T, typename U>
        struct _copy_cv<volatile T, U> {
            using type = typename std::add_volatile<U>::type;
        };
        template <typename T, typename U>
        struct _copy_cv<const volatile T, U> {
            using type = typename std::add_cv<U>::type;
        };
        template <typename T, typename U>
        using _copy_cv_t = typename _copy_cv<T, U>::type;

        template <typename T>
        using _cref_t = typename std::add_lvalue_reference<
            const typename std::remove_reference<T>::type>::type;

        template <typename T>
        struct _rref_res {
            using type = T;
        };
        template <typename T>
        struct _rref_res<T&> {
            using type = typename std::remove_reference<T>::type&&;
        };
        template <typename T>
        using _rref_res_t = typename _rref_res<T>::type;

        template <typename T, typename U>
        using _cond_res_t =
            decltype(std::declval<bool>() ? std::declval<T (&)()>()()
                                          : std::declval<U (&)()>()());

        template <typename T, typename U>
        struct simple_common_reference {
        };

        template <
            typename T,
            typename U,
            typename C =
                _test_t<_cond_res_t, _copy_cv_t<T, U>&, _copy_cv_t<U, T>&>>
        struct lvalue_simple_common_reference
            : std::enable_if<std::is_reference<C>::value, C> {
        };
        template <typename T, typename U>
        using lvalue_scr_t =
            typename lvalue_simple_common_reference<T, U>::type;
        template <typename T, typename U>
        struct simple_common_reference<T&, U&>
            : lvalue_simple_common_reference<T, U> {
        };

        template <typename T,
                  typename U,
                  typename LCR = _test_t<lvalue_scr_t, T, U>,
                  typename C = _rref_res_t<LCR>>
        struct rvalue_simple_common_reference
            : std::enable_if<std::is_convertible<T&&, C>::value &&
                             std::is_convertible<U&&, C>::value>::type {
        };
        template <typename T, typename U>
        struct simple_common_reference<T&&, U&&>
            : rvalue_simple_common_reference<T, U> {
        };

        template <typename A,
                  typename B,
                  typename C = _test_t<lvalue_scr_t, A, const B>>
        struct mixed_simple_common_reference
            : std::enable_if<std::is_convertible<B&&, C>::value, C>::type {
        };

        template <typename A, typename B>
        struct simple_common_reference<A&, B&&>
            : mixed_simple_common_reference<A, B> {
        };
        template <typename A, typename B>
        struct simple_common_reference<A&&, B&>
            : simple_common_reference<B&&, A&> {
        };
        template <typename T, typename U>
        using simple_common_reference_t =
            typename simple_common_reference<T, U>::type;

        template <typename>
        struct xref {
            template <typename U>
            using type = U;
        };

        template <typename A>
        struct xref<A&> {
            template <typename U>
            using type = typename std::add_lvalue_reference<
                typename xref<A>::template type<U>>::type;
        };

        template <typename A>
        struct xref<A&&> {
            template <typename U>
            using type = typename std::add_rvalue_reference<
                typename xref<A>::template type<U>>::type;
        };

        template <typename A>
        struct xref<const A> {
            template <typename U>
            using type = typename std::add_const<
                typename xref<A>::template type<U>>::type;
        };

        template <typename A>
        struct xref<volatile A> {
            template <typename U>
            using type = typename std::add_volatile<
                typename xref<A>::template type<U>>::type;
        };

        template <typename A>
        struct xref<const volatile A> {
            template <typename U>
            using type =
                typename std::add_cv<typename xref<A>::template type<U>>::type;
        };

        template <typename T,
                  typename U,
                  template <class>
                  class TQual,
                  template <class>
                  class UQual>
        struct basic_common_reference {
        };

        template <typename...>
        struct get_common_reference;
        template <typename... Ts>
        using get_common_reference_t =
            typename get_common_reference<Ts...>::type;

        template <>
        struct get_common_reference<> {
        };
        template <typename T0>
        struct get_common_reference<T0> {
            using type = T0;
        };

        template <typename T, typename U>
        struct has_simple_common_ref : exists<simple_common_reference_t, T, U> {
        };
        template <typename T, typename U>
        using basic_common_ref_t =
            typename basic_common_reference<remove_cvref_t<T>,
                                            remove_cvref_t<U>,
                                            xref<T>::template type,
                                            xref<U>::template type>::type;

        template <typename T, typename U>
        struct has_basic_common_ref : exists<basic_common_ref_t, T, U> {
        };
        template <typename T, typename U>
        struct has_cond_res : exists<_cond_res_t, T, U> {
        };

        template <typename T, typename U, typename = void>
        struct binary_common_ref : get_common_type<T, U> {
        };
        template <typename T, typename U>
        struct binary_common_ref<
            T,
            U,
            typename std::enable_if<has_simple_common_ref<T, U>::value>::type>
            : simple_common_reference<T, U> {
        };
        template <typename T, typename U>
        struct binary_common_ref<
            T,
            U,
            typename std::enable_if<
                has_basic_common_ref<T, U>::value &&
                !has_simple_common_ref<T, U>::value>::type> {
            using type = basic_common_ref_t<T, U>;
        };
        template <typename T, typename U>
        struct binary_common_ref<
            T,
            U,
            typename std::enable_if<
                has_cond_res<T, U>::value &&
                !has_basic_common_ref<T, U>::value &&
                !has_simple_common_ref<T, U>::value>::type> {
            using type = _cond_res_t<T, U>;
        };
        template <typename T1, typename T2>
        struct get_common_reference<T1, T2> : binary_common_ref<T1, T2> {
        };

        template <typename Void, typename T1, typename T2, typename... Rest>
        struct multiple_common_reference {
        };
        template <typename T1, typename T2, typename... Rest>
        struct multiple_common_reference<void_t<get_common_reference_t<T1, T2>>,
                                         T1,
                                         T2,
                                         Rest...>
            : get_common_reference<get_common_reference_t<T1, T2>, Rest...> {
        };
        template <typename T1, typename T2, typename... Rest>
        struct get_common_reference<T1, T2, Rest...>
            : multiple_common_reference<void, T1, T2, Rest...> {
        };

        template <typename... Ts>
        using get_common_type_t = typename get_common_type<Ts...>::type;

        template <typename T, typename U>
        struct _same_decayed
            : std::integral_constant<
                  bool,
                  std::is_same<T, typename std::decay<T>::type>::value &&
                      std::is_same<U, typename std::decay<U>::type>::value> {
        };

        template <typename T, typename U>
        using ternary_return_t = typename std::decay<decltype(
            false ? std::declval<T>() : std::declval<U>())>::type;

        template <typename, typename, typename = void>
        struct binary_common_type {
        };

        template <typename T, typename U>
        struct binary_common_type<
            T,
            U,
            typename std::enable_if<!_same_decayed<T, U>::value>::type>
            : get_common_type<typename std::decay<T>::type,
                              typename std::decay<U>::type> {
        };

        template <typename T, typename U>
        struct binary_common_type<
            T,
            U,
            typename std::enable_if<
                _same_decayed<T, U>::value &&
                exists<ternary_return_t, T, U>::value>::type> {
            using type = ternary_return_t<T, U>;
        };

        template <typename T, typename U>
        struct binary_common_type<
            T,
            U,
            typename std::enable_if<
                _same_decayed<T, U>::value &&
                !exists<ternary_return_t, T, U>::value &&
                exists<_cond_res_t, _cref_t<T>, _cref_t<U>>::value>::type> {
            using type =
                typename std::decay<_cond_res_t<_cref_t<T>, _cref_t<U>>>::type;
        };

        template <>
        struct get_common_type<> {
        };

        template <typename T>
        struct get_common_type<T> : get_common_type<T, T> {
        };

        template <typename T, typename U>
        struct get_common_type<T, U> : binary_common_type<T, U> {
        };

        template <typename Void, typename...>
        struct multiple_common_type {
        };

        template <typename T1, typename T2, typename... R>
        struct multiple_common_type<void_t<get_common_type_t<T1, T2>>,
                                    T1,
                                    T2,
                                    R...>
            : get_common_type<get_common_type_t<T1, T2>, R...> {
        };

        template <typename T1, typename T2, typename... R>
        struct get_common_type<T1, T2, R...>
            : multiple_common_type<void, T1, T2, R...> {
        };

        template <typename T>
        constexpr typename std::decay<T>::type decay_copy(T&& t) noexcept(
            noexcept(
                static_cast<typename std::decay<T>::type>(std::forward<T>(t))))
        {
            return std::forward<T>(t);
        }

        template <typename Integral>
        SCN_CONSTEXPR14 int _max_digits(int base) noexcept
        {
            using lim = std::numeric_limits<Integral>;

            char base8_digits[8] = {3, 5, 0, 11, 0, 0, 0, 21};

            if (base == 10) {
                return lim::digits10;
            }
            if (base == 8) {
                return static_cast<int>(base8_digits[sizeof(Integral) - 1]);
            }
            if (base == lim::radix) {
                return lim::digits;
            }

            auto i = lim::max();

            Integral digits = 0;
            while (i) {
                i /= static_cast<Integral>(base);
                digits++;
            }
            return static_cast<int>(digits);
        }
        template <typename Integral>
        SCN_CONSTEXPR14 int max_digits(int base) noexcept
        {
            auto b = base == 0 ? 8 : base;
            auto d = _max_digits<Integral>(b) +
                     (std::is_signed<Integral>::value ? 1 : 0);
            if (base == 0) {
                return d + 2;  // accommondate for 0x/0o
            }
            return d;
        }

        template <typename T>
        constexpr std::pair<T, T> div(T l, T r) noexcept
        {
            return {l / r, l % r};
        }

        template <typename T>
        constexpr T* launder(T* p) noexcept
        {
#if SCN_HAS_LAUNDER
            return std::launder(p);
#else
            return p;
#endif
        }

        template <typename CharT>
        CharT ascii_widen(char ch);
        template <>
        constexpr char ascii_widen(char ch)
        {
            return ch;
        }
        template <>
        constexpr wchar_t ascii_widen(char ch)
        {
            return static_cast<wchar_t>(ch);
        }

        template <typename T>
        constexpr T max(T a, T b) noexcept
        {
            return (a < b) ? b : a;
        }

        template <typename It>
        SCN_CONSTEXPR14 It min_element(It first, It last)
        {
            if (first == last) {
                return last;
            }

            It smallest = first;
            ++first;
            for (; first != last; ++first) {
                if (*first < *smallest) {
                    smallest = first;
                }
            }
            return smallest;
        }

        template <typename T>
        constexpr T min(T a, T b) noexcept
        {
            return (b < a) ? b : a;
        }
        template <typename T>
        SCN_CONSTEXPR14 T min(std::initializer_list<T> list) noexcept
        {
            return *min_element(list.begin(), list.end());
        }

        template <typename CharT>
        bool is_base_digit(CharT ch, int base)
        {
            if (base <= 10) {
                return ch >= ascii_widen<CharT>('0') &&
                       ch <= ascii_widen<CharT>('0') + base - 1;
            }
            return is_base_digit(ch, 10) ||
                   (ch >= ascii_widen<CharT>('a') &&
                    ch <= ascii_widen<CharT>('a') + base - 1) ||
                   (ch >= ascii_widen<CharT>('A') &&
                    ch <= ascii_widen<CharT>('A') + base - 1);
        }

        template <typename T>
        class SCN_TRIVIAL_ABI unique_ptr {
        public:
            using element_type = T;
            using pointer = T*;

            constexpr unique_ptr() noexcept = default;
            constexpr unique_ptr(std::nullptr_t) noexcept {}

            constexpr explicit unique_ptr(pointer p) noexcept : m_ptr(p) {}

            template <
                typename U,
                typename std::enable_if<
                    std::is_convertible<U*, pointer>::value>::type* = nullptr>
            SCN_CONSTEXPR14 unique_ptr(unique_ptr<U>&& u) noexcept
                : m_ptr(std::move(u.get()))
            {
                u.reset();
            }

            unique_ptr(const unique_ptr&) = delete;
            unique_ptr& operator=(const unique_ptr&) = delete;

            SCN_CONSTEXPR14 unique_ptr(unique_ptr&& p) noexcept
                : m_ptr(std::move(p.m_ptr))
            {
                p.m_ptr = nullptr;
            }
            unique_ptr& operator=(unique_ptr&& p) noexcept
            {
                if (m_ptr) {
                    delete m_ptr;
                }
                m_ptr = p.m_ptr;
                p.m_ptr = nullptr;
                return *this;
            }

            ~unique_ptr() noexcept
            {
                if (m_ptr) {
                    delete m_ptr;
                }
            }

            constexpr explicit operator bool() const noexcept
            {
                return get() != nullptr;
            }

            constexpr pointer get() const noexcept
            {
                return m_ptr;
            }

            constexpr pointer operator->() const noexcept
            {
                return m_ptr;
            }
            constexpr typename std::add_lvalue_reference<T>::type operator*()
                const
            {
                return *m_ptr;
            }

            SCN_CONSTEXPR14 void reset()
            {
                m_ptr = nullptr;
            }

        private:
            pointer m_ptr{nullptr};
        };

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")

        template <typename T, typename... Args>
        unique_ptr<T> make_unique(Args&&... a)
        {
            return unique_ptr<T>(new T(std::forward<Args>(a)...));
        }

        template <typename T, std::size_t N>
        struct array {
            static_assert(N > 0, "zero-sized array not supported");

            using value_type = T;
            using size_type = std::size_t;
            using difference_type = std::ptrdiff_t;
            using reference = T&;
            using const_reference = const T&;
            using pointer = T*;
            using const_pointer = const T*;
            using iterator = pointer;
            using const_iterator = const_pointer;

            SCN_CONSTEXPR14 reference operator[](size_type i)
            {
                SCN_EXPECT(i < size());
                return m_data[i];
            }
            constexpr const_reference operator[](size_type i) const
            {
                SCN_EXPECT(i < size());
                return m_data[i];
            }

            SCN_CONSTEXPR14 iterator begin() noexcept
            {
                return m_data;
            }
            constexpr const_iterator begin() const noexcept
            {
                return m_data;
            }
            constexpr const_iterator cbegin() const noexcept
            {
                return m_data;
            }

            SCN_CONSTEXPR14 iterator end() noexcept
            {
                return m_data + N;
            }
            constexpr const_iterator end() const noexcept
            {
                return m_data + N;
            }
            constexpr const_iterator cend() const noexcept
            {
                return m_data + N;
            }

            SCN_CONSTEXPR14 pointer data() noexcept
            {
                return m_data;
            }
            constexpr const_pointer data() const noexcept
            {
                return m_data;
            }

            constexpr size_type size() const noexcept
            {
                return N;
            }

            T m_data[N];
        };

        template <typename T>
        class SCN_TRIVIAL_ABI erased_storage {
        public:
            using value_type = T;
            using pointer = T*;
            using storage_type = unsigned char[sizeof(T)];

            constexpr erased_storage() noexcept = default;

            erased_storage(T val) noexcept(
                std::is_nothrow_move_constructible<T>::value)
                : m_ptr(::new (static_cast<void*>(&m_data)) T(std::move(val)))
            {
            }

            erased_storage(const erased_storage& other)
                : m_ptr(other ? ::new (static_cast<void*>(&m_data))
                                    T(other.get())
                              : nullptr)
            {
            }
            erased_storage& operator=(const erased_storage& other)
            {
                _destruct();
                if (other) {
                    m_ptr = ::new (static_cast<void*>(&m_data)) T(other.get());
                }
                return *this;
            }

            erased_storage(erased_storage&& other) noexcept
                : m_ptr(other ? ::new (static_cast<void*>(&m_data))
                                    T(std::move(other.get()))
                              : nullptr)
            {
                other.m_ptr = nullptr;
            }
            erased_storage& operator=(erased_storage&& other) noexcept
            {
                _destruct();
                if (other) {
                    m_ptr = ::new (static_cast<void*>(&m_data))
                        T(std::move(other.get()));
                    other.m_ptr = nullptr;
                }
                return *this;
            }

            ~erased_storage() noexcept
            {
                _destruct();
            }

            constexpr bool has_value() const noexcept
            {
                return m_ptr != nullptr;
            }
            constexpr explicit operator bool() const noexcept
            {
                return has_value();
            }

            SCN_CONSTEXPR14 T& get() noexcept
            {
                SCN_EXPECT(has_value());
                return _get();
            }
            SCN_CONSTEXPR14 const T& get() const noexcept
            {
                SCN_EXPECT(has_value());
                return _get();
            }

            SCN_CONSTEXPR14 T& operator*() noexcept
            {
                SCN_EXPECT(has_value());
                return _get();
            }
            SCN_CONSTEXPR14 const T& operator*() const noexcept
            {
                SCN_EXPECT(has_value());
                return _get();
            }

            SCN_CONSTEXPR14 T* operator->() noexcept
            {
                return m_ptr;
            }
            SCN_CONSTEXPR14 const T* operator->() const noexcept
            {
                return m_ptr;
            }

        private:
            void _destruct()
            {
                if (m_ptr) {
                    _get().~T();
                }
                m_ptr = nullptr;
            }
            static pointer _toptr(storage_type& data)
            {
                return ::scn::detail::launder(
                    reinterpret_cast<T*>(std::addressof(data)));
            }
            SCN_CONSTEXPR14 T& _get() noexcept
            {
                return *m_ptr;
            }
            SCN_CONSTEXPR14 const T& _get() const noexcept
            {
                return *m_ptr;
            }

            alignas(T) storage_type m_data{};
            pointer m_ptr{nullptr};
        };

        SCN_CLANG_POP
    }  // namespace detail

    template <typename T>
    class wrap_default {
    public:
        using value_type = T;
        using storage_type = detail::erased_storage<T>;

        wrap_default() = default;

        wrap_default(value_type&& val) : m_storage(std::move(val)) {}
        wrap_default& operator=(value_type&& val)
        {
            m_storage = storage_type(std::move(val));
            return *this;
        }

        constexpr bool has_value() const noexcept
        {
            return m_storage.operator bool();
        }
        constexpr explicit operator bool() const noexcept
        {
            return has_value();
        }

        SCN_CONSTEXPR14 T& get() noexcept
        {
            return m_storage.get();
        }
        SCN_CONSTEXPR14 const T& get() const noexcept
        {
            return m_storage.get();
        }

        SCN_CONSTEXPR14 T& operator*() noexcept
        {
            return get();
        }
        SCN_CONSTEXPR14 const T& operator*() const noexcept
        {
            return get();
        }

        SCN_CONSTEXPR14 T* operator->() noexcept
        {
            return m_storage.operator->();
        }
        SCN_CONSTEXPR14 const T* operator->() const noexcept
        {
            return m_storage.operator->();
        }

    private:
        storage_type m_storage;
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_UTIL_H

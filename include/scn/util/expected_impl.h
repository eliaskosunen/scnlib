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

#include <array>
#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

namespace scn {
    SCN_BEGIN_NAMESPACE

    struct monostate {};

    template <typename T, typename E>
    class expected;

    template <typename E>
    class SCN_TRIVIAL_ABI unexpected {
        static_assert(std::is_destructible<E>::value);

    public:
        unexpected() = delete;

        template <typename Err = E,
                  typename = std::enable_if_t<
                      !std::is_same<Err, unexpected>::value &&
                      !std::is_same<Err, std::in_place_t>::value &&
                      std::is_constructible<E, Err>::value>>
        constexpr explicit unexpected(Err&& e)
            : m_unexpected(std::forward<Err>(e))
        {
        }

        template <typename... Args,
                  typename = std::enable_if_t<
                      std::is_constructible<E, Args...>::value>>
        constexpr explicit unexpected(std::in_place_t, Args&&... args)
            : m_unexpected(std::forward<Args>(args)...)
        {
        }

        constexpr E& error() & SCN_NOEXCEPT
        {
            return m_unexpected;
        }
        constexpr const E& error() const& SCN_NOEXCEPT
        {
            return m_unexpected;
        }

        constexpr E&& error() && SCN_NOEXCEPT
        {
            return std::move(m_unexpected);
        }
        constexpr const E&& error() const&& SCN_NOEXCEPT
        {
            return std::move(m_unexpected);
        }

    private:
        E m_unexpected;
    };

    template <typename E>
    unexpected(E) -> unexpected<E>;

    struct unexpect_t {};
    inline constexpr unexpect_t unexpect{};

    namespace detail {
        template <typename T, typename... Args>
        T* construct_at(T* p, Args&&... args)
        {
            return ::new (
                const_cast<void*>(static_cast<const volatile void*>(p)))
                T(std::forward<Args>(args)...);
        }
        template <typename T>
        void destroy_at(T* p)
        {
            if constexpr (std::is_array_v<T>) {
                for (auto& elem : *p) {
                    scn::detail::destroy_at(std::addressof(elem));
                }
            }
            else {
                p->~T();
            }
        }

        struct deferred_init_tag_t {};
        static constexpr deferred_init_tag_t deferred_init_tag{};

        template <typename T,
                  typename E,
                  bool IsTriviallyDestructible =
                      std::is_trivially_destructible_v<T>&&
                          std::is_trivially_destructible_v<E>>
        struct expected_storage_base;

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_storage_base<T, E, true> {
            constexpr expected_storage_base() : m_value(T{}), m_has_value(true)
            {
            }
            constexpr expected_storage_base(deferred_init_tag_t) SCN_NOEXCEPT
                : m_deferred_init(),
                  m_has_value(false)
            {
            }

            template <typename... Args,
                      typename = std::enable_if_t<
                          std::is_constructible<T, Args...>::value>>
            constexpr expected_storage_base(std::in_place_t, Args&&... args)
                : m_value(std::forward<Args>(args)...), m_has_value(true)
            {
            }

            template <typename... Args,
                      typename = std::enable_if_t<
                          std::is_constructible<E, Args...>::value>>
            constexpr expected_storage_base(unexpect_t, Args&&... args)
                : m_unexpected(std::in_place, std::forward<Args>(args)...),
                  m_has_value(false)
            {
            }

            constexpr T& get_value() & SCN_NOEXCEPT
            {
                return m_value;
            }
            constexpr const T& get_value() const& SCN_NOEXCEPT
            {
                return m_value;
            }
            constexpr T&& get_value() && SCN_NOEXCEPT
            {
                return std::move(m_value);
            }
            constexpr const T&& get_value() const&& SCN_NOEXCEPT
            {
                return std::move(m_value);
            }

            constexpr unexpected<E>& get_unexpected() & SCN_NOEXCEPT
            {
                return m_unexpected;
            }
            constexpr const unexpected<E>& get_unexpected() const& SCN_NOEXCEPT
            {
                return m_unexpected;
            }
            constexpr unexpected<E>&& get_unexpected() && SCN_NOEXCEPT
            {
                return std::move(m_unexpected);
            }
            constexpr const unexpected<E>&& get_unexpected()
                const&& SCN_NOEXCEPT
            {
                return std::move(m_unexpected);
            }

            constexpr bool has_value() const SCN_NOEXCEPT
            {
                return m_has_value;
            }

            template <typename... Args>
            void construct(Args&&... args)
            {
                scn::detail::construct_at(std::addressof(get_value()),
                                          std::forward<Args>(args)...);
                m_has_value = true;
            }
            template <typename... Args>
            void construct_unexpected(Args&&... args)
            {
                scn::detail::construct_at(std::addressof(get_unexpected()),
                                          std::forward<Args>(args)...);
                m_has_value = false;
            }

            // No-op, because T and E are trivially destructible
            void destroy_value() {}
            void destroy_unexpected() {}

        private:
            union {
                T m_value;
                unexpected<E> m_unexpected;
                char m_deferred_init;
            };
            bool m_has_value;
        };

        template <typename E>
        struct SCN_TRIVIAL_ABI expected_storage_base<void, E, true> {
            constexpr expected_storage_base() : m_has_value(true) {}
            constexpr expected_storage_base(deferred_init_tag_t) SCN_NOEXCEPT
                : m_deferred_init(),
                  m_has_value(false)
            {
            }

            constexpr expected_storage_base(std::in_place_t) : m_has_value(true)
            {
            }

            template <typename... Args,
                      typename = std::enable_if_t<
                          std::is_constructible<E, Args...>::value>>
            constexpr expected_storage_base(unexpect_t, Args&&... args)
                : m_unexpected(std::in_place, std::forward<Args>(args)...),
                  m_has_value(false)
            {
            }

            constexpr unexpected<E>& get_unexpected() & SCN_NOEXCEPT
            {
                return m_unexpected;
            }
            constexpr const unexpected<E>& get_unexpected() const& SCN_NOEXCEPT
            {
                return m_unexpected;
            }
            constexpr unexpected<E>&& get_unexpected() && SCN_NOEXCEPT
            {
                return std::move(m_unexpected);
            }
            constexpr const unexpected<E>&& get_unexpected()
                const&& SCN_NOEXCEPT
            {
                return std::move(m_unexpected);
            }

            constexpr bool has_value() const SCN_NOEXCEPT
            {
                return m_has_value;
            }

            template <typename... Args>
            void construct(Args&&...)
            {
                m_has_value = true;
            }
            template <typename... Args>
            void construct_unexpected(Args&&... args)
            {
                scn::detail::construct_at(std::addressof(get_unexpected()),
                                          std::forward<Args>(args)...);
                m_has_value = false;
            }

            void destroy_value() {}
            void destroy_unexpected() {}

        private:
            union {
                unexpected<E> m_unexpected;
                char m_deferred_init;
            };
            bool m_has_value;
        };

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_storage_base<T, E, false> {
            constexpr expected_storage_base() : m_has_value(true)
            {
                construct();
            }
            constexpr expected_storage_base(deferred_init_tag_t) SCN_NOEXCEPT
                : m_has_value(false)
            {
            }

            template <typename... Args,
                      typename = std::enable_if_t<
                          std::is_constructible<T, Args...>::value>>
            constexpr expected_storage_base(std::in_place_t, Args&&... args)
                : m_has_value(true)
            {
                construct(std::forward<Args>(args)...);
            }

            template <typename... Args,
                      typename = std::enable_if_t<
                          std::is_constructible<E, Args...>::value>>
            constexpr expected_storage_base(unexpect_t, Args&&... args)
                : m_has_value(false)
            {
                construct_unexpected(std::in_place,
                                     std::forward<Args>(args)...);
            }

            ~expected_storage_base()
            {
                if (has_value()) {
                    destroy_value();
                }
                else {
                    destroy_unexpected();
                }
            }

            constexpr T& get_value() & SCN_NOEXCEPT
            {
                return *value_ptr();
            }
            constexpr const T& get_value() const& SCN_NOEXCEPT
            {
                return *value_ptr();
            }
            constexpr T&& get_value() && SCN_NOEXCEPT
            {
                return std::move(*value_ptr());
            }
            constexpr const T&& get_value() const&& SCN_NOEXCEPT
            {
                return std::move(*value_ptr());
            }

            constexpr unexpected<E>& get_unexpected() & SCN_NOEXCEPT
            {
                return *unexpected_ptr();
            }
            constexpr const unexpected<E>& get_unexpected() const& SCN_NOEXCEPT
            {
                return *unexpected_ptr();
            }
            constexpr unexpected<E>&& get_unexpected() && SCN_NOEXCEPT
            {
                return std::move(*unexpected_ptr());
            }
            constexpr const unexpected<E>&& get_unexpected()
                const&& SCN_NOEXCEPT
            {
                return std::move(*unexpected_ptr());
            }

            constexpr bool has_value() const SCN_NOEXCEPT
            {
                return m_has_value;
            }

            template <typename... Args>
            void construct(Args&&... args)
            {
                scn::detail::construct_at(value_ptr(),
                                          std::forward<Args>(args)...);
                m_has_value = true;
            }
            template <typename... Args>
            void construct_unexpected(Args&&... args)
            {
                scn::detail::construct_at(unexpected_ptr(),
                                          std::forward<Args>(args)...);
                m_has_value = false;
            }

            void destroy_value()
            {
                scn::detail::destroy_at(value_ptr());
            }
            void destroy_unexpected()
            {
                scn::detail::destroy_at(unexpected_ptr());
            }

        private:
            T* value_ptr()
            {
                return reinterpret_cast<T*>(m_memory.data());
            }
            const T* value_ptr() const
            {
                return reinterpret_cast<const T*>(m_memory.data());
            }

            unexpected<E>* unexpected_ptr()
            {
                return reinterpret_cast<unexpected<E>*>(m_memory.data());
            }
            const unexpected<E>* unexpected_ptr() const
            {
                return reinterpret_cast<const unexpected<E>*>(m_memory.data());
            }

            static constexpr std::size_t required_size =
                std::max(sizeof(T), sizeof(unexpected<E>));
            static constexpr std::size_t required_alignment =
                std::max(alignof(T), alignof(unexpected<E>));

            alignas(required_alignment)
                std::array<unsigned char, required_size> m_memory;
            bool m_has_value;
        };

        template <typename E>
        struct SCN_TRIVIAL_ABI expected_storage_base<void, E, false> {
            constexpr expected_storage_base() : m_has_value(true) {}
            constexpr expected_storage_base(deferred_init_tag_t) SCN_NOEXCEPT
                : m_has_value(false)
            {
            }

            constexpr expected_storage_base(std::in_place_t) : m_has_value(true)
            {
            }

            template <typename... Args,
                      typename = std::enable_if_t<
                          std::is_constructible<E, Args...>::value>>
            constexpr expected_storage_base(unexpect_t, Args&&... args)
                : m_has_value(false)
            {
                construct_unexpected(std::in_place,
                                     std::forward<Args>(args)...);
            }

            ~expected_storage_base()
            {
                if (!has_value()) {
                    destroy_unexpected();
                }
            }

            constexpr unexpected<E>& get_unexpected() & SCN_NOEXCEPT
            {
                return *unexpected_ptr();
            }
            constexpr const unexpected<E>& get_unexpected() const& SCN_NOEXCEPT
            {
                return *unexpected_ptr();
            }
            constexpr unexpected<E>&& get_unexpected() && SCN_NOEXCEPT
            {
                return std::move(*unexpected_ptr());
            }
            constexpr const unexpected<E>&& get_unexpected()
                const&& SCN_NOEXCEPT
            {
                return std::move(*unexpected_ptr());
            }

            constexpr bool has_value() const SCN_NOEXCEPT
            {
                return m_has_value;
            }

            template <typename... Args>
            void construct(Args&&...)
            {
                m_has_value = true;
            }
            template <typename... Args>
            void construct_unexpected(Args&&... args)
            {
                scn::detail::construct_at(unexpected_ptr(),
                                          std::forward<Args>(args)...);
                m_has_value = false;
            }

            void destroy_value() {}
            void destroy_unexpected()
            {
                scn::detail::destroy_at(unexpected_ptr());
            }

        private:
            unexpected<E>* unexpected_ptr()
            {
                return reinterpret_cast<unexpected<E>*>(m_memory.data());
            }
            const unexpected<E>* unexpected_ptr() const
            {
                return reinterpret_cast<const unexpected<E>*>(m_memory.data());
            }

            static constexpr std::size_t required_size = sizeof(unexpected<E>);
            static constexpr std::size_t required_alignment =
                alignof(unexpected<E>);

            alignas(required_alignment)
                std::array<unsigned char, required_size> m_memory;
            bool m_has_value;
        };

        template <typename T, typename U>
        using is_void_or =
            std::conditional_t<std::is_void_v<T>, std::true_type, U>;

        template <typename T,
                  typename E,
                  bool IsTriviallyCopyable =
                      std::conjunction<std::is_trivially_copyable<T>,
                                       std::is_trivially_copyable<E>>::value>
        struct expected_operations_base;

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_operations_base<T, E, true>
            : expected_storage_base<T, E> {
            using expected_storage_base<T, E>::expected_storage_base;
        };

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_operations_base<T, E, false>
            : expected_storage_base<T, E> {
            using expected_storage_base<T, E>::expected_storage_base;

            expected_operations_base(const expected_operations_base& other)
                : expected_storage_base<T, E>(deferred_init_tag)
            {
                construct_common(other);
            }
            expected_operations_base(expected_operations_base&& other)
                : expected_storage_base<T, E>(deferred_init_tag)
            {
                construct_common(std::move(other));
            }

            expected_operations_base& operator=(
                const expected_operations_base& other)
            {
                assign_common(other);
                return *this;
            }
            expected_operations_base& operator=(
                expected_operations_base&& other)
            {
                assign_common(std::move(other));
                return *this;
            }

            ~expected_operations_base() = default;

        private:
            template <typename Other>
            void construct_common(Other&& other)
            {
                if (other.has_value()) {
                    this->construct(std::forward<Other>(other).get_value());
                }
                else {
                    this->construct_unexpected(
                        std::forward<Other>(other).get_unexpected());
                }
            }

            template <typename Other>
            void assign_common(Other&& other)
            {
                if (this->has_value()) {
                    if (other.has_value()) {
                        return reassign_value(std::forward<Other>(other));
                    }
                    return assign_unexpected_over_value(
                        std::forward<Other>(other));
                }

                if (other.has_value()) {
                    return assign_value_over_unexpected(
                        std::forward<Other>(other));
                }
                return reassign_unexpected(std::forward<Other>(other));
            }

            template <typename Other>
            void reassign_value(Other&& other)
            {
                this->get_value() = std::forward<Other>(other).get_value();
            }

            template <typename Other>
            void reassign_unexpected(Other&& other)
            {
                this->get_unexpected() =
                    std::forward<Other>(other).get_unexpected();
            }

#if SCN_HAS_EXCEPTIONS
            void assign_value_over_unexpected(
                const expected_operations_base& other)
                SCN_NOEXCEPT_P(std::is_nothrow_copy_constructible_v<T> ||
                               std::is_nothrow_move_constructible_v<T>)
            {
                if constexpr (std::is_nothrow_copy_constructible_v<T>) {
                    this->destroy_unexpected();
                    this->construct(other.get_value());
                }
                else if constexpr (std::is_nothrow_move_constructible_v<T>) {
                    T tmp = other.get_value();
                    this->destroy_unexpected();
                    this->construct(std::move(tmp));
                }
                else {
                    auto tmp = std::move(this->get_unexpecetd());
                    this->destroy_unexpected();

                    try {
                        this->construct(other.get());
                    }
                    catch (...) {
                        this->construct_unexpected(std::move(tmp));
                        throw;
                    }
                }
            }

            void assign_value_over_unexpected(expected_operations_base&& other)
                SCN_NOEXCEPT_P(std::is_nothrow_move_constructible_v<T>)
            {
                if constexpr (std::is_nothrow_move_constructible_v<T>) {
                    this->destroy_unexpected();
                    this->construct(std::move(other).get_value());
                }
                else {
                    auto tmp = std::move(this->get_unexpected());
                    this->destroy_unexpected();

                    try {
                        this->construct(std::move(other).get_value());
                    }
                    catch (...) {
                        this->construct_unexpected(std::move(tmp));
                        throw;
                    }
                }
            }
#else
            template <typename Other>
            void assing_value_over_unexpected(Other&& other)
            {
                this->destroy_unexpected();
                this->construct_value(std::forward<Other>(other).get_value());
            }
#endif

            template <typename Other>
            void assign_unexpected_over_value(Other&& other)
            {
                this->destroy_value();
                this->construct_unexpected(
                    std::forward<Other>(other).get_unexpected());
            }
        };

        /**
         * base class trickery to conditionally mark copy and move
         * constructors of an expected as =deleted.
         *
         * We need to do this, because otherwise utilities like
         * std::is_copy_constructible wouldn't work for expected: the
         * constructors need to be explicitly =deleted, not just cause a
         * compiler error when trying to copy a value of a non-copyable
         * type.
         *
         * Rationale for doing this with base classes is above.
         */
        template <typename T,
                  typename E,
                  bool EnableCopy = (std::is_copy_constructible<T>::value &&
                                     std::is_copy_constructible<E>::value),
                  bool EnableMove = (std::is_move_constructible<T>::value &&
                                     std::is_move_constructible<E>::value)>
        struct expected_delete_ctor_base;

        // Implementation for types that are both copy and move
        // constructible: Copy and move constructors are =defaulted
        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_delete_ctor_base<T, E, true, true> {
            expected_delete_ctor_base() = default;
            expected_delete_ctor_base& operator=(
                const expected_delete_ctor_base&) = default;
            expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) =
                default;
            ~expected_delete_ctor_base() = default;

            expected_delete_ctor_base(const expected_delete_ctor_base&) =
                default;
            expected_delete_ctor_base(expected_delete_ctor_base&&) = default;
        };

        // Implementation for types that are neither copy nor move
        // constructible: Copy and move constructors are =deleted
        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_delete_ctor_base<T, E, false, false> {
            expected_delete_ctor_base() = default;
            expected_delete_ctor_base& operator=(
                const expected_delete_ctor_base&) = default;
            expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) =
                default;
            ~expected_delete_ctor_base() = default;

            expected_delete_ctor_base(const expected_delete_ctor_base&) =
                delete;
            expected_delete_ctor_base(expected_delete_ctor_base&&) = delete;
        };

        // Implementation for types that are move constructible, but not
        // copy constructible Copy constructor is =deleted, but move
        // constructor is =defaulted
        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_delete_ctor_base<T, E, false, true> {
            expected_delete_ctor_base() = default;
            expected_delete_ctor_base& operator=(
                const expected_delete_ctor_base&) = default;
            expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) =
                default;
            ~expected_delete_ctor_base() = default;

            expected_delete_ctor_base(const expected_delete_ctor_base&) =
                delete;
            expected_delete_ctor_base(expected_delete_ctor_base&&) = default;
        };

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_delete_ctor_base<T, E, true, false> {
            static_assert(dependent_false<T>::value,
                          "Nonsensical type: copy constructible, but not move "
                          "constructible");
        };

        /// Same as above, but for assignment
        template <typename T,
                  typename E,
                  bool EnableCopy = (std::is_copy_constructible<T>::value &&
                                     std::is_copy_constructible<E>::value &&
                                     std::is_copy_assignable<T>::value &&
                                     std::is_copy_assignable<E>::value),
                  bool EnableMove = (std::is_move_constructible<T>::value &&
                                     std::is_move_constructible<E>::value &&
                                     std::is_move_assignable<T>::value &&
                                     std::is_move_assignable<E>::value)>
        struct expected_delete_assign_base;

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_delete_assign_base<T, E, true, true> {
            expected_delete_assign_base() = default;
            expected_delete_assign_base(const expected_delete_assign_base&) =
                default;
            expected_delete_assign_base(expected_delete_assign_base&&) =
                default;
            ~expected_delete_assign_base() = default;

            expected_delete_assign_base& operator=(
                const expected_delete_assign_base&) = default;
            expected_delete_assign_base& operator=(
                expected_delete_assign_base&&) = default;
        };

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_delete_assign_base<T, E, false, false> {
            expected_delete_assign_base() = default;
            expected_delete_assign_base(const expected_delete_assign_base&) =
                default;
            expected_delete_assign_base(expected_delete_assign_base&&) =
                default;
            ~expected_delete_assign_base() = default;

            expected_delete_assign_base& operator=(
                const expected_delete_assign_base&) = delete;
            expected_delete_assign_base& operator=(
                expected_delete_assign_base&&) = delete;
        };

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_delete_assign_base<T, E, false, true> {
            expected_delete_assign_base() = default;
            expected_delete_assign_base(const expected_delete_assign_base&) =
                default;
            expected_delete_assign_base(expected_delete_assign_base&&) =
                default;
            ~expected_delete_assign_base() = default;

            expected_delete_assign_base& operator=(
                const expected_delete_assign_base&) = delete;
            expected_delete_assign_base& operator=(
                expected_delete_assign_base&&) = default;
        };

        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_delete_assign_base<T, E, true, false> {
            static_assert(dependent_false<T>::value,
                          "Nonsensical type: copy assignable, but not move "
                          "assignable");
        };

        struct non_default_ctor_tag_t {};

        /**
         * Same as above, but for the default constructor
         *
         * The constructor taking a non_default_ctor_tag_t is needed, to
         * signal that we're not default constructing.
         */
        template <typename T,
                  typename E,
                  bool = std::is_default_constructible<T>::value ||
                         std::is_void<T>::value>
        struct SCN_TRIVIAL_ABI expected_default_ctor_base {
            constexpr expected_default_ctor_base() = default;
            constexpr explicit expected_default_ctor_base(
                non_default_ctor_tag_t)
            {
            }
        };
        template <typename T, typename E>
        struct SCN_TRIVIAL_ABI expected_default_ctor_base<T, E, false> {
            constexpr expected_default_ctor_base() = delete;
            constexpr explicit expected_default_ctor_base(
                non_default_ctor_tag_t)
            {
            }
        };

        template <typename T>
        using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

        template <typename T>
        struct is_expected_impl : std::false_type {};
        template <typename T, typename E>
        struct is_expected_impl<expected<T, E>> : std::true_type {};
        template <typename T>
        using is_expected = is_expected_impl<remove_cvref_t<T>>;

        template <typename Exp>
        using is_exp_void =
            std::is_void<typename remove_cvref_t<Exp>::value_type>;

        // and_then

        template <
            typename Exp,
            typename F,
            typename std::enable_if_t<!is_exp_void<Exp>::value>* = nullptr,
            typename Ret = decltype(std::invoke(SCN_DECLVAL(F),
                                                *SCN_DECLVAL(Exp)))>
        constexpr auto and_then_impl(Exp&& exp, F&& f)
        {
            static_assert(is_expected<Ret>::value, "F must return an expected");

            return exp.has_value()
                       ? std::invoke(std::forward<F>(f),
                                     *std::forward<Exp>(exp))
                       : Ret(unexpect, std::forward<Exp>(exp).error());
        }
        template <typename Exp,
                  typename F,
                  typename std::enable_if_t<is_exp_void<Exp>::value>* = nullptr,
                  typename Ret = decltype(std::invoke(SCN_DECLVAL(F)))>
        constexpr auto and_then_impl(Exp&& exp, F&& f)
        {
            static_assert(is_expected<Ret>::value, "F must return an expected");

            return exp.has_value()
                       ? std::invoke(std::forward<F>(f))
                       : Ret(unexpect, std::forward<Exp>(exp).error());
        }

        // or_else

        template <
            typename Exp,
            typename F,
            typename Ret = decltype(std::invoke(SCN_DECLVAL(F),
                                                SCN_DECLVAL(Exp).error()))>
        constexpr auto or_else_impl(Exp&& exp, F&& f)
        {
            static_assert(is_expected<Ret>::value, "F must return an expected");

            return exp.has_value()
                       ? Ret(std::forward<Exp>(exp))
                       : std::invoke(std::forward<F>(f),
                                     std::forward<Exp>(exp).error());
        }

        // transform

        template <
            typename Exp,
            typename F,
            typename std::enable_if_t<!is_exp_void<Exp>::value>* = nullptr,
            typename Ret = decltype(std::invoke(SCN_DECLVAL(F),
                                                *SCN_DECLVAL(Exp)))>
        constexpr auto transform_impl(Exp&& exp, F&& f)
        {
            using result = typename remove_cvref_t<Exp>::template rebind<Ret>;
            if constexpr (std::is_void<Ret>::value) {
                if (exp.has_value()) {
                    std::invoke(std::forward<F>(f), *std::forward<Exp>(exp));
                    return result();
                }
                return result(unexpect, std::forward<Exp>(exp).error());
            }
            else {
                return exp.has_value()
                           ? result(std::invoke(std::forward<F>(f),
                                                *std::forward<Exp>(exp)))
                           : result(unexpect, std::forward<Exp>(exp).error());
            }
        }
        template <typename Exp,
                  typename F,
                  typename std::enable_if_t<is_exp_void<Exp>::value>* = nullptr,
                  typename Ret = decltype(std::invoke(SCN_DECLVAL(F)))>
        constexpr auto transform_impl(Exp&& exp, F&& f)
        {
            using result = typename remove_cvref_t<Exp>::template rebind<Ret>;
            if constexpr (std::is_void<Ret>::value) {
                if (exp.has_value()) {
                    std::invoke(std::forward<F>(f));
                    return result();
                }
                return result(unexpect, std::forward<Exp>(exp).error());
            }
            else {
                return exp.has_value()
                           ? result(std::invoke(std::forward<F>(f)))
                           : result(unexpect, std::forward<Exp>(exp).error());
            }
        }

        // transform_error

        template <
            typename Exp,
            typename F,
            typename std::enable_if_t<!is_exp_void<Exp>::value>* = nullptr,
            typename Ret = decltype(std::invoke(SCN_DECLVAL(F),
                                                SCN_DECLVAL(Exp).error()))>
        constexpr auto transform_error_impl(Exp&& exp, F&& f)
        {
            if constexpr (std::is_void<Ret>::value) {
                using result = expected<typename Exp::value_type, monostate>;
                if (exp.has_value()) {
                    return result(*std::forward<Exp>(exp));
                }

                std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
                return result(unexpect, monostate{});
            }
            else {
                using result =
                    expected<typename Exp::value_type, remove_cvref_t<Ret>>;
                return exp.has_value()
                           ? result(*std::forward<Exp>(exp))
                           : result(
                                 unexpect,
                                 std::invoke(std::forward<F>(f),
                                             std::forward<Exp>(exp).error()));
            }
        }
        template <
            typename Exp,
            typename F,
            typename std::enable_if_t<is_exp_void<Exp>::value>* = nullptr,
            typename Ret = decltype(std::invoke(SCN_DECLVAL(F),
                                                SCN_DECLVAL(Exp).error()))>
        constexpr auto transform_error_impl(Exp&& exp, F&& f)
        {
            if constexpr (std::is_void<Ret>::value) {
                using result = expected<typename Exp::value_type, monostate>;
                if (exp.has_value()) {
                    return result();
                }

                std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
                return result(unexpect, monostate{});
            }
            else {
                using result =
                    expected<typename Exp::value_type, remove_cvref_t<Ret>>;
                return exp.has_value()
                           ? result()
                           : result(
                                 unexpect,
                                 std::invoke(std::forward<F>(f),
                                             std::forward<Exp>(exp).error()));
            }
        }

        template <class T, class E, class U, class G, class UR, class GR>
        using enable_from_other = std::enable_if_t<
            std::is_constructible<T, UR>::value &&
            std::is_constructible<E, GR>::value &&
            !std::is_constructible<T, expected<U, G>&>::value &&
            !std::is_constructible<T, expected<U, G>&&>::value &&
            !std::is_constructible<T, const expected<U, G>&>::value &&
            !std::is_constructible<T, const expected<U, G>&&>::value &&
            !std::is_convertible<expected<U, G>&, T>::value &&
            !std::is_convertible<expected<U, G>&&, T>::value &&
            !std::is_convertible<const expected<U, G>&, T>::value &&
            !std::is_convertible<const expected<U, G>&&, T>::value>;
    }  // namespace detail

    template <typename T, typename E>
    class SCN_TRIVIAL_ABI expected
        : private detail::expected_operations_base<T, E>,
          private detail::expected_delete_ctor_base<T, E>,
          private detail::expected_delete_assign_base<T, E>,
          private detail::expected_default_ctor_base<T, E> {
        using base = detail::expected_operations_base<T, E>;
        using ctor_base = detail::expected_default_ctor_base<T, E>;

        static_assert(std::is_void<T>::value || std::is_destructible<T>::value,
                      "T must be void or Destructible");
        static_assert(std::is_destructible<E>::value, "E must be Destructible");

        static_assert(
            !std::is_same<std::remove_cv_t<T>, std::in_place_t>::value);
        static_assert(!std::is_same<std::remove_cv_t<T>, unexpect_t>::value);
        static_assert(!std::is_same<std::remove_cv_t<T>, unexpected<E>>::value);

    public:
        using value_type = T;
        using error_type = E;
        using unexpected_type = unexpected<E>;

        template <typename U>
        using rebind = expected<U, error_type>;

        // Special member functions are defaulted, implementations provided
        // by base classes

        constexpr expected() = default;

        constexpr expected(const expected&) = default;
        constexpr expected(expected&&) = default;
        constexpr expected& operator=(const expected&) = default;
        constexpr expected& operator=(expected&&) = default;

        ~expected() = default;

        /**
         * Construct an expected value.
         * Intentionally non-explicit, to make constructing an expected
         * value as transparent as possible.
         */
        template <typename U = value_type,
                  typename = std::enable_if_t<
                      std::is_convertible<U, value_type>::value>>
        /* implicit */ constexpr expected(U&& val)
            : base(std::in_place, std::forward<U>(val)),
              ctor_base(detail::non_default_ctor_tag_t{})
        {
        }

        /// Construct an expected value directly in-place
        template <typename... Args,
                  typename = std::enable_if_t<
                      std::is_constructible<T, Args...>::value>>
        constexpr explicit expected(std::in_place_t, Args&&... args)
            : base(std::in_place, std::forward<Args>(args)...),
              ctor_base(detail::non_default_ctor_tag_t{})
        {
        }

        template <typename G = E,
                  std::enable_if_t<std::is_constructible<E, const G&>::value>* =
                      nullptr,
                  std::enable_if_t<!std::is_convertible<const G&, E>::value>* =
                      nullptr>
        explicit constexpr expected(const unexpected<G>& e)
            : base(unexpect, e.error()),
              ctor_base(detail::non_default_ctor_tag_t{})
        {
        }
        template <typename G = E,
                  std::enable_if_t<std::is_constructible<E, const G&>::value>* =
                      nullptr,
                  std::enable_if_t<std::is_convertible<const G&, E>::value>* =
                      nullptr>
        /*implicit*/ constexpr expected(const unexpected<G>& e)
            : base(unexpect, e.error()),
              ctor_base(detail::non_default_ctor_tag_t{})
        {
        }

        template <
            typename G = E,
            std::enable_if_t<std::is_constructible<E, G&&>::value>* = nullptr,
            std::enable_if_t<!std::is_convertible<G&&, E>::value>* = nullptr>
        explicit constexpr expected(unexpected<G>&& e)
            : base(unexpect, std::move(e.error())),
              ctor_base(detail::non_default_ctor_tag_t{})
        {
        }
        template <
            typename G = E,
            std::enable_if_t<std::is_constructible<E, G&&>::value>* = nullptr,
            std::enable_if_t<std::is_convertible<G&&, E>::value>* = nullptr>
        /*implicit*/ constexpr expected(unexpected<G>&& e)
            : base(unexpect, std::move(e.error())),
              ctor_base(detail::non_default_ctor_tag_t{})
        {
        }

        /// Construct an unexpected value directly in-place
        template <typename... Args,
                  typename = std::enable_if_t<
                      std::is_constructible<E, Args...>::value>>
        constexpr explicit expected(unexpect_t, Args&&... args)
            : base(unexpect, std::forward<Args>(args)...),
              ctor_base(detail::non_default_ctor_tag_t{})
        {
        }

        template <typename U,
                  typename G,
                  std::enable_if_t<
                      !(std::is_convertible<const U&, T>::value &&
                        std::is_convertible<const G&, E>::value)>* = nullptr,
                  detail::enable_from_other<T, E, U, G, const U&, const G&>* =
                      nullptr>
        explicit constexpr expected(const expected<U, G>& other)
            : base(), ctor_base(detail::non_default_ctor_tag_t{})
        {
            if (other.has_value()) {
                this->construct(*other);
            }
            else {
                this->construct_unexpected(other.error());
            }
        }
        template <typename U,
                  typename G,
                  std::enable_if_t<(std::is_convertible<const U&, T>::value &&
                                    std::is_convertible<const G&, E>::value)>* =
                      nullptr,
                  detail::enable_from_other<T, E, U, G, const U&, const G&>* =
                      nullptr>
        constexpr expected(const expected<U, G>& other)
            : base(), ctor_base(detail::non_default_ctor_tag_t{})
        {
            if (other.has_value()) {
                this->construct(*other);
            }
            else {
                this->construct_unexpected(other.error());
            }
        }

        template <
            typename U,
            typename G,
            std::enable_if_t<!(std::is_convertible<U&&, T>::value &&
                               std::is_convertible<G&&, E>::value)>* = nullptr,
            detail::enable_from_other<T, E, U, G, U&&, G&&>* = nullptr>
        explicit constexpr expected(expected<U, G>&& other)
            : base(), ctor_base(detail::non_default_ctor_tag_t{})
        {
            if (other.has_value()) {
                this->construct(std::move(*other));
            }
            else {
                this->construct_unexpected(std::move(other.error()));
            }
        }
        template <
            typename U,
            typename G,
            std::enable_if_t<(std::is_convertible<U&&, T>::value &&
                              std::is_convertible<G&&, E>::value)>* = nullptr,
            detail::enable_from_other<T, E, U, G, U&&, G&&>* = nullptr>
        constexpr expected(expected<U, G>&& other)
            : base(), ctor_base(detail::non_default_ctor_tag_t{})
        {
            if (other.has_value()) {
                this->construct(std::move(*other));
            }
            else {
                this->construct_unexpected(std::move(other.error()));
            }
        }

        template <typename U = value_type,
                  typename = std::enable_if_t<
                      std::is_convertible<U, value_type>::value>>
        expected& operator=(U&& val)
        {
            assign_value(std::forward<U>(val));
            return *this;
        }

        expected& operator=(const unexpected_type& unex)
        {
            assign_unexpected(unex);
            return *this;
        }
        expected& operator=(unexpected_type&& unex)
        {
            assign_unexpected(std::move(unex));
            return *this;
        }

        /// Destroys the contained value, and then initializes the expected
        /// value directly in-place.
        template <typename... Args,
                  typename std::enable_if_t<
                      std::is_constructible<T, Args...>::value>* = nullptr>
        decltype(auto) emplace(Args&&... args)
        {
            emplace_impl(std::forward<Args>(args)...);
            if constexpr (!std::is_void_v<T>) {
                return this->get_value();
            }
        }

        using base::has_value;
        constexpr explicit operator bool() const
        {
            return has_value();
        }

        /// Get the unexpected value, if one is contained in *this
        constexpr error_type& error() &
        {
            SCN_EXPECT(!has_value());
            return this->get_unexpected().error();
        }
        constexpr const error_type& error() const&
        {
            SCN_EXPECT(!has_value());
            return this->get_unexpected().error();
        }
        constexpr error_type&& error() &&
        {
            SCN_EXPECT(!has_value());
            return std::move(this->get_unexpected().error());
        }
        constexpr const error_type&& error() const&&
        {
            SCN_EXPECT(!has_value());
            return std::move(this->get_unexpected().error());
        }

        /// Get the expected value, if one is contained in *this
        template <typename U = T,
                  std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        constexpr U& value() &
        {
            SCN_EXPECT(has_value());
            return this->get_value();
        }
        template <typename U = T,
                  std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        constexpr const U& value() const&
        {
            SCN_EXPECT(has_value());
            return this->get_value();
        }
        template <typename U = T,
                  std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        constexpr U&& value() &&
        {
            SCN_EXPECT(has_value());
            return std::move(this->get_value());
        }
        template <typename U = T,
                  std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        constexpr const U&& value() const&&
        {
            SCN_EXPECT(has_value());
            return std::move(this->get_value());
        }

        /// Get the expected value, if one is contained in *this
        template <typename U = T,
                  std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        constexpr U& operator*() &
        {
            return value();
        }
        template <typename U = T,
                  std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        constexpr const U& operator*() const&
        {
            return value();
        }
        template <typename U = T,
                  std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        constexpr U&& operator*() &&
        {
            return std::move(value());
        }
        template <typename U = T,
                  std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        constexpr const U&& operator*() const&&
        {
            return std::move(value());
        }

        constexpr value_type* operator->()
        {
            return std::addressof(value());
        }
        constexpr const value_type* operator->() const
        {
            return std::addressof(value());
        }

        /// Returns the expected value if *this contains one, otherwise
        /// returns default_value
        template <
            typename U,
            typename = std::enable_if_t<std::is_copy_constructible<T>::value &&
                                        std::is_convertible<U, T>::value>>
        constexpr T value_or(U&& default_value) const&
        {
            if (has_value()) {
                return value();
            }
            return std::forward<U>(default_value);
        }
        template <
            typename U,
            typename = std::enable_if_t<std::is_move_constructible<T>::value &&
                                        std::is_convertible<U, T>::value>>
        constexpr T value_or(U&& default_value) &&
        {
            if (has_value()) {
                return std::move(value());
            }
            return std::forward<U>(default_value);
        }

        template <
            typename G,
            typename = std::enable_if_t<std::is_copy_constructible<E>::value &&
                                        std::is_convertible<G, E>::value>>
        constexpr E error_or(G&& default_error) const&
        {
            if (!has_value()) {
                return error();
            }
            return std::forward<G>(default_error);
        }
        template <
            typename G,
            typename = std::enable_if_t<std::is_move_constructible<E>::value &&
                                        std::is_convertible<G, E>::value>>
        constexpr E error_or(G&& default_error) &&
        {
            if (!has_value()) {
                return std::move(error());
            }
            return std::forward<G>(default_error);
        }

        template <typename F>
        constexpr auto and_then(F&& f) &
        {
            return detail::and_then_impl(*this, std::forward<F>(f));
        }
        template <typename F>
        constexpr auto and_then(F&& f) const&
        {
            return detail::and_then_impl(*this, std::forward<F>(f));
        }
        template <typename F>
        constexpr auto and_then(F&& f) &&
        {
            return detail::and_then_impl(std::move(*this), std::forward<F>(f));
        }
        template <typename F>
        constexpr auto and_then(F&& f) const&&
        {
            return detail::and_then_impl(std::move(*this), std::forward<F>(f));
        }

        template <typename F>
        constexpr auto or_else(F&& f) &
        {
            return detail::or_else_impl(*this, std::forward<F>(f));
        }
        template <typename F>
        constexpr auto or_else(F&& f) const&
        {
            return detail::or_else_impl(*this, std::forward<F>(f));
        }
        template <typename F>
        constexpr auto or_else(F&& f) &&
        {
            return detail::or_else_impl(std::move(*this), std::forward<F>(f));
        }
        template <typename F>
        constexpr auto or_else(F&& f) const&&
        {
            return detail::or_else_impl(std::move(*this), std::forward<F>(f));
        }

        template <typename F>
        constexpr auto transform(F&& f) &
        {
            return detail::transform_impl(*this, std::forward<F>(f));
        }
        template <typename F>
        constexpr auto transform(F&& f) const&
        {
            return detail::transform_impl(*this, std::forward<F>(f));
        }
        template <typename F>
        constexpr auto transform(F&& f) &&
        {
            return detail::transform_impl(std::move(*this), std::forward<F>(f));
        }
        template <typename F>
        constexpr auto transform(F&& f) const&&
        {
            return detail::transform_impl(std::move(*this), std::forward<F>(f));
        }

        template <typename F>
        constexpr auto transform_error(F&& f) &
        {
            return detail::transform_error_impl(*this, std::forward<F>(f));
        }
        template <typename F>
        constexpr auto transform_error(F&& f) const&
        {
            return detail::transform_error_impl(*this, std::forward<F>(f));
        }
        template <typename F>
        constexpr auto transform_error(F&& f) &&
        {
            return detail::transform_error_impl(std::move(*this),
                                                std::forward<F>(f));
        }
        template <typename F>
        constexpr auto transform_error(F&& f) const&&
        {
            return detail::transform_error_impl(std::move(*this),
                                                std::forward<F>(f));
        }

    private:
        template <typename... Args>
        void emplace_impl(Args&&... args)
        {
            if (this->has_value()) {
                this->destroy_value();
                this->construct(std::forward<Args>(args)...);
            }
            else {
#if SCN_HAS_EXCEPTIONS
                if constexpr (std::is_nothrow_constructible_v<T, Args&&...>) {
                    auto tmp = std::move(error());
                    this->destroy_unexpected();

                    try {
                        this->construct(std::forward<Args>(args)...);
                    }
                    catch (...) {
                        this->construct_unexpected(std::move(tmp));
                        throw;
                    }
                }
                else {
                    this->construct(std::forward<Args>(args)...);
                }
#else
                this->destroy_unexpected();
                this->construct(std::forward<Args>(args)...);
#endif
            }
        }

        template <typename Value>
        void assign_value(Value&& val)
        {
            if (has_value()) {
                this->get_value() = std::forward<Value>(val);
                return;
            }

#if SCN_HAS_EXCEPTIONS
            if constexpr (std::is_nothrow_constructible_v<T, Value&&>) {
                this->destroy_unexpected();
                this->construct(std::forward<Value>(val));
            }
            else {
                auto tmp = std::move(this->get_unexpected());
                this->destroy_unexpected();

                try {
                    this->construct(std::forward<Value>(val));
                }
                catch (...) {
                    this->construct_unexpected(std::move(tmp));
                }
            }
#else
            this->destroy_unexpected();
            this->construct(std::forward<Value>(val));
#endif
        }

        template <typename Unexpected>
        void assign_unexpected(Unexpected&& unex)
        {
            if (!has_value()) {
                this->get_unexpected() = std::forward<Unexpected>(unex);
                return;
            }

            this->destroy_value();
            this->construct_unexpected(std::forward<Unexpected>(unex));
        }
    };

    SCN_END_NAMESPACE
}  // namespace scn

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
//
// Ported from the reference implementation of C++23 std::function_ref:
//     https://github.com/zhihaoy/nontype_functional
//
// nontype_functional:
// BSD 2-Clause License
//
// Copyright (c) 2022, Zhihao Yuan
// All rights reserved.

#pragma once

#include <scn/util/meta.h>

#include <functional>
#include <utility>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
namespace fnref_detail {
template <class T>
inline constexpr auto select_param_type = [] {
    if constexpr (std::is_trivially_copyable_v<T>) {
        return detail::type_identity<T>();
    }
    else {
        return std::add_rvalue_reference<T>();
    }
};

template <class T>
using param_t =
    typename std::invoke_result_t<decltype(select_param_type<T>)>::type;

template <typename Sig>
struct qual_fn_sig;

template <typename R, typename... Args>
struct qual_fn_sig<R(Args...)> {
    using function = R(Args...);

    static constexpr bool is_noexcept = false;

    template <typename... T>
    static constexpr bool is_invocable_using =
        std::is_invocable_r_v<R, T..., Args...>;

    template <typename T>
    using cv = T;
};

template <typename R, typename... Args>
struct qual_fn_sig<R(Args...) noexcept> {
    using function = R(Args...);

    static constexpr bool is_noexcept = true;

    template <typename... T>
    static constexpr bool is_invocable_using =
        std::is_nothrow_invocable_r_v<R, T..., Args...>;

    template <typename T>
    using cv = T;
};

template <typename R, typename... Args>
struct qual_fn_sig<R(Args...) const> : qual_fn_sig<R(Args...)> {
    template <typename T>
    using cv = T const;
};

template <typename R, typename... Args>
struct qual_fn_sig<R(Args...) const noexcept>
    : qual_fn_sig<R(Args...) noexcept> {
    template <typename T>
    using cv = T const;
};

struct base {
    union storage {
        constexpr storage() = default;

        template <typename T, std::enable_if_t<std::is_object_v<T>>* = nullptr>
        constexpr explicit storage(T* p) noexcept : m_p(p)
        {
        }

        template <typename T, std::enable_if_t<std::is_object_v<T>>* = nullptr>
        constexpr explicit storage(const T* p) noexcept : m_cp(p)
        {
        }

        template <typename F,
                  std::enable_if_t<std::is_function_v<F>>* = nullptr>
        constexpr explicit storage(F* f) noexcept
            : m_fp(reinterpret_cast<decltype(m_fp)>(f))
        {
        }

        void* m_p{nullptr};
        const void* m_cp;
        void (*m_fp)();
    };

    template <typename T>
    static constexpr auto get(storage s)
    {
        if constexpr (std::is_const_v<T>) {
            return static_cast<T*>(s.m_cp);
        }
        else if constexpr (std::is_object_v<T>) {
            return static_cast<T*>(s.m_p);
        }
        else {
            return reinterpret_cast<T*>(s.m_fp);
        }
    }
};
}  // namespace fnref_detail

template <typename Sig,
          typename = typename fnref_detail::qual_fn_sig<Sig>::function>
class function_ref;

template <typename Sig, typename R, typename... Args>
class function_ref<Sig, R(Args...)> : fnref_detail::base {
    using signature = fnref_detail::qual_fn_sig<Sig>;

    template <typename T>
    using cv = typename signature::template cv<T>;
    template <typename T>
    using cvref = cv<T>&;
    static constexpr bool noex = signature::is_noexcept;

    template <typename... T>
    static constexpr bool is_invocable_using =
        signature::template is_invocable_using<T...>;

    using fwd_t = R(storage, fnref_detail::param_t<Args>...) noexcept(noex);

public:
    template <typename F,
              std::enable_if_t<std::is_function_v<F> &&
                               is_invocable_using<F>>* = nullptr>
    /*implicit*/ function_ref(F* f) noexcept
        : m_fptr([](storage fn,
                    fnref_detail::param_t<Args>... args) noexcept(noex) -> R {
              if constexpr (std::is_void_v<R>) {
                  get<F>(fn)(static_cast<decltype(args)>(args)...);
              }
              else {
                  return get<F>(fn)(static_cast<decltype(args)>(args)...);
              }
          }),
          m_storage(f)
    {
        SCN_EXPECT(f != nullptr);
    }

    template <typename F,
              typename T = std::remove_reference_t<F>,
              std::enable_if_t<detail::is_not_self<F, function_ref> &&
                               !std::is_member_pointer_v<T> &&
                               is_invocable_using<cvref<T>>>* = nullptr>
    /*implicit*/ constexpr function_ref(F&& f) noexcept
        : m_fptr([](storage fn,
                    fnref_detail::param_t<Args>... args) noexcept(noex) -> R {
              cvref<T> obj = *get<T>(fn);
              if constexpr (std::is_void_v<R>) {
                  obj(static_cast<decltype(args)>(args)...);
              }
              else {
                  return obj(static_cast<decltype(args)>(args)...);
              }
          }),
          m_storage(std::addressof(f))
    {
    }

    template <typename T,
              std::enable_if_t<detail::is_not_self<T, function_ref> &&
                               !std::is_pointer_v<T>>* = nullptr>
    function_ref& operator=(T) = delete;

    constexpr R operator()(Args... args) const noexcept(noex)
    {
        return m_fptr(m_storage, SCN_FWD(args)...);
    }

private:
    fwd_t* m_fptr{nullptr};
    storage m_storage;
};

template <typename F, std::enable_if_t<std::is_function_v<F>>* = nullptr>
function_ref(F*) -> function_ref<F>;
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn

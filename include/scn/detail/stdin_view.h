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

#include <scn/detail/ranges.h>

#include <atomic>
#include <mutex>
#include <utility>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        class stdin_view;
        class stdin_iterator;

        class stdin_manager {
        public:
            using iterator = stdin_iterator;
            using sentinel = ranges_std::default_sentinel_t;

            bool require_locking(bool req)
            {
                return m_require_locking.exchange(req);
            }

            void sync_now(stdin_iterator& begin);

            std::mutex& mutex()
            {
                return m_mutex;
            }
            const std::mutex& mutex() const
            {
                return m_mutex;
            }

            stdin_view make_view();

        private:
            constexpr stdin_manager() = default;

            std::optional<char> extract_char() const;

            void auto_sync();

            friend auto& stdin_manager_instance();

            friend class stdin_view;
            friend class stdin_iterator;

            std::mutex m_mutex;
            std::string m_putback_buffer{};
            // std::optional<char> m_latest_read{};
            std::atomic<std::ptrdiff_t> m_end_index{-1};
            std::atomic<bool> m_require_locking{true};
            bool m_never_read{true};
        };

        inline auto& stdin_manager_instance()
        {
            static stdin_manager mng{};
            return mng;
        }

        class stdin_iterator {
        public:
            using value_type = char;
            using reference = char;
            using pointer = char*;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

            stdin_iterator() = default;

            stdin_iterator& operator++()
            {
                SCN_EXPECT(m_parent);
                increment_current();
                return *this;
            }
            stdin_iterator operator++(int)
            {
                auto copy = *this;
                operator++();
                return copy;
            }

            char operator*() const
            {
                SCN_EXPECT(m_parent);
                read_current();
                SCN_EXPECT(m_current_cached);
                return *m_current_cached;
            }

            stdin_manager* manager() const
            {
                return m_parent;
            }

            friend bool operator==(const stdin_iterator& x,
                                   ranges_std::default_sentinel_t)
            {
                return x.is_at_end();
            }
            friend bool operator==(ranges_std::default_sentinel_t s,
                                   const stdin_iterator& x)
            {
                return x == s;
            }

            friend bool operator!=(const stdin_iterator& x,
                                   ranges_std::default_sentinel_t s)
            {
                return !(x == s);
            }
            friend bool operator!=(ranges_std::default_sentinel_t s,
                                   const stdin_iterator& x)
            {
                return !(x == s);
            }

            friend bool operator==(const stdin_iterator& a,
                                   const stdin_iterator& b)
            {
                if (a.is_at_end() == b.is_at_end() && a.is_at_end()) {
                    return true;
                }
                return a.m_current_index == b.m_current_index;
            }

        private:
            friend class stdin_manager;
            friend class stdin_view;

            stdin_iterator(stdin_manager* mgr) : m_parent(mgr) {}

            void read_current() const
            {
                SCN_EXPECT(m_parent);

                if (m_current_cached) {
                    return;
                }

                if (m_current_index < m_parent->m_putback_buffer.size()) {
                    m_current_cached =
                        m_parent->m_putback_buffer[m_current_index];
                    return;
                }

                m_current_cached = m_parent->extract_char();
                if (!m_current_cached) {
                    m_parent->m_end_index = m_current_index;
                }
                else {
                    m_parent->m_putback_buffer.push_back(*m_current_cached);
                }
                m_parent->m_never_read = false;
            }

            void increment_current()
            {
                ++m_current_index;
                m_current_cached.reset();
                read_current();
            }

            bool is_at_end() const
            {
                if (!m_parent) {
                    return true;
                }
                if (m_parent->m_never_read) {
                    read_current();
                }
                if (m_parent->m_end_index < 0) {
                    return false;
                }
                return m_current_index == m_parent->m_end_index;
            }

            stdin_manager* m_parent{nullptr};
            mutable std::ptrdiff_t m_current_index{0};
            mutable std::optional<char> m_current_cached{std::nullopt};
        };

        class stdin_view : public ranges::view_interface<stdin_view> {
        public:
            stdin_view(const stdin_view&) = delete;
            stdin_view& operator=(const stdin_view&) = delete;

            stdin_view(stdin_view&&) = default;
            stdin_view& operator=(stdin_view&&) = default;

            ~stdin_view()
            {
                if (is_this_locked()) {
                    m_manager->auto_sync();
                    release();
                }
            }

            void acquire()
            {
                if (m_manager->m_require_locking) {
                    SCN_EXPECT(!is_this_locked());
                    m_lock.lock();
                }
            }
            SCN_NODISCARD bool try_acquire()
            {
                if (m_manager->m_require_locking) {
                    SCN_EXPECT(!is_this_locked());
                    return m_lock.try_lock();
                }
                return true;
            }
            SCN_NODISCARD bool is_this_locked() const
            {
                if (m_manager->m_require_locking) {
                    return m_lock.owns_lock();
                }
                return true;
            }
            void release()
            {
                if (m_manager->m_require_locking) {
                    SCN_EXPECT(is_this_locked());
                    m_lock.release();
                }
            }

            stdin_manager& manager()
            {
                SCN_EXPECT(is_this_locked());
                return *m_manager;
            }

            auto& get_lock()
            {
                return m_lock;
            }
            const auto& get_lock() const
            {
                return m_lock;
            }

            stdin_iterator begin() const;

            ranges_std::default_sentinel_t end() const
            {
                return {};
            }

        private:
            friend class stdin_iterator;

            friend auto stdin_manager::make_view() -> stdin_view;

            stdin_view(stdin_manager& mg)
                : m_manager(&mg), m_lock(m_manager->m_mutex, std::defer_lock)
            {
            }

            stdin_manager* m_manager;
            std::unique_lock<std::mutex> m_lock;
        };

        inline stdin_view stdin_manager::make_view()
        {
            return {*this};
        }

        inline void stdin_manager::auto_sync()
        {
            stdin_iterator it{this};
            sync_now(it);
        }

        inline auto stdin_view::begin() const -> stdin_iterator
        {
            return {m_manager};
        }

        static_assert(ranges::forward_range<stdin_view>);
        static_assert(ranges_std::forward_iterator<stdin_iterator>);

        struct stdin_subrange
            : public ranges::subrange<ranges::iterator_t<stdin_view>,
                                      ranges_std::default_sentinel_t,
                                      ranges::subrange_kind::unsized> {
            using iterator = ranges::iterator_t<stdin_view>;
            using sentinel = ranges_std::default_sentinel_t;
            using base = ranges::
                subrange<iterator, sentinel, ranges::subrange_kind::unsized>;
            using base::base;

            stdin_subrange(const stdin_view& other)
                : stdin_subrange(other.begin(), other.end())
            {
            }

            stdin_subrange(const base& o) : base(o.begin(), o.end()) {}

            stdin_manager* manager() const
            {
                return begin().manager();
            }
        };
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

#if SCN_STD_RANGES

namespace std::ranges {
    template <>
    inline constexpr bool enable_view<scn::detail::stdin_view> = true;
    template <>
    inline constexpr bool enable_view<scn::detail::stdin_subrange> = true;

    template <>
    inline constexpr bool enable_borrowed_range<scn::detail::stdin_subrange> =
        true;
}  // namespace std::ranges

#else

NANO_BEGIN_NAMESPACE

template <>
inline constexpr bool enable_view<scn::detail::stdin_view> = true;
template <>
inline constexpr bool enable_view<scn::detail::stdin_subrange> = true;

template <>
inline constexpr bool enable_borrowed_range<scn::detail::stdin_subrange> = true;

NANO_END_NAMESPACE

#endif

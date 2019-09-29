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

#ifndef SCN_DETAIL_FILE_H
#define SCN_DETAIL_FILE_H

#include "range.h"

#include <cstdio>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        struct file_handle {
#if SCN_WINDOWS
            using handle_type = void*;
#else
            using handle_type = int;
#endif

            static constexpr file_handle invalid()
            {
#if SCN_WINDOWS
                return {nullptr};
#else
                return {-1};
#endif
            }

            handle_type handle;
        };

        class byte_mapped_file {
        public:
            using iterator = const char*;
            using sentinel = const char*;

            byte_mapped_file() = default;
            byte_mapped_file(const char* filename);

            byte_mapped_file(const byte_mapped_file&) = delete;
            byte_mapped_file& operator=(const byte_mapped_file&) = delete;

            byte_mapped_file(byte_mapped_file&& o) noexcept
                : m_file(o.m_file), m_begin(o.m_begin), m_end(o.m_end)
            {
                o.m_file = file_handle::invalid();
                o.m_begin = nullptr;
                o.m_end = nullptr;

                SCN_ENSURE(!o.valid());
                SCN_ENSURE(valid());
            }
            byte_mapped_file& operator=(byte_mapped_file&& o) noexcept
            {
                if (valid()) {
                    _destruct();
                }
                m_file = o.m_file;
                m_begin = o.m_begin;
                m_end = o.m_end;

                o.m_file = file_handle::invalid();
                o.m_begin = nullptr;
                o.m_end = nullptr;

                SCN_ENSURE(!o.valid());
                SCN_ENSURE(valid());
                return *this;
            }

            ~byte_mapped_file()
            {
                if (valid()) {
                    _destruct();
                }
            }

            bool valid() const
            {
                return m_file.handle != file_handle::invalid().handle;
            }

            iterator begin() const
            {
                return m_begin;
            }
            sentinel end() const
            {
                return m_end;
            }

        private:
            void _destruct();

            file_handle m_file{file_handle::invalid().handle};
            char* m_begin{nullptr};
            char* m_end{nullptr};
        };
    }  // namespace detail

    template <typename CharT>
    class basic_mapped_file : public detail::byte_mapped_file {
    public:
        using iterator = const CharT*;
        using sentinel = const CharT*;

        using byte_mapped_file::byte_mapped_file;

        // embrace the UB
        iterator begin() const
        {
            return reinterpret_cast<iterator>(byte_mapped_file::begin());
        }
        sentinel end() const
        {
            return reinterpret_cast<sentinel>(byte_mapped_file::end());
        }
    };

    using mapped_file = basic_mapped_file<char>;
    using wmapped_file = basic_mapped_file<wchar_t>;

    template <typename CharT>
    class basic_file;

    namespace detail {
        template <typename CharT>
        class cfile_iterator {
        public:
            using char_type = CharT;
            using value_type = expected<CharT>;
            using reference = value_type&;
            using pointer = value_type*;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::input_iterator_tag;

            cfile_iterator() = default;
            cfile_iterator(basic_file<CharT>* f) : m_file(f) {}

            expected<CharT> operator*() const;
            const cfile_iterator& operator++() const
            {
                return *this;
            }

            bool valid() const
            {
                return m_file != nullptr;
            }

            bool operator==(const cfile_iterator& o) const
            {
                return m_file == o.m_file;
            }
            bool operator!=(const cfile_iterator& o) const
            {
                return !operator==(o);
            }

            const basic_file<CharT>& file() const
            {
                return *m_file;
            }

        private:
            basic_file<CharT>* m_file{nullptr};
        };
    }  // namespace detail

    template <typename CharT>
    class basic_file {
    public:
        using underlying_iterator = detail::cfile_iterator<CharT>;
        using iterator = backtracking_iterator<underlying_iterator>;
        using sentinel = underlying_iterator;

        basic_file(FILE* f) : m_file(f), m_it(this) {}

        basic_file(const basic_file&) = delete;
        basic_file& operator=(const basic_file&) = delete;

        basic_file(basic_file&&) = default;
        basic_file& operator=(basic_file&&) = default;

        ~basic_file()
        {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            sync();
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }

        const iterator& begin() const
        {
            return m_it;
        }

        sentinel end() const
        {
            return {};
        }

        FILE* file() const
        {
            return m_file;
        }

        bool sync() const;

    private:
        FILE* m_file;
        mutable iterator m_it;
    };

    using file = basic_file<char>;
    using wfile = basic_file<wchar_t>;

    namespace detail {
        template <typename CharT>
        class basic_file_view_iterator {
        public:
            using file_type = basic_file<CharT>;
            using underlying_iterator = typename file_type::iterator;
            using value_type = ranges::iter_value_t<underlying_iterator>;
            using reference = ranges::iter_reference_t<underlying_iterator>;
            using pointer = typename underlying_iterator::pointer;
            using difference_type =
                ranges::iter_difference_t<underlying_iterator>;
            using iterator_category =
                ranges::iterator_category_t<underlying_iterator>;

            basic_file_view_iterator() = default;
            basic_file_view_iterator(const underlying_iterator& it)
                // eww
                : m_it(std::addressof(const_cast<underlying_iterator&>(it)))
            {
            }

            value_type operator*() const
            {
                return m_it->operator*();
            }
            const basic_file_view_iterator& operator++() const
            {
                m_it->operator++();
                return *this;
            }
            const basic_file_view_iterator& operator--() const
            {
                m_it->operator--();
                return *this;
            }

            template <typename F>
            bool sync(F&& s) const
            {
                return m_it->sync(std::forward<F>(s));
            }

            const underlying_iterator& base() const
            {
                SCN_EXPECT(m_it != nullptr);
                return *m_it;
            }

            bool operator==(const basic_file_view_iterator& o) const
            {
                return o.m_it == m_it;
            }
            bool operator!=(const basic_file_view_iterator& o) const
            {
                return !operator==(o);
            }

        private:
            // const correctness? never heard of it
            mutable underlying_iterator* m_it{nullptr};
        };

        template <typename CharT, typename Sentinel>
        bool operator==(const basic_file_view_iterator<CharT>& l,
                        const Sentinel& r)
        {
            return l.base() == r;
        }
        template <typename CharT, typename Sentinel>
        bool operator!=(const basic_file_view_iterator<CharT>& l,
                        const Sentinel& r)
        {
            return !(l == r);
        }

        template <typename CharT, typename Sentinel>
        bool operator==(const Sentinel& l,
                        const basic_file_view_iterator<CharT>& r)
        {
            return l == r.base();
        }
        template <typename CharT, typename Sentinel>
        bool operator!=(const Sentinel& l,
                        const basic_file_view_iterator<CharT>& r)
        {
            return !(l == r);
        }

        template <typename CharT>
        class basic_file_view {
        public:
            using file_type = basic_file<CharT>;
            using iterator = basic_file_view_iterator<CharT>;
            using sentinel = typename file_type::sentinel;

            basic_file_view() = default;
            basic_file_view(const file_type& f) : m_file(std::addressof(f)) {}
            basic_file_view(iterator i, sentinel)
                : m_file(std::addressof(i.base().base().file()))
            {
            }

            iterator begin() const
            {
                SCN_EXPECT(*this);
                return {m_file->begin()};
            }
            sentinel end() const
            {
                SCN_EXPECT(*this);
                return m_file->end();
            }

            bool sync() const
            {
                SCN_EXPECT(*this);
                return begin().base().base().file().sync();
            }

            FILE* file()
            {
                SCN_EXPECT(*this);
                return m_file->file();
            }

            const file_type& get() const
            {
                SCN_EXPECT(*this);
                return *m_file;
            }

            operator bool() const
            {
                return m_file != nullptr;
            }

        private:
            const file_type* m_file{nullptr};
        };
    }  // namespace detail

    template <typename CharT>
    detail::view_range_wrapper<detail::basic_file_view<CharT>>
    make_range_wrapper(const basic_file<CharT>& f)
    {
        return {detail::basic_file_view<CharT>{f}};
    }

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wexit-time-destructors")
    template <typename CharT>
    auto stdin_range()
        -> decltype(wrap(std::declval<const basic_file<CharT>&>()))&
    {
        static auto file = basic_file<CharT>{stdin};
        static auto wrapped = wrap(file);
        return wrapped;
    }
    inline auto cstdin() -> decltype(wrap(std::declval<const file&>()))&
    {
        return stdin_range<char>();
    }
    inline auto wcstdin() -> decltype(wrap(std::declval<const wfile&>()))&
    {
        return stdin_range<wchar_t>();
    }
    SCN_CLANG_POP

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_FILE_CPP)
#include "file.cpp"
#endif

#endif  // SCN_DETAIL_FILE_H

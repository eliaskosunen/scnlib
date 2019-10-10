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
#include <string>

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
            cfile_iterator(const basic_file<CharT>* f) : m_file(f) {}

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
            const basic_file<CharT>* m_file{nullptr};
        };

        template <typename CharT>
        struct cfile_iterator_cache {
            using char_type = CharT;
            using traits = std::char_traits<CharT>;
            using int_type = typename traits::int_type;

            template <typename F>
            bool sync(F sync_fn)
            {
                if (n == 0) {
                    return true;
                }
                auto s = span<char_type>(std::addressof(*(buffer.end() - n)),
                                         &buffer[0] + buffer.size());
                if (sync_fn(s)) {
                    buffer.clear();
                    n = 0;
                    return true;
                }
                return false;
            }

            std::basic_string<char_type> buffer{};
            std::ptrdiff_t n{0};
            int_type latest{traits::eof()};
            error err{};
        };

        template <typename CharT>
        class caching_cfile_iterator {
        public:
            using char_type = CharT;
            using underlying_iterator = cfile_iterator<CharT>;
            using cache_type = cfile_iterator_cache<CharT>;
            using traits = std::char_traits<char_type>;

            using value_type = ranges::iter_value_t<underlying_iterator>;
            using reference = ranges::iter_reference_t<underlying_iterator>;
            using pointer = value_type*;
            using difference_type =
                ranges::iter_difference_t<underlying_iterator>;
            using iterator_category = std::bidirectional_iterator_tag;

            caching_cfile_iterator() = default;
            caching_cfile_iterator(underlying_iterator it, cache_type& c)
                : m_it(std::move(it)), m_cache(std::addressof(c))
            {
            }

            underlying_iterator base()
            {
                return m_it;
            }
            cache_type* cache()
            {
                return m_cache;
            }

            expected<char_type> operator*()
            {
                SCN_EXPECT(m_cache != nullptr);
                if (m_cache->n > 0) {
                    return {*(m_cache->buffer.end() - m_cache->n)};
                }
                if (m_cache->err) {
                    if (m_cache->latest == traits::eof()) {
                        return _read_next();
                    }
                    return traits::to_char_type(m_cache->latest);
                }
                return m_cache->err;
            }
            caching_cfile_iterator& operator++()
            {
                SCN_EXPECT(m_cache != nullptr);
                if (m_cache->n > 0) {
                    --m_cache->n;
                }
                else {
                    _read_next();
                }
                return *this;
            }
            caching_cfile_iterator& operator--() noexcept
            {
                SCN_EXPECT(m_cache != nullptr);
                ++m_cache->n;
                return *this;
            }

            bool operator==(const caching_cfile_iterator& o) const
            {
                if (m_it == o.m_it) {
                    if (!m_cache) {
                        return true;
                    }
                    return m_cache->n == o.m_cache->n;
                }
                return false;
            }
            bool operator!=(const caching_cfile_iterator& o) const
            {
                return !operator==(o);
            }

            bool operator==(const cfile_iterator<CharT>& o) const
            {
                if (!m_cache || m_cache->n == 0) {
                    return m_it == o;
                }
                return false;
            }
            bool operator!=(const cfile_iterator<CharT>& o) const
            {
                return !operator==(o);
            }

        private:
            expected<char_type> _read_next()
            {
                SCN_EXPECT(m_cache != nullptr);
                if (m_cache->err && m_cache->latest != traits::eof()) {
                    m_cache->buffer.push_back(
                        traits::to_char_type(m_cache->latest));
                }
                if (!m_cache->err) {
                    return m_cache->err;
                }
                ++m_it;
                SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                auto next = wrap_deref(*m_it);
                SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                if (next) {
                    m_cache->latest = traits::to_int_type(next.value());
                }
                else {
                    m_cache->err = next.error();
                }
                return next;
            }

            underlying_iterator m_it{};
            cache_type* m_cache{nullptr};
        };
    }  // namespace detail

    template <typename CharT>
    class basic_file_view;

    template <typename CharT>
    class basic_file {
    public:
        using iterator = detail::caching_cfile_iterator<CharT>;
        using underlying_iterator = detail::cfile_iterator<CharT>;
        using sentinel = underlying_iterator;
        using cache_type = detail::cfile_iterator_cache<CharT>;

        basic_file(FILE* f) : m_file(f), m_cache{} {}

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

        iterator begin() const noexcept
        {
            auto uit = underlying_iterator{this};
            return {std::move(uit), m_cache};
        }

        sentinel end() const noexcept
        {
            return {};
        }

        FILE* file() const noexcept
        {
            return m_file;
        }

        cache_type& cache() const noexcept
        {
            return m_cache;
        }

        bool sync() const;

        basic_file_view<CharT> make_view() const;
        detail::range_wrapper<basic_file_view<CharT>> wrap() const
        {
            return {make_view()};
        }

    private:
        FILE* m_file;
        mutable cache_type m_cache;
    };

    namespace detail {
        template <typename CharT>
        struct is_caching_range_impl<basic_file<CharT>> : std::true_type {
        };
    }  // namespace detail

    using file = basic_file<char>;
    using wfile = basic_file<wchar_t>;

    template <typename CharT>
    class basic_file_view : public detail::ranges::view_base {
    public:
        using file_type = basic_file<CharT>;
        using iterator = typename file_type::iterator;
        using sentinel = typename file_type::sentinel;

        basic_file_view() = default;
        basic_file_view(const file_type& f) : m_file(std::addressof(f)) {}
        basic_file_view(iterator i, sentinel)
            : m_file(std::addressof(i.base().file()))
        {
        }

        iterator begin() const noexcept
        {
            SCN_EXPECT(*this);
            return {m_file->begin()};
        }
        sentinel end() const noexcept
        {
            SCN_EXPECT(*this);
            return m_file->end();
        }

        bool sync() const
        {
            SCN_EXPECT(*this);
            return begin().base().base().file().sync();
        }

        FILE* file() const noexcept
        {
            SCN_EXPECT(*this);
            return m_file->file();
        }

        const file_type& get() const
        {
            SCN_EXPECT(*this);
            return *m_file;
        }

        explicit operator bool() const
        {
            return m_file != nullptr;
        }

    private:
        const file_type* m_file{nullptr};
    };

    namespace detail {
        template <typename CharT>
        struct is_caching_range_impl<basic_file_view<CharT>> : std::true_type {
        };
    }  // namespace detail

    using file_view = basic_file_view<char>;
    using wfile_view = basic_file_view<wchar_t>;

    template <typename CharT>
    basic_file_view<CharT> basic_file<CharT>::make_view() const
    {
        return {*this};
    }

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wexit-time-destructors")
    template <typename CharT>
    basic_file_view<CharT>& stdin_range()
    {
        static auto f = basic_file<CharT>{stdin};
        static auto view = basic_file_view<CharT>(f);
        return view;
    }
    inline file_view& cstdin()
    {
        return stdin_range<char>();
    }
    inline wfile_view& wcstdin()
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

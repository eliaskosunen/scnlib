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

#include <scn/detail/scan_buffer.h>

#include <scn/detail/ranges.h>

#include <cstdio>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
namespace {
struct file_wrapper_impl_base {
private:
    static auto fgetc_impl(std::FILE* file)
    {
#if SCN_POSIX
        return getc_unlocked(file);
#elif SCN_WINDOWS
        return _fgetc_nolock(file);
#else
        return std::fgetc(file);
#endif
    }

    static auto ungetc_impl(std::FILE* file, int ch)
    {
#if SCN_WINDOWS && !SCN_MINGW
        return _ungetc_nolock(ch, file);
#else
        return std::ungetc(ch, file);
#endif
    }

public:
    static void lock(std::FILE* file)
    {
#if SCN_POSIX
        ::flockfile(file);
#elif SCN_WINDOWS
        ::_lock_file(file);
#else
        SCN_UNUSED(file);
#endif
    }
    static void unlock(std::FILE* file)
    {
#if SCN_POSIX
        ::funlockfile(file);
#elif SCN_WINDOWS
        ::_unlock_file(file);
#else
        SCN_UNUSED(file);
#endif
    }

    static void lock_for_unget(std::FILE* file)
    {
#if SCN_WINDOWS && !SCN_MINGW
        SCN_UNUSED(file);
#else
        lock(file);
#endif
    }

    static void unlock_for_unget(std::FILE* file)
    {
#if SCN_WINDOWS && !SCN_MINGW
        SCN_UNUSED(file);
#else
        unlock(file);
#endif
    }

    static std::optional<char> read(std::FILE* file)
    {
        auto res = fgetc_impl(file);
        if (res == EOF) {
            return std::nullopt;
        }
        return static_cast<char>(res);
    }

    static void unget(std::FILE* file, char ch)
    {
        auto res = ungetc_impl(file, static_cast<unsigned char>(ch));
        SCN_ENSURE(res != EOF);
    }

    static std::optional<char> peek(std::FILE* file)
    {
        if (auto res = read(file); res) {
            unget(file, *res);
            return res;
        }
        return std::nullopt;
    }
};

template <typename F, typename = void>
struct file_wrapper_impl : file_wrapper_impl_base {
    constexpr static std::string_view get_current_buffer(F*)
    {
        return {};
    }

    constexpr static bool has_buffering()
    {
        return false;
    }

    static bool fill_buffer(F*)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    constexpr static void unsafe_advance_to_buffer_end(F*) {}

    static void unsafe_advance_n(F*, std::ptrdiff_t)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
};

// GNU libc (Linux)
template <typename F>
struct file_wrapper_impl<F, std::enable_if_t<sizeof(F::_IO_read_ptr) != 0>>
    : file_wrapper_impl_base {
    static std::string_view get_current_buffer(F* file)
    {
        return make_string_view_from_pointers(file->_IO_read_ptr,
                                              file->_IO_read_end);
    }

    constexpr static bool has_buffering()
    {
        return true;
    }

    static bool fill_buffer(F* file)
    {
        return peek(file).has_value();
    }

    static void unsafe_advance_to_buffer_end(F* file)
    {
        SCN_EXPECT(file->_IO_read_ptr && file->_IO_read_end);
        file->_IO_read_ptr = file->_IO_read_end;
    }

    static void unsafe_advance_n(F* file, std::ptrdiff_t n)
    {
        SCN_EXPECT(file->_IO_read_ptr);
        SCN_EXPECT(file->_IO_read_end - file->_IO_read_ptr >= n);
        file->_IO_read_ptr += n;
    }

    static std::optional<char> peek(F* file)
    {
        if (file->_IO_read_ptr != file->_IO_read_end) {
            return file->_IO_read_ptr[0];
        }
        if (auto res = read(file); res) {
            --file->_IO_read_ptr;
            return res;
        }
        return std::nullopt;
    }
};

// BSD libc (Apple)
template <typename F>
struct file_wrapper_impl<F, std::enable_if_t<sizeof(F::_p) != 0>>
    : file_wrapper_impl_base {
    static std::string_view get_current_buffer(F* file)
    {
        return {reinterpret_cast<const char*>(file->_p),
                static_cast<std::size_t>(file->_r)};
    }

    constexpr static bool has_buffering()
    {
        return true;
    }

    static bool fill_buffer(F* file)
    {
        return peek(file).has_value();
    }

    static void unsafe_advance_to_buffer_end(F* file)
    {
        SCN_EXPECT(file->_p != nullptr);
        file->_p += file->_r;
        file->_r = 0;
    }

    static void unsafe_advance_n(F* file, std::ptrdiff_t n)
    {
        SCN_EXPECT(file->_p != nullptr);
        SCN_EXPECT(file->_r >= n);
        file->_p += n;
        file->_r -= n;
    }

    static std::optional<char> peek(F* file)
    {
        if (file->_p != nullptr && file->_r != 0) {
            return static_cast<char>(*file->_p);
        }
        if (auto res = read(file); res) {
            --file->_p;
            ++file->_r;
            return res;
        }
        return std::nullopt;
    }
};

using file_wrapper = file_wrapper_impl<std::FILE>;

bool fill_with_buffering(std::FILE* file, std::string_view& current_view)
{
    SCN_EXPECT(file_wrapper::has_buffering());

    if (!current_view.empty()) {
        file_wrapper::unsafe_advance_to_buffer_end(file);
    }

    if (!file_wrapper::fill_buffer(file)) {
        current_view = {};
        return false;
    }

    current_view = file_wrapper::get_current_buffer(file);
    return true;
}

bool fill_without_buffering(std::FILE* file,
                            std::string_view& current_view,
                            std::optional<char>& latest)
{
    latest = file_wrapper::read(file);
    if (!latest) {
        current_view = {};
        return false;
    }
    current_view = {&*latest, 1};
    return true;
}
}  // namespace

scan_file_buffer::scan_file_buffer(std::FILE* file)
    : base(base::non_contiguous_tag{}), m_file(file)
{
    file_wrapper::lock(file);
}

scan_file_buffer::~scan_file_buffer()
{
    file_wrapper::unlock(m_file);
}

bool scan_file_buffer::fill()
{
    SCN_EXPECT(m_file);

    if (!this->m_current_view.empty()) {
        this->m_putback_buffer.insert(this->m_putback_buffer.end(),
                                      this->m_current_view.begin(),
                                      this->m_current_view.end());
    }

    if (file_wrapper::has_buffering()) {
        return fill_with_buffering(m_file, this->m_current_view);
    }

    return fill_without_buffering(m_file, this->m_current_view, this->m_latest);
}

namespace {
struct file_unlocker_for_unget {
    file_unlocker_for_unget(std::FILE* f) : file(f)
    {
        file_wrapper::unlock_for_unget(file);
    }
    ~file_unlocker_for_unget()
    {
        file_wrapper::lock_for_unget(file);
    }

    std::FILE* file;
};
}  // namespace

void scan_file_buffer::sync(std::ptrdiff_t position)
{
    SCN_EXPECT(m_file);

    if (file_wrapper::has_buffering()) {
        if (position <
            static_cast<std::ptrdiff_t>(this->putback_buffer().size())) {
            file_unlocker_for_unget unlocker{m_file};
            auto putback_segment = this->get_segment_starting_at(position);
            for (auto ch : ranges::views::reverse(putback_segment)) {
                file_wrapper::unget(m_file, ch);
            }
            return;
        }

        file_wrapper::unsafe_advance_n(
            m_file, position - static_cast<std::ptrdiff_t>(
                                   this->putback_buffer().size()));
        return;
    }

    const auto chars_avail = this->chars_available();
    if (position == chars_avail) {
        return;
    }

    file_unlocker_for_unget unlocker{m_file};
    SCN_EXPECT(m_current_view.size() == 1);
    file_wrapper::unget(m_file, m_current_view.front());

    auto putback_segment =
        std::string_view{this->putback_buffer()}.substr(position);
    for (auto ch : ranges::views::reverse(putback_segment)) {
        file_wrapper::unget(m_file, ch);
    }
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn

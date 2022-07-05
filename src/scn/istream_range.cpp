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

#include <scn/detail/istream_range.h>

#if SCN_USE_IOSTREAMS

#include <streambuf>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT>
        bool basic_input_istreambuf_view<CharT>::read_next_char() const
        {
            if (!traits_type::eq_int_type(m_last_char, traits_type::eof())) {
                return true;
            }
            m_last_char = m_streambuf->sbumpc();
            m_end_reached =
                traits_type::eq_int_type(m_last_char, traits_type::eof());
            return !m_end_reached;
        }

        template <typename CharT>
        bool basic_input_istreambuf_view<CharT>::iterator::is_at_end() const
        {
            if (!m_view) {
                return true;
            }

            if (m_view->m_end_reached) {
                return true;
            }

            return !m_view->read_next_char();
        }

        template bool basic_input_istreambuf_view<char>::read_next_char() const;
        template bool basic_input_istreambuf_view<wchar_t>::read_next_char()
            const;

        template bool basic_input_istreambuf_view<char>::iterator::is_at_end()
            const;
        template bool
        basic_input_istreambuf_view<wchar_t>::iterator::is_at_end() const;
    }  // namespace detail

    template <typename CharT>
    void basic_istreambuf_view<CharT>::sync(iterator it)
    {
        if (it.index() == this->m_iterator_offset) {
            return;
        }

        for (; this->m_iterator_offset != it.index();
             --this->m_iterator_offset) {
            auto result = this->m_iterator.view().rdbuf()->sputbackc(
                this->get_cached_at_index(this->m_iterator_offset - 1));
            SCN_ENSURE(!traits_type::eq_int_type(result, traits_type::eof()));
        }
        this->m_buffer.erase(static_cast<std::size_t>(this->m_iterator_offset));
    }

    template <typename CharT>
    void basic_istreambuf_subrange<CharT>::sync(iterator it)
    {
        static_cast<basic_istreambuf_view<CharT>&>(this->begin().view())
            .sync(it);
    }

    template void basic_istreambuf_view<char>::sync(iterator);
    template void basic_istreambuf_view<wchar_t>::sync(iterator);

    template void basic_istreambuf_subrange<char>::sync(iterator);
    template void basic_istreambuf_subrange<wchar_t>::sync(iterator);

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_USE_IOSTREAMS

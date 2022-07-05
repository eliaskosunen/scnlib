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

#include <scn/detail/istream_scanner.h>

#if SCN_USE_IOSTREAMS

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename SourceRange>
        auto range_streambuf<SourceRange>::underflow() -> int_type
        {
            // Already read
            if (!traits_type::eq_int_type(m_ch, traits_type::eof())) {
                return m_ch;
            }

            if (m_begin == ranges::end(m_range)) {
                return traits_type::eof();
            }
            m_begin_prev = m_begin;
            m_ch = traits_type::to_int_type(*m_begin++);
            return m_ch;
        }

        template <typename SourceRange>
        auto range_streambuf<SourceRange>::uflow() -> int_type
        {
            auto ret = underflow();
            if (ret != traits_type::eof()) {
                m_ch = traits_type::eof();
            }
            return ret;
        }

        template <typename SourceRange>
        auto range_streambuf<SourceRange>::showmanyc() -> std::streamsize
        {
            return traits_type::eq_int_type(m_ch, traits_type::eof()) ? 0 : 1;
        }

        template <typename SourceRange>
        auto range_streambuf<SourceRange>::pbackfail(int_type c) -> int_type
        {
            SCN_EXPECT(traits_type::eq_int_type(c, traits_type::eof()));
            SCN_EXPECT(!m_has_put_back);
            m_has_put_back = true;

            m_begin = m_begin_prev;

            if (m_begin == ranges::end(m_range)) {
                return traits_type::eof();
            }
            return traits_type::to_int_type(0);
        }

#define SCN_DEFINE_RANGE_STREAMBUF(Range)                               \
    template auto range_streambuf<Range>::underflow()->int_type;        \
    template auto range_streambuf<Range>::uflow()->int_type;            \
    template auto range_streambuf<Range>::showmanyc()->std::streamsize; \
    template auto range_streambuf<Range>::pbackfail(int_type)->int_type;

        SCN_DEFINE_RANGE_STREAMBUF(scanner_scan_contexts::sv::subrange_type)
        SCN_DEFINE_RANGE_STREAMBUF(scanner_scan_contexts::wsv::subrange_type)
        SCN_DEFINE_RANGE_STREAMBUF(istreambuf_subrange)
        SCN_DEFINE_RANGE_STREAMBUF(wistreambuf_subrange)
        SCN_DEFINE_RANGE_STREAMBUF(erased_subrange)
        SCN_DEFINE_RANGE_STREAMBUF(werased_subrange)

#undef SCN_DEFINE_RANGE_STREAMBUF
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_USE_IOSTREAMS

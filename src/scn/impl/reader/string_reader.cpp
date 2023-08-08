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

#include <scn/impl/reader/string_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace ascii_charset_specifiers {
            using detail::character_set_specifier;

            static constexpr std::array<character_set_specifier, 128> table = {{
                character_set_specifier::cntrl,  // 0x00: '\0' (NUL)
                character_set_specifier::cntrl,  // 0x01: SOH
                character_set_specifier::cntrl,  // 0x02: STX
                character_set_specifier::cntrl,  // 0x03: ETX
                character_set_specifier::cntrl,  // 0x04: EOT
                character_set_specifier::cntrl,  // 0x05: ENQ
                character_set_specifier::cntrl,  // 0x06: ACK
                character_set_specifier::cntrl,  // 0x07: '\a' (BEL)
                character_set_specifier::cntrl,  // 0x08: '\b' (BS)
                character_set_specifier::cntrl |
                    character_set_specifier::space |
                    character_set_specifier::blank,  // 0x09: '\t' (HT)
                character_set_specifier::cntrl |
                    character_set_specifier::space,  // 0x0a: '\n' (LF)
                character_set_specifier::cntrl |
                    character_set_specifier::space,  // 0x0b: '\v' (VT)
                character_set_specifier::cntrl |
                    character_set_specifier::space,  // 0x0c: '\f' (FF)
                character_set_specifier::cntrl |
                    character_set_specifier::space,  // 0x0d: '\r' (CR)
                character_set_specifier::cntrl,      // 0x0e: SO
                character_set_specifier::cntrl,      // 0x0f: SI
                character_set_specifier::cntrl,      // 0x10: DLE
                character_set_specifier::cntrl,      // 0x11: DC1
                character_set_specifier::cntrl,      // 0x12: DC2
                character_set_specifier::cntrl,      // 0x13: DC3
                character_set_specifier::cntrl,      // 0x14: DC4
                character_set_specifier::cntrl,      // 0x15: NAK
                character_set_specifier::cntrl,      // 0x16: SYN
                character_set_specifier::cntrl,      // 0x17: ETB
                character_set_specifier::cntrl,      // 0x18: CAN
                character_set_specifier::cntrl,      // 0x19: EM
                character_set_specifier::cntrl,      // 0x1a: SUB
                character_set_specifier::cntrl,      // 0x1b: ESC
                character_set_specifier::cntrl,      // 0x1c: FS
                character_set_specifier::cntrl,      // 0x1d: GS
                character_set_specifier::cntrl,      // 0x1e: RS
                character_set_specifier::cntrl,      // 0x1f: US
                character_set_specifier::space_literal |
                    character_set_specifier::space |
                    character_set_specifier::blank,  // 0x20: ' ' (SPACE)
                character_set_specifier::punct,      // 0x21: '!'
                character_set_specifier::punct,      // 0x22: '"'
                character_set_specifier::punct,      // 0x23: '#'
                character_set_specifier::punct,      // 0x24: '$'
                character_set_specifier::punct,      // 0x25: '%'
                character_set_specifier::punct,      // 0x26: '&'
                character_set_specifier::punct,      // 0x27: '\''
                character_set_specifier::punct,      // 0x28: '('
                character_set_specifier::punct,      // 0x29: ')'
                character_set_specifier::punct,      // 0x2a: '*'
                character_set_specifier::punct,      // 0x2b: '+'
                character_set_specifier::punct,      // 0x2c: ','
                character_set_specifier::punct,      // 0x2d: '-'
                character_set_specifier::punct,      // 0x2e: '.'
                character_set_specifier::punct,      // 0x2f: '/'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x30: '0'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x31: '1'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x32: '2'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x33: '3'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x34: '4'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x35: '5'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x36: '6'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x37: '7'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x38: '8'
                character_set_specifier::digit |
                    character_set_specifier::xdigit,  // 0x39: '9'
                character_set_specifier::punct,       // 0x3a: ':'
                character_set_specifier::punct,       // 0x3b: ';'
                character_set_specifier::punct,       // 0x3c: '<'
                character_set_specifier::punct,       // 0x3d: '='
                character_set_specifier::punct,       // 0x3e: '?'
                character_set_specifier::punct,       // 0x3f: '?'
                character_set_specifier::punct,       // 0x40: '@'
                character_set_specifier::alpha |
                    character_set_specifier::upper |
                    character_set_specifier::xdigit,  // 0x41: 'A'
                character_set_specifier::alpha |
                    character_set_specifier::upper |
                    character_set_specifier::xdigit,  // 0x42: 'B'
                character_set_specifier::alpha |
                    character_set_specifier::upper |
                    character_set_specifier::xdigit,  // 0x43: 'C'
                character_set_specifier::alpha |
                    character_set_specifier::upper |
                    character_set_specifier::xdigit,  // 0x44: 'D'
                character_set_specifier::alpha |
                    character_set_specifier::upper |
                    character_set_specifier::xdigit,  // 0x45: 'E'
                character_set_specifier::alpha |
                    character_set_specifier::upper |
                    character_set_specifier::xdigit,  // 0x46: 'F'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x47: 'G'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x48: 'H'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x49: 'I'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x4a: 'J'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x4b: 'K'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x4c: 'L'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x4d: 'M'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x4e: 'N'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x4f: 'O'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x50: 'P'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x51: 'Q'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x52: 'R'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x53: 'S'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x54: 'T'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x55: 'U'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x56: 'V'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x57: 'W'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x58: 'X'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x59: 'Y'
                character_set_specifier::alpha |
                    character_set_specifier::upper,  // 0x5a: 'Z'
                character_set_specifier::punct,      // 0x5b: '['
                character_set_specifier::punct,      // 0x5c: '\\'
                character_set_specifier::punct,      // 0x5d: ']'
                character_set_specifier::punct,      // 0x5e: '^'
                character_set_specifier::punct |
                    character_set_specifier::underscore_literal,  // 0x5f: '_'
                character_set_specifier::punct,                   // 0x60: '`'
                character_set_specifier::alpha |
                    character_set_specifier::lower |
                    character_set_specifier::xdigit,  // 0x61: 'a'
                character_set_specifier::alpha |
                    character_set_specifier::lower |
                    character_set_specifier::xdigit,  // 0x62: 'b'
                character_set_specifier::alpha |
                    character_set_specifier::lower |
                    character_set_specifier::xdigit,  // 0x63: 'c'
                character_set_specifier::alpha |
                    character_set_specifier::lower |
                    character_set_specifier::xdigit,  // 0x64: 'd'
                character_set_specifier::alpha |
                    character_set_specifier::lower |
                    character_set_specifier::xdigit,  // 0x65: 'e'
                character_set_specifier::alpha |
                    character_set_specifier::lower |
                    character_set_specifier::xdigit,  // 0x66: 'f'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x67: 'g'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x68: 'h'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x69: 'i'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x6a: 'j'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x6b: 'k'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x6c: 'l'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x6d: 'm'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x6e: 'n'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x6f: 'o'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x70: 'p'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x71: 'q'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x72: 'r'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x73: 's'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x74: 't'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x75: 'u'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x76: 'v'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x77: 'w'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x78: 'x'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x79: 'y'
                character_set_specifier::alpha |
                    character_set_specifier::lower,  // 0x7a: 'z'
                character_set_specifier::punct,      // 0x7b: '{'
                character_set_specifier::punct,      // 0x7c: '|'
                character_set_specifier::punct,      // 0x7d: '}'
                character_set_specifier::punct,      // 0x7e: '~'
                character_set_specifier::cntrl,      // 0x7f: DEL
            }};
        }  // namespace ascii_charset_specifiers

        detail::character_set_specifier get_charset_specifier_for_ascii(char ch)
        {
            SCN_EXPECT(is_ascii_char(ch));
            return ascii_charset_specifiers::table[static_cast<size_t>(
                static_cast<unsigned char>(ch))];
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn

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

#include <scn/istream.h>

#include <chrono>

#include "fuzz.h"

#include <fstream>

namespace {

using namespace scn::fuzz;

constexpr const char* path = "file_fuzz_tmp_file";

struct cfile_guard {
    cfile_guard()
    {
        file = std::fopen(path, "rb");
        SCN_ENSURE(file);
    }

    ~cfile_guard()
    {
        std::fclose(file);
    }

    std::FILE* file;
};

template <typename Source>
std::string do_run_line_by_line(Source& source,
                                std::string_view expected,
                                bool is_valid_unicode)
{
    const auto start = std::chrono::steady_clock::now();
    std::string buffer{};
    while (true) {
        {
            auto res = scn::scan<std::string>(source, "{:[^\n]}");
            if (!res) {
                if (res.error().code() == scn::scan_error::end_of_input) {
                    break;
                }
                if (res.error().code() ==
                        scn::scan_error::invalid_scanned_value &&
                    std::string{res.error().msg()} ==
                        "Invalid encoding in scanned string" &&
                    !is_valid_unicode) {
                    // Allow this kind of error,
                    // if the input really was invalid
                    return std::string{expected};
                }
                throw std::runtime_error("scan failed with " +
                                         std::string(res.error().msg()));
            }
            buffer.append(res->value());
        }
        {
            auto res = scn::scan<char>(source, "{}");
            if (!res) {
                if (res.error().code() == scn::scan_error::end_of_input) {
                    break;
                }
                throw std::runtime_error("scan failed with " +
                                         std::string(res.error().msg()));
            }
            if (res->value() != '\n') {
                throw std::runtime_error("expected line break");
            }
            buffer.push_back('\n');
        }

        if (std::chrono::steady_clock::now() - start >=
            std::chrono::seconds{5}) {
            throw std::runtime_error("operation timed out");
        }
    }
    if (!is_valid_unicode) {
        throw std::runtime_error("no error on invalid encoding");
    }
    return buffer;
}

template <typename Source>
std::string do_run_all(Source& source,
                       std::string_view expected,
                       bool is_valid_unicode)
{
    auto res = scn::scan<std::string>(source, "{:.4096c}");
    if (!res) {
        if (res.error().code() == scn::scan_error::invalid_scanned_value &&
            std::string{res.error().msg()} ==
                "Invalid encoding in scanned string" &&
            !is_valid_unicode) {
            // Allow this kind of error,
            // if the input really was invalid
            return std::string{expected};
        }
        throw std::runtime_error("scan failed with " +
                                 std::string(res.error().msg()));
    }
    if (!is_valid_unicode) {
        throw std::runtime_error("no error on invalid encoding");
    }
    return res->value();
}

void check(std::string_view result, std::string_view expected)
{
    if (result != expected) {
        throw std::runtime_error("incorrect result");
    }
}

void run_file(std::string_view expected, bool is_valid_unicode)
{
    auto ensure_eof = [&](scn::scan_file& file) {
        if (is_valid_unicode) {
            SCN_ENSURE(file.prelude().empty());
            if (std::feof(file.handle().value()) == 0) {
                throw std::runtime_error("expected eof");
            }
        }
    };

    {
        const cfile_guard guard{};
        scn::scan_file file{guard.file};
        const auto s = do_run_all(file, expected, is_valid_unicode);
        check(s, expected);
        ensure_eof(file);
    }
    {
        const cfile_guard guard{};
        scn::scan_file file{guard.file};
        const auto s = do_run_line_by_line(file, expected, is_valid_unicode);
        check(s, expected);
        ensure_eof(file);
    }
}

void run_fstream(std::string_view expected, bool is_valid_unicode)
{
    auto ensure_eof = [&](std::istream& strm) {
        if (is_valid_unicode) {
            if (strm.eof()) {
                return;
            }
            char ch{};
            strm >> ch;
            if (!strm.eof()) {
                throw std::runtime_error("expected eof");
            }
        }
    };

    {
        std::ifstream strm{path, std::ios::in | std::ios::binary};
        const auto s = do_run_all(strm, expected, is_valid_unicode);
        check(s, expected);
        ensure_eof(strm);
    }
    {
        std::ifstream strm{path, std::ios::in | std::ios::binary};
        const auto s = do_run_line_by_line(strm, expected, is_valid_unicode);
        check(s, expected);
        ensure_eof(strm);
    }
}

void run(const uint8_t* data, size_t size)
{
    if (size > max_input_bytes || size == 0) {
        return;
    }

    std::vector<char> buffer{};
    buffer.resize(size + 1);
    std::memcpy(buffer.data(), data, size);
    std::string_view input{buffer.data()};

    {
        auto out = std::ofstream{
            path, std::ios::out | std::ios::binary | std::ios::trunc};
        out.write(input.data(), static_cast<std::streamsize>(input.size()));
    }

    bool is_valid_unicode = scn::impl::validate_unicode(input);

    run_file(input, is_valid_unicode);
    run_fstream(input, is_valid_unicode);
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    run(data, size);
    return 0;
}

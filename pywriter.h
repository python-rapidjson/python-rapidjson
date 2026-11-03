// -*- coding: utf-8 -*-
// :Project:   python-rapidjson -- Fast JSON string serializers
// :Author:    Lele Gaifax <lele@metapensiero.it>
// :License:   MIT License
// :Copyright: © 2026 Lele Gaifax
//

#ifndef PYTHON_RAPIDJSON_PYWRITER_H
#define PYTHON_RAPIDJSON_PYWRITER_H

#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include "rapidjson/encodings.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/writer.h"

/* SSE2 is part of the x86-64 baseline, so we can rely on the optimizer provided
   macros instead of requiring a specific build flag. */

#if defined(__SSE2__) || (defined(_M_X64) && !defined(_M_ARM64EC))
#define PYTHON_RAPIDJSON_SSE2 1
#include <emmintrin.h>
#elif defined(RAPIDJSON_SSE2)
#define PYTHON_RAPIDJSON_SSE2 1
#include <emmintrin.h>
#endif


/* This header implements the two writers used by python-rapidjson, deriving them
   from the rapidjson ones and replacing the implementation of WriteString().

   rapidjson's own Writer::WriteString() has to cope with any combination of source
   and target encodings, it validates the source and it emits one code unit at a
   time: all of that is redundant here, because

   - the source is always UTF-8 coming from PyUnicode_AsUTF8AndSize(), and CPython
     guarantees that it is valid;

   - when the target encoding is ASCII we only need to translate each well formed
     UTF-8 sequence into a \uXXXX escape, which can be done directly from the UTF-8
     bytes, without decoding a codepoint first;

   - when the target encoding is UTF-8 the job is a memcpy() plus a scan for the
     characters that need escaping, and that scan can proceed 16 bytes at a time.

   Likewise, WriteRawValue() transcodes the value code unit by code unit, while
   RawJSON values are already valid UTF-8 as far as we are concerned and can simply
   be copied over. */

namespace pyrapidjson {

//! Append a run of bytes to the output stream.
/*! The generic form relies on an Append() method on the stream; the overload
    below covers rapidjson's buffers, whose Ch is a char in every encoding we
    use here. */

template<typename OutputStream>
inline void
append(OutputStream& os, const char* s, size_t n)
{
    os.Append(s, n);
}

template<typename Encoding, typename Allocator>
inline void
append(rapidjson::GenericStringBuffer<Encoding, Allocator>& os, const char* s, size_t n)
{
    std::memcpy(os.Push(n), s, n);
}


//! Index of the lowest set bit, the argument must not be zero.
inline unsigned
firstSetBit(unsigned mask)
{
#if defined(_MSC_VER)
    unsigned long index;
    _BitScanForward(&index, mask);
    return static_cast<unsigned>(index);
#else
    return static_cast<unsigned>(__builtin_ctz(mask));
#endif
}

inline unsigned
firstSetBit64(uint64_t mask)
{
#if defined(_MSC_VER)
    unsigned long index;
    const uint32_t low = static_cast<uint32_t>(mask);
    if (low != 0) {
        _BitScanForward(&index, low);
    } else {
        _BitScanForward(&index, static_cast<uint32_t>(mask >> 32));
        index += 32;
    }
    return static_cast<unsigned>(index);
#else
    return static_cast<unsigned>(__builtin_ctzll(mask));
#endif
}


//! Position of the first byte that needs to be escaped.
/*! When \a asciiTarget is true, non-ASCII bytes also stop the scan, because they
    must be turned into \uXXXX escapes. */

template<bool asciiTarget>
inline size_t
scan_plain(const char* p, size_t len)
{
    size_t i = 0;

#if PYTHON_RAPIDJSON_SSE2
    const __m128i dquote = _mm_set1_epi8('"');
    const __m128i bslash = _mm_set1_epi8('\\');
    const __m128i control = _mm_set1_epi8(0x1F);

    for (; i + 16 <= len; i += 16) {
        const __m128i s = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + i));
        __m128i x = _mm_cmpeq_epi8(s, dquote);
        x = _mm_or_si128(x, _mm_cmpeq_epi8(s, bslash));
        // s <= 0x1F, i.e. some control character
        x = _mm_or_si128(x, _mm_cmpeq_epi8(_mm_max_epu8(s, control), control));
        if (asciiTarget) {
            x = _mm_or_si128(x, _mm_cmplt_epi8(s, _mm_setzero_si128()));
        }
        const unsigned mask = static_cast<unsigned>(_mm_movemask_epi8(x));
        if (mask != 0) {
            return i + static_cast<size_t>(firstSetBit(mask));
        }
    }
#elif defined(__GNUC__) || defined(__clang__)
    // Portable 8 bytes at a time version, for the CPUs without SSE2
    for (; i + 8 <= len; i += 8) {
        uint64_t word;
        std::memcpy(&word, p + i, sizeof(word));
        const uint64_t ones = UINT64_C(0x0101010101010101);
        uint64_t x = (word ^ ones * static_cast<uint64_t>('"'));
        uint64_t hits = (x - ones) & ~x & UINT64_C(0x8080808080808080);
        x = (word ^ ones * static_cast<uint64_t>('\\'));
        hits |= (x - ones) & ~x & UINT64_C(0x8080808080808080);
        hits |= (word - ones * 0x20) & ~word & UINT64_C(0x8080808080808080);
        if (asciiTarget) {
            hits |= word & UINT64_C(0x8080808080808080);
        }
        if (hits != 0) {
            return i + static_cast<size_t>(firstSetBit64(hits) >> 3);
        }
    }
#endif

    for (; i < len; i++) {
        const unsigned char c = static_cast<unsigned char>(p[i]);
        if (c < 0x20 || c == '"' || c == '\\' || (asciiTarget && c >= 0x80)) {
            return i;
        }
    }

    return len;
}


//! Translate one UTF-8 sequence into its \uXXXX escape(s).
/*! The caller guarantees that there is room for 12 bytes in \a dst. Returns the
    number of bytes consumed from \a src, zero if the sequence is truncated. */

inline size_t
write_utf8_escape(char* dst, const unsigned char* src, size_t avail)
{
    static const char hexDigits[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    const unsigned b1 = src[0];

    if (b1 < 0xE0) {
        if (avail < 2) {
            return 0;
        }
        const unsigned b2 = src[1];
        dst[0] = '\\';
        dst[1] = 'u';
        dst[2] = '0';
        dst[3] = hexDigits[(b1 & 0x1F) >> 2];
        dst[4] = hexDigits[((b1 & 0x03) << 2) | ((b2 & 0x3F) >> 4)];
        dst[5] = hexDigits[b2 & 0x0F];
        return 2;
    }
    else if (b1 < 0xF0) {
        if (avail < 3) {
            return 0;
        }
        const unsigned b2 = src[1];
        const unsigned b3 = src[2];
        dst[0] = '\\';
        dst[1] = 'u';
        dst[2] = hexDigits[b1 & 0x0F];
        dst[3] = hexDigits[(b2 & 0x3F) >> 2];
        dst[4] = hexDigits[((b2 & 0x03) << 2) | ((b3 & 0x3F) >> 4)];
        dst[5] = hexDigits[b3 & 0x0F];
        return 3;
    }
    else {
        if (avail < 4) {
            return 0;
        }
        const unsigned b2 = src[1];
        const unsigned b3 = src[2];
        const unsigned b4 = src[3];
        unsigned codepoint = (((b1 & 0x07u) << 18)
                              | ((b2 & 0x3Fu) << 12)
                              | ((b3 & 0x3Fu) << 6)
                              | (b4 & 0x3Fu));
        codepoint -= 0x10000;
        const unsigned lead = (codepoint >> 10) + 0xD800;
        const unsigned trail = (codepoint & 0x3FF) + 0xDC00;
        dst[0] = '\\';
        dst[1] = 'u';
        dst[2] = hexDigits[(lead >> 12) & 0x0F];
        dst[3] = hexDigits[(lead >> 8) & 0x0F];
        dst[4] = hexDigits[(lead >> 4) & 0x0F];
        dst[5] = hexDigits[lead & 0x0F];
        dst[6] = '\\';
        dst[7] = 'u';
        dst[8] = hexDigits[(trail >> 12) & 0x0F];
        dst[9] = hexDigits[(trail >> 8) & 0x0F];
        dst[10] = hexDigits[(trail >> 4) & 0x0F];
        dst[11] = hexDigits[trail & 0x0F];
        return 4;
    }
}


//! Serialize a UTF-8 string, wrapping it in double quotes.
/*! This is the equivalent of Writer::WriteString(), specialized for the fact that
    the source is known to be valid UTF-8. The escapes are accumulated into a
    small buffer, so that a string with many of them (ie. a non-ASCII one encoded
    into an ASCII stream) is emitted in a handful of bulk writes. */

template<bool asciiTarget, typename OutputStream>
inline bool
write_string(OutputStream& os, const char* str, size_t len)
{
    static const char hexDigits[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };
    static const char quote = '"';

    char escape[252];
    size_t nescape = 0;

    auto flush = [&]() {
        if (nescape > 0) {
            append(os, escape, nescape);
            nescape = 0;
        }
    };

    append(os, &quote, 1);

    size_t i = 0;
    while (i < len) {
        const size_t start = i;
        i += scan_plain<asciiTarget>(str + i, len - i);
        if (i > start) {
            /* Short runs are staged together with the escapes, so that a string
               with many of either is emitted with just a handful of bulk writes;
               longer ones go straight to the output stream. */

            const size_t run = i - start;
            if (run > sizeof(escape) - 12 - nescape) {
                flush();
                append(os, str + start, run);
            } else {
                char* dst = escape + nescape;
                if (run <= 8) {
                    for (size_t k = 0; k < run; k++) {
                        dst[k] = str[start + k];
                    }
                } else {
                    std::memcpy(dst, str + start, run);
                }
                nescape += run;
            }
        }
        if (i == len) {
            break;
        }

        const unsigned char b1 = static_cast<unsigned char>(str[i]);
        if (b1 < 0x80) {
            size_t n;
            switch (b1) {
            case '"':  escape[nescape] = '\\'; escape[nescape+1] = '"';  n = 2; break;
            case '\\': escape[nescape] = '\\'; escape[nescape+1] = '\\'; n = 2; break;
            case '\b': escape[nescape] = '\\'; escape[nescape+1] = 'b';  n = 2; break;
            case '\f': escape[nescape] = '\\'; escape[nescape+1] = 'f';  n = 2; break;
            case '\n': escape[nescape] = '\\'; escape[nescape+1] = 'n';  n = 2; break;
            case '\r': escape[nescape] = '\\'; escape[nescape+1] = 'r';  n = 2; break;
            case '\t': escape[nescape] = '\\'; escape[nescape+1] = 't';  n = 2; break;
            default:
                escape[nescape] = '\\';
                escape[nescape+1] = 'u';
                escape[nescape+2] = '0';
                escape[nescape+3] = '0';
                escape[nescape+4] = hexDigits[b1 >> 4];
                escape[nescape+5] = hexDigits[b1 & 0x0F];
                n = 6;
                break;
            }
            nescape += n;
            i++;
            if (nescape > sizeof(escape) - 12) {
                flush();
            }
        }
        else {
            /* A non-ASCII byte: the target encoding is ASCII, so translate the
               UTF-8 sequences into their \uXXXX escapes reading the bytes
               directly, without decoding a codepoint first. Consecutive ones are
               handled here, without going back through the scan. */

            do {
                if (nescape > sizeof(escape) - 12) {
                    flush();
                }
                const size_t consumed = write_utf8_escape(escape + nescape,
                                                          reinterpret_cast<const unsigned char*>(str) + i,
                                                          len - i);
                if (consumed == 0) {
                    return false;
                }
                nescape += consumed == 4 ? 12 : 6;
                i += consumed;
            } while (i < len && static_cast<unsigned char>(str[i]) >= 0x80);
        }
    }

    flush();
    append(os, &quote, 1);

    return true;
}


//! Write a pre-serialized JSON value, as it is.
template<typename OutputStream>
inline bool
write_raw_value(OutputStream& os, const char* json, size_t len)
{
    append(os, json, len);
    return true;
}


//! Writer using the fast string serializer above.
template<typename OutputStream, typename TargetEncoding = rapidjson::UTF8<> >
class FastWriter : public rapidjson::Writer<OutputStream, rapidjson::UTF8<>, TargetEncoding>
{
    typedef rapidjson::Writer<OutputStream, rapidjson::UTF8<>, TargetEncoding> Base;

public:
    explicit FastWriter(OutputStream& os, rapidjson::CrtAllocator* allocator = 0,
                        size_t levelDepth = Base::kDefaultLevelDepth)
        : Base(os, allocator, levelDepth)
    {}

    bool String(const char* str, rapidjson::SizeType length, bool copy = false) {
        (void) copy;
        this->Prefix(rapidjson::kStringType);
        return this->EndValue(write_string<TargetEncoding::supportUnicode == 0>(*this->os_,
                                                                               str, length));
    }

    bool Key(const char* str, rapidjson::SizeType length, bool copy = false) {
        return String(str, length, copy);
    }

    bool RawValue(const char* json, size_t length, rapidjson::Type type) {
        this->Prefix(type);
        return this->EndValue(write_raw_value(*this->os_, json, length));
    }
};


//! Pretty writer using the fast string serializer above.
template<typename OutputStream, typename TargetEncoding = rapidjson::UTF8<> >
class FastPrettyWriter
    : public rapidjson::PrettyWriter<OutputStream, rapidjson::UTF8<>, TargetEncoding>
{
    typedef rapidjson::PrettyWriter<OutputStream, rapidjson::UTF8<>, TargetEncoding> Base;

public:
    explicit FastPrettyWriter(OutputStream& os, rapidjson::CrtAllocator* allocator = 0,
                              size_t levelDepth = Base::kDefaultLevelDepth)
        : Base(os, allocator, levelDepth)
    {}

    bool String(const char* str, rapidjson::SizeType length, bool copy = false) {
        (void) copy;
        this->PrettyPrefix(rapidjson::kStringType);
        return Base::EndValue(write_string<TargetEncoding::supportUnicode == 0>(*Base::os_,
                                                                               str, length));
    }

    bool Key(const char* str, rapidjson::SizeType length, bool copy = false) {
        return String(str, length, copy);
    }

    bool RawValue(const char* json, size_t length, rapidjson::Type type) {
        this->PrettyPrefix(type);
        return Base::EndValue(write_raw_value(*Base::os_, json, length));
    }
};

}  // namespace pyrapidjson

#endif  // PYTHON_RAPIDJSON_PYWRITER_H

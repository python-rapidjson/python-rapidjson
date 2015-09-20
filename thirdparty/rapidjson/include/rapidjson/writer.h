// Tencent is pleased to support the open source community by making RapidJSON available.
//
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All rights reserved.
//
// Licensed under the MIT License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#ifndef RAPIDJSON_WRITER_H_
#define RAPIDJSON_WRITER_H_

#include "rapidjson.h"
#include "internal/stack.h"
#include "internal/strfunc.h"
#include "internal/dtoa.h"
#include "internal/itoa.h"
#include "stringbuffer.h"
#include <new>      // placement new

#if RAPIDJSON_HAS_STDSTRING
#include <string>
#endif

#ifdef _MSC_VER
RAPIDJSON_DIAG_PUSH
RAPIDJSON_DIAG_OFF(4127) // conditional expression is constant
#endif

RAPIDJSON_NAMESPACE_BEGIN

//! JSON writer
/*! Writer implements the concept Handler.
    It generates JSON text by events to an output os.

    User may programmatically calls the functions of a writer to generate JSON text.

    On the other side, a writer can also be passed to objects that generates events,

    for example Reader::Parse() and Document::Accept().

    \tparam OutputStream Type of output stream.
    \tparam SourceEncoding Encoding of source string.
    \tparam TargetEncoding Encoding of output stream.
    \tparam StackAllocator Type of allocator for allocating memory of stack.
    \note implements Handler concept
*/
template<typename OutputStream, typename SourceEncoding = UTF8<>, typename TargetEncoding = UTF8<>, typename StackAllocator = CrtAllocator>
class Writer {
public:
    typedef typename SourceEncoding::Ch Ch;

    //! Constructor
    /*! \param os Output stream.
        \param stackAllocator User supplied allocator. If it is null, it will create a private one.
        \param levelDepth Initial capacity of stack.
    */
    explicit
    Writer(OutputStream& os, StackAllocator* stackAllocator = 0, size_t levelDepth = kDefaultLevelDepth) :
        os_(&os), level_stack_(stackAllocator, levelDepth * sizeof(Level)), hasRoot_(false) {}

    explicit
    Writer(StackAllocator* allocator = 0, size_t levelDepth = kDefaultLevelDepth) :
        os_(0), level_stack_(allocator, levelDepth * sizeof(Level)), hasRoot_(false) {}

    //! Reset the writer with a new stream.
    /*!
        This function reset the writer with a new stream and default settings,
        in order to make a Writer object reusable for output multiple JSONs.

        \param os New output stream.
        \code
        Writer<OutputStream> writer(os1);
        writer.StartObject();
        // ...
        writer.EndObject();

        writer.Reset(os2);
        writer.StartObject();
        // ...
        writer.EndObject();
        \endcode
    */
    void Reset(OutputStream& os) {
        os_ = &os;
        hasRoot_ = false;
        level_stack_.Clear();
    }

    //! Checks whether the output is a complete JSON.
    /*!
        A complete JSON has a complete root object or array.
    */
    bool IsComplete() const {
        return hasRoot_ && level_stack_.Empty();
    }

    /*!@name Implementation of Handler
        \see Handler
    */
    //@{

    bool RawNumber(const Ch* str, SizeType length) {
        Prefix(kNumberType);
        const char* end = str + length;
        for (const char* p = str; p != end; ++p)
            os_->Put(*p);
        return true;
    }

    bool Null()                 { Prefix(kNullType);   return WriteNull(); }
    bool Bool(bool b)           { Prefix(b ? kTrueType : kFalseType); return WriteBool(b); }
    bool Int(int i)             { Prefix(kNumberType); return WriteInt(i); }
    bool Uint(unsigned u)       { Prefix(kNumberType); return WriteUint(u); }
    bool Int64(int64_t i64)     { Prefix(kNumberType); return WriteInt64(i64); }
    bool Uint64(uint64_t u64)   { Prefix(kNumberType); return WriteUint64(u64); }

    //! Writes the given \c double value to the stream
    /*!
        \param d The value to be written.
        \return Whether it is succeed.
    */
    bool Double(double d)       { Prefix(kNumberType); return WriteDouble(d); }

    bool String(const Ch* str, SizeType length, bool copy = false) {
        (void)copy;
        Prefix(kStringType);
        return WriteString(str, length);
    }

#if RAPIDJSON_HAS_STDSTRING
    bool String(const std::basic_string<Ch>& str) {
        return String(str.data(), SizeType(str.size()));
    }
#endif

    bool StartObject() {
        Prefix(kObjectType);
        new (level_stack_.template Push<Level>()) Level(false);
        return WriteStartObject();
    }

    bool Key(const Ch* str, SizeType length, bool copy = false) { return String(str, length, copy); }

    bool EndObject(SizeType memberCount = 0) {
        (void)memberCount;
        RAPIDJSON_ASSERT(level_stack_.GetSize() >= sizeof(Level));
        RAPIDJSON_ASSERT(!level_stack_.template Top<Level>()->inArray);
        level_stack_.template Pop<Level>(1);
        bool ret = WriteEndObject();
        if (level_stack_.Empty())   // end of json text
            os_->Flush();
        return ret;
    }

    bool StartArray() {
        Prefix(kArrayType);
        new (level_stack_.template Push<Level>()) Level(true);
        return WriteStartArray();
    }

    bool EndArray(SizeType elementCount = 0) {
        (void)elementCount;
        RAPIDJSON_ASSERT(level_stack_.GetSize() >= sizeof(Level));
        RAPIDJSON_ASSERT(level_stack_.template Top<Level>()->inArray);
        level_stack_.template Pop<Level>(1);
        bool ret = WriteEndArray();
        if (level_stack_.Empty())   // end of json text
            os_->Flush();
        return ret;
    }
    //@}

    /*! @name Convenience extensions */
    //@{

    //! Simpler but slower overload.
    bool String(const Ch* str) { return String(str, internal::StrLen(str)); }
    bool Key(const Ch* str) { return Key(str, internal::StrLen(str)); }

    //@}

protected:
    //! Information for each nested level
    struct Level {
        Level(bool inArray_) : valueCount(0), inArray(inArray_) {}
        size_t valueCount;  //!< number of values in this level
        bool inArray;       //!< true if in array, otherwise in object
    };

    static const size_t kDefaultLevelDepth = 32;

    bool WriteNull()  {
        os_->Put('n'); os_->Put('u'); os_->Put('l'); os_->Put('l'); return true;
    }

    bool WriteBool(bool b)  {
        if (b) {
            os_->Put('t'); os_->Put('r'); os_->Put('u'); os_->Put('e');
        }
        else {
            os_->Put('f'); os_->Put('a'); os_->Put('l'); os_->Put('s'); os_->Put('e');
        }
        return true;
    }

    bool WriteInt(int i) {
        char buffer[11];
        const char* end = internal::i32toa(i, buffer);
        for (const char* p = buffer; p != end; ++p)
            os_->Put(*p);
        return true;
    }

    bool WriteUint(unsigned u) {
        char buffer[10];
        const char* end = internal::u32toa(u, buffer);
        for (const char* p = buffer; p != end; ++p)
            os_->Put(*p);
        return true;
    }

    bool WriteInt64(int64_t i64) {
        char buffer[21];
        const char* end = internal::i64toa(i64, buffer);
        for (const char* p = buffer; p != end; ++p)
            os_->Put(*p);
        return true;
    }

    bool WriteUint64(uint64_t u64) {
        char buffer[20];
        char* end = internal::u64toa(u64, buffer);
        for (char* p = buffer; p != end; ++p)
            os_->Put(*p);
        return true;
    }

    bool WriteDouble(double d) {
        char buffer[25];
        char* end = internal::dtoa(d, buffer);
        for (char* p = buffer; p != end; ++p)
            os_->Put(*p);
        return true;
    }

    bool WriteString(const Ch* str, SizeType length)  {
        static const char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        static const char escape[256] = {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            //0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
            'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
            'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
              0,   0, '"',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20
            Z16, Z16,                                                                       // 30~4F
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,'\\',   0,   0,   0, // 50
            Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16                                // 60~FF
#undef Z16
        };

        os_->Put('\"');
        GenericStringStream<SourceEncoding> is(str);
        while (is.Tell() < length) {
            const Ch c = is.Take();
            const unsigned char byte1 = (unsigned char) c;
            if (!TargetEncoding::supportUnicode && byte1 >= 0x80) {
                // Unicode escaping
                static const char Utf8NumBytes[256] = {
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 1
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 2
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 3
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 7
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // a
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // b
                    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // c
                    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // d
                    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // e
                    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0, // f
                };

                Ch* start;
                unsigned char byte2, byte3, byte4;
                unsigned lead, trail, codepoint;
                switch (Utf8NumBytes[byte1]) {
                    case 2:
                        byte2 = (unsigned char) is.Take();
                        start = os_->stack_.template Push<Ch>(6);
                        *start = '\\';
                        *(++start) = 'u';
                        *(++start) = '0';
                        *(++start) = hexDigits[((byte1 & 0x1Fu) >> 2)];
                        *(++start) = hexDigits[((byte1 & 0x03u) << 2) | ((byte2 & 0x3Fu) >> 4)];
                        *(++start) = hexDigits[byte2 & 0xFu];
                        break;
                    case 3:
                        byte2 = (unsigned char) is.Take();
                        byte3 = (unsigned char) is.Take();
                        start = os_->stack_.template Push<Ch>(6);
                        *start = '\\';
                        *(++start) = 'u';
                        *(++start) = hexDigits[byte1 & 0xFu];
                        *(++start) = hexDigits[(byte2 & 0x3Fu) >> 2];
                        *(++start) = hexDigits[((byte2 & 0x03u) << 2) | ((byte3 & 0x3Fu) >> 4)];
                        *(++start) = hexDigits[byte3 & 0xFu];
                        break;
                    case 4:
                        byte2 = (unsigned char) is.Take();
                        byte3 = (unsigned char) is.Take();
                        byte4 = (unsigned char) is.Take();

                        codepoint = (
                            (byte1 & 0x7u) << 18) | ((byte2 & 0x3Fu) << 12) |
                            ((byte3 & 0x3Fu) << 6) | ((byte4 & 0x3Fu));
                        codepoint -= 0x10000;
                        lead = (codepoint >> 10) + 0xD800;
                        trail = (codepoint & 0x3FF) + 0xDC00;

                        start = os_->stack_.template Push<Ch>(12);
                        *start = '\\';
                        *(++start) = 'u';
                        *(++start) = hexDigits[(lead >> 12) & 15];
                        *(++start) = hexDigits[(lead >>  8) & 15];
                        *(++start) = hexDigits[(lead >>  4) & 15];
                        *(++start) = hexDigits[(lead      ) & 15];
                        *(++start) = '\\';
                        *(++start) = 'u';
                        *(++start) = hexDigits[(trail >> 12) & 15];
                        *(++start) = hexDigits[(trail >>  8) & 15];
                        *(++start) = hexDigits[(trail >>  4) & 15];
                        *(++start) = hexDigits[(trail      ) & 15];
                        break;
                    default:
                        return false;
                }
            }
            else if ((sizeof(Ch) == 1 || (unsigned)c < 256) && escape[(unsigned char)c])  {
                Ch* start = os_->stack_.template Push<Ch>(2);
                *start = '\\';
                *(++start) = escape[(unsigned char)c];
                if (escape[(unsigned char)c] == 'u') {
                    start = os_->stack_.template Push<Ch>(4);
                    *start = '0';
                    *(++start) = '0';
                    *(++start) = hexDigits[(unsigned char)c >> 4];
                    *(++start) = hexDigits[(unsigned char)c & 0xF];
                }
            }
            else
                os_->Put(c);
        }
        os_->Put('\"');
        return true;
    }

    bool WriteStartObject() { os_->Put('{'); return true; }
    bool WriteEndObject()   { os_->Put('}'); return true; }
    bool WriteStartArray()  { os_->Put('['); return true; }
    bool WriteEndArray()    { os_->Put(']'); return true; }

    void Prefix(Type type) {
        (void)type;
        if (level_stack_.GetSize() != 0) { // this value is not at root
            Level* level = level_stack_.template Top<Level>();
            if (level->valueCount > 0) {
                if (level->inArray)
                    os_->Put(','); // add comma if it is not the first element in array
                else  // in object
                    os_->Put((level->valueCount % 2 == 0) ? ',' : ':');
            }
            if (!level->inArray && level->valueCount % 2 == 0)
                RAPIDJSON_ASSERT(type == kStringType);  // if it's in object, then even number should be a name
            level->valueCount++;
        }
        else {
            RAPIDJSON_ASSERT(!hasRoot_);    // Should only has one and only one root.
            hasRoot_ = true;
        }
    }

    OutputStream* os_;
    internal::Stack<StackAllocator> level_stack_;
    bool hasRoot_;

private:
    // Prohibit copy constructor & assignment operator.
    Writer(const Writer&);
    Writer& operator=(const Writer&);
};

// Full specialization for StringStream to prevent memory copying

template<>
inline bool Writer<StringBuffer>::WriteInt(int i) {
    char *buffer = os_->Push(11);
    const char* end = internal::i32toa(i, buffer);
    os_->Pop(static_cast<size_t>(11 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::WriteUint(unsigned u) {
    char *buffer = os_->Push(10);
    const char* end = internal::u32toa(u, buffer);
    os_->Pop(static_cast<size_t>(10 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::WriteInt64(int64_t i64) {
    char *buffer = os_->Push(21);
    const char* end = internal::i64toa(i64, buffer);
    os_->Pop(static_cast<size_t>(21 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::WriteUint64(uint64_t u) {
    char *buffer = os_->Push(20);
    const char* end = internal::u64toa(u, buffer);
    os_->Pop(static_cast<size_t>(20 - (end - buffer)));
    return true;
}

template<>
inline bool Writer<StringBuffer>::WriteDouble(double d) {
    char *buffer = os_->Push(25);
    char* end = internal::dtoa(d, buffer);
    os_->Pop(static_cast<size_t>(25 - (end - buffer)));
    return true;
}

RAPIDJSON_NAMESPACE_END

#ifdef _MSC_VER
RAPIDJSON_DIAG_POP
#endif

#endif // RAPIDJSON_RAPIDJSON_H_

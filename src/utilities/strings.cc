#include "utilities/strings.h"

#include <sstream>

SIF_NAMESPACE_BEGIN

static inline bool isoctal(char c) { return c >= '0' && c < '8'; }

static inline int hex_to_int(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    c = tolower(c);
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return 0;
}

std::string string_from_escaped_string(const std::string &str) {
    std::ostringstream ss;
    auto i = str.begin();
    auto end = str.end();
    while (i < end) {
        if (*i == '\\') {
            if (++i == end)
                continue;
            if (isoctal(*i)) {
                unsigned char c = *i - '0';
                auto send = i + 3;
                i++;
                while (i < send && i < end && isoctal(*i)) {
                    c <<= 3;
                    c |= *i - '0';
                    i++;
                }
                ss << c;
                continue;
            }
            if (*i == 'x') {
                i++;
                if (i < end && isxdigit(*i)) {
                    unsigned char c = hex_to_int(*i);
                    i++;
                    if (i < end && isxdigit(*i)) {
                        c <<= 4;
                        c |= hex_to_int(*i);
                        i++;
                    }
                    ss << c;
                }
                continue;
            }
            switch (*i) {
            case '\\':
                ss << '\\';
                break;
            case '\'':
                ss << '\'';
                break;
            case '"':
                ss << '\"';
                break;
            case '?':
                ss << '\?';
                break;
            case 'a':
                ss << '\a';
                break;
            case 'b':
                ss << '\b';
                break;
            case 'e':
                ss << '\e';
                break;
            case 'f':
                ss << '\f';
                break;
            case 'n':
                ss << '\n';
                break;
            case 'r':
                ss << '\r';
                break;
            case 't':
                ss << '\t';
                break;
            case 'v':
                ss << '\v';
                break;
            default:
                ss << *i;
                break;
            }
            i++;
        } else {
            ss << *i;
            i++;
        }
    }
    return ss.str();
}

std::string escaped_string_from_string(const std::string &str) {
    std::ostringstream ss;
    auto i = str.begin();
    auto end = str.end();
    while (i < end) {
        switch (*i) {
        case '\n':
            ss << "\\n";
            break;
        case '\r':
            ss << "\\r";
            break;
        case '"':
            ss << "\\\"";
            break;
        case '\'':
            ss << "\\'";
            break;
        case '\\':
            ss << "\\\\";
            break;
        default:
            ss << *i;
        }
        i++;
    }
    return ss.str();
}

std::string encode_utf8(uint32_t codepoint) {
    std::string utf8;

    if (codepoint <= 0x7F) {
        // 1-byte sequence: 0xxxxxxx
        utf8 += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        // 2-byte sequence: 110xxxxx 10xxxxxx
        utf8 += static_cast<char>(0xC0 | (codepoint >> 6));
        utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
        utf8 += static_cast<char>(0xE0 | (codepoint >> 12));
        utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0x10FFFF) {
        // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        utf8 += static_cast<char>(0xF0 | (codepoint >> 18));
        utf8 += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        throw std::invalid_argument("invalid unicode codepoint");
    }

    return utf8;
}

uint32_t decode_utf8(const std::string &utf8) {
    const unsigned char *bytes = reinterpret_cast<const unsigned char *>(utf8.data());
    size_t length = utf8.size();

    if (length == 0)
        throw std::invalid_argument("empty string");

    uint32_t codepoint = 0;

    if (bytes[0] <= 0x7F) {
        // 1-byte sequence
        codepoint = bytes[0];
    } else if ((bytes[0] & 0xE0) == 0xC0) {
        // 2-byte sequence
        if (length < 2 || (bytes[1] & 0xC0) != 0x80)
            throw std::invalid_argument("invalid UTF-8 continuation byte (2-byte)");

        codepoint = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);

        // Overlong check: must be >= 0x80
        if (codepoint < 0x80)
            throw std::invalid_argument("overlong UTF-8 encoding");

    } else if ((bytes[0] & 0xF0) == 0xE0) {
        // 3-byte sequence
        if (length < 3 || (bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80)
            throw std::invalid_argument("invalid UTF-8 continuation byte (3-byte)");

        codepoint = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);

        // Overlong check: must be >= 0x800
        if (codepoint < 0x800)
            throw std::invalid_argument("overlong UTF-8 encoding");

        // Surrogate check
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
            throw std::invalid_argument("invalid UTF-8: surrogate code point");

    } else if ((bytes[0] & 0xF8) == 0xF0) {
        // 4-byte sequence
        if (length < 4 || (bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80 ||
            (bytes[3] & 0xC0) != 0x80)
            throw std::invalid_argument("invalid UTF-8 continuation byte (4-byte)");

        codepoint = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) |
                    ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);

        // Overlong check: must be >= 0x10000
        if (codepoint < 0x10000)
            throw std::invalid_argument("overlong UTF-8 encoding");

        // Max range check
        if (codepoint > 0x10FFFF)
            throw std::invalid_argument("invalid UTF-8: code point too large");
    } else {
        throw std::invalid_argument("invalid UTF-8 start byte");
    }

    return codepoint;
}
SIF_NAMESPACE_END

#include "chunk.h"

#include "extern/utf8.h"

SIF_NAMESPACE_BEGIN

static bool isnewline(uint32_t c) { return c == '\r' || c == '\n'; }

static bool iswhitespace(uint32_t c) { return iswblank(c) || isnewline(c); }

std::string::const_iterator chunk::scan(std::string::const_iterator it, size_t location) {
    if (_type == word)
        while (it < _end && iswhitespace(utf8::peek_next(it, _end)))
            utf8::next(it, _end);
    for (size_t i = 0; i < location && it < _end; i++) {
        if (_type == character) {
            utf8::next(it, _end);
        } else if (_type == word) {
            while (it < _end && !iswhitespace(utf8::peek_next(it, _end)))
                utf8::next(it, _end);
            while (it < _end && iswhitespace(utf8::peek_next(it, _end)))
                utf8::next(it, _end);
        } else if (_type == item) {
            while (it < _end && utf8::peek_next(it, _end) != ',')
                utf8::next(it, _end);
            if (it < _end)
                utf8::next(it, _end);
        } else if (_type == line) {
            while (it < _end && !isnewline(utf8::peek_next(it, _end)))
                utf8::next(it, _end);
            if (it < _end)
                utf8::next(it, _end);
        }
    }
    return it;
}

std::string::const_iterator chunk::scan_end(std::string::const_iterator it) {
    if (it < _end && _type == character) {
        utf8::next(it, _end);
    } else if (_type == word) {
        while (it < _end && !iswhitespace(utf8::peek_next(it, _end)))
            utf8::next(it, _end);
    } else if (_type == item) {
        while (it < _end && utf8::peek_next(it, _end) != ',')
            utf8::next(it, _end);
    } else if (_type == line) {
        while (it < _end && !isnewline(utf8::peek_next(it, _end)))
            utf8::next(it, _end);
    }
    return it;
}

SIF_NAMESPACE_END

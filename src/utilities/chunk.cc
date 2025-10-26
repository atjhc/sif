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
            // Skip to end of current item (until we find delimiter)
            auto delim_it = _delimiter.begin();
            while (it < _end) {
                if (delim_it == _delimiter.end()) {
                    // Found complete delimiter, skip past it
                    break;
                }
                auto current_char = utf8::peek_next(it, _end);
                if (current_char == static_cast<uint32_t>(*delim_it)) {
                    delim_it++;
                    utf8::next(it, _end);
                } else {
                    // Mismatch, reset delimiter search
                    if (delim_it != _delimiter.begin()) {
                        delim_it = _delimiter.begin();
                    } else {
                        utf8::next(it, _end);
                    }
                }
            }
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
        // Find end of current item (until we find delimiter)
        auto delim_it = _delimiter.begin();
        while (it < _end) {
            auto current_char = utf8::peek_next(it, _end);
            if (current_char == static_cast<uint32_t>(*delim_it)) {
                delim_it++;
                if (delim_it == _delimiter.end()) {
                    // Found complete delimiter, stop here (don't consume it)
                    it = std::prev(it, _delimiter.size() - 1);
                    break;
                }
                utf8::next(it, _end);
            } else {
                // Mismatch, reset delimiter search and continue
                if (delim_it != _delimiter.begin()) {
                    delim_it = _delimiter.begin();
                } else {
                    utf8::next(it, _end);
                }
            }
        }
    } else if (_type == line) {
        while (it < _end && !isnewline(utf8::peek_next(it, _end)))
            utf8::next(it, _end);
    }
    return it;
}

SIF_NAMESPACE_END

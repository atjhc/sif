#include "utilities/strings.h"

#include <sstream>

CH_NAMESPACE_BEGIN

static inline bool isoctal(char c) {
	return c >= '0' && c < '8';
}

static inline int oct_to_int(char c) {
	return c - '0';
}

static inline int hex_to_int(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	c = tolower(c);
	if (c >= 'a' && c <= 'f') return c - 'a';
	return 0;
}

std::string string_from_escaped_string(const std::string &str) {	
	std::ostringstream ss;
	auto i = str.begin();
	auto end = str.end();
	while (i < end) {
		if (*i == '\\') {
			if (++i == end) continue;
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
			}
			if (isoctal(*i)) {
				unsigned char c = *i - '0';
				auto send = i + 3;
				i++;
				while (i < send && i < end && isoctal(*i)) {
					c *= 8;
					c += *i - '0';
					i++;
				}
				ss << c;
				continue;
			}
			if (*i == 'x') {
				unsigned char c = 0;
				auto send = i + 3;
				i++;
				while (i < send && i < end && isxdigit(*i)) {
					c *= 16;
					c += hex_to_int(*i);
					i++;
				}
				ss << c;
				continue;
			}
			i++;
		} else {
			ss << *i;
			i++;
		}
	}
	return ss.str();
}

CH_NAMESPACE_END

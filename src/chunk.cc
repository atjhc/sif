#include "chunk.h"

CH_NAMESPACE_BEGIN

static bool iswhite(int c) {
	return isblank(c) || c == '\n';
}

std::string::iterator base_chunk::scan(std::string::iterator it, size_t location) {
	if (_type == character) {
		return min(_end, it + location);
	}
	if (_type == word) while(iswhite(*it) && it < _end) it++;
	for (size_t i = 0; i < location; i++) {
		if (_type == word) {
			while(!iswhite(*it) && it < _end) it++;
			while(iswhite(*it) && it < _end) it++;
		} else if (_type == item) {
			while(*it != ',' && it < _end) it++;
			if (it < _end) it++;
		} else if (_type == line) {
			while(*it != '\n' && it < _end) it++;
			if (it < _end) it++;
		}
	}
	return it;
}

std::string::iterator base_chunk::scan_end(std::string::iterator it) {
	if (_type == character && it < _end) {
		return min(_end, it + 1);
	}
	if (_type == word) {
		while(!iswhite(*it) && it < _end) it++;
	} else if (_type == item) {
		while(*it != ',' && it < _end) it++;
	} else if (_type == line) {
		while(*it != '\n' && it < _end) it++;
	}
	return it;
}

CH_NAMESPACE_END

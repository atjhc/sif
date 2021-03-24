#include "chunk.h"

static void chunk_tests(TestSuite &s) {
	std::string string = 
		"firstly, line 1 of the string\n"
		"then, line 2\n"
		"lastly, line 3 of the string\n"
	;

	s.assert_eq(chatter::chunk(chatter::character, 0, string).get(), "f");
	s.assert_eq(chatter::chunk(chatter::character, 5, string).get(), "l");
	s.assert_eq(chatter::chunk(chatter::character, 100, string).get(), "");

	s.assert_eq(chatter::range_chunk(chatter::character, 0, 6, string).get(), "firstly");
	s.assert_eq(chatter::range_chunk(chatter::character, 9, 12, string).get(), "line");
	s.assert_eq(chatter::range_chunk(chatter::character, 51, 100, string).get(), "line 3 of the string\n");

	s.assert_eq(chatter::last_chunk(chatter::character, string).get(), "\n");

	s.assert_eq(chatter::chunk(chatter::word, 0, string).get(), "firstly,");
	s.assert_eq(chatter::chunk(chatter::word, 5, string).get(), "string");
	s.assert_eq(chatter::chunk(chatter::word, 100, string).get(), "");

	s.assert_eq(chatter::range_chunk(chatter::word, 0, 5, string).get(), "firstly, line 1 of the string");
	s.assert_eq(chatter::range_chunk(chatter::word, 6, 7, string).get(), "then, line");
	s.assert_eq(chatter::range_chunk(chatter::word, 9, 100, string).get(), "lastly, line 3 of the string\n");

	s.assert_eq(chatter::last_chunk(chatter::word, string).get(), "string");

	s.assert_eq(chatter::chunk(chatter::item, 0, string).get(), "firstly");
	s.assert_eq(chatter::chunk(chatter::item, 2, string).get(), " line 2\nlastly");
	s.assert_eq(chatter::chunk(chatter::item, 100, string).get(), "");

	s.assert_eq(chatter::range_chunk(chatter::item, 0, 1, string).get(), "firstly, line 1 of the string\nthen");
	s.assert_eq(chatter::range_chunk(chatter::item, 1, 2, string).get(), " line 1 of the string\nthen, line 2\nlastly");
	s.assert_eq(chatter::range_chunk(chatter::item, 2, 100, string).get(), " line 2\nlastly, line 3 of the string\n");

	s.assert_eq(chatter::last_chunk(chatter::item, string).get(), " line 3 of the string\n");

	s.assert_eq(chatter::chunk(chatter::line, 0, string).get(), "firstly, line 1 of the string");
	s.assert_eq(chatter::chunk(chatter::line, 2, string).get(), "lastly, line 3 of the string");
	s.assert_eq(chatter::chunk(chatter::line, 100, string).get(), "");

	s.assert_eq(chatter::range_chunk(chatter::line, 0, 1, string).get(), "firstly, line 1 of the string\nthen, line 2");
	s.assert_eq(chatter::range_chunk(chatter::line, 1, 2, string).get(), "then, line 2\nlastly, line 3 of the string");
	s.assert_eq(chatter::range_chunk(chatter::line, 2, 100, string).get(), "lastly, line 3 of the string\n");

	s.assert_eq(chatter::last_chunk(chatter::line, string).get(), "lastly, line 3 of the string");

	s.assert_eq(chatter::random_chunk(chatter::line, [](int count) { return 1; }, string).get(), "then, line 2");	
}
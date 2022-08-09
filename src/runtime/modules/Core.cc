//
//  Copyright (c) 2021 James Callender
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include "Error.h"
#include "Utilities.h"

#include "runtime/modules/Core.h"
#include "runtime/objects/Dictionary.h"
#include "runtime/objects/List.h"
#include "runtime/objects/Native.h"
#include "runtime/objects/String.h"

#include "utilities/chunk.h"

#include <random>

SIF_NAMESPACE_BEGIN

Mapping<std::string, Strong<Native>> _functions = []() -> Mapping<std::string, Strong<Native>> {
    static std::random_device rd;
    static std::mt19937 engine(rd());
    auto random = [&](int max) {
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(engine);
    };

    Mapping<std::string, Strong<Native>> natives;
    natives["{} (up) to {}"] = MakeStrong<Native>([](Value *values) -> Value {
        return MakeStrong<Range>(values[0].asInteger(), values[1].asInteger(), true);
    });
    natives["quit"] = MakeStrong<Native>([](Value *values) -> Value { exit(0); });
    natives["quit with code {}"] =
        MakeStrong<Native>([](Value *values) -> Value { exit(values[0].asInteger()); });
    natives["write {}"] = MakeStrong<Native>([](Value *values) -> Value {
        std::cout << values[0];
        return Value();
    });
    natives["write error {}"] = MakeStrong<Native>([](Value *values) -> Value {
        std::cerr << values[0];
        return Value();
    });
    natives["print {}"] = MakeStrong<Native>([](Value *values) -> Value {
        if (const auto &list = values[0].as<List>()) {
            std::cout << Join(list->values(), " ");
        } else {
            std::cout << values[0];
        }
        std::cout << std::endl;
        return Value();
    });
    natives["print error {}"] = MakeStrong<Native>([](Value *values) -> Value {
        if (const auto &list = values[0].as<List>()) {
            for (const auto &item : list->values()) {
                std::cerr << item;
            }
        } else {
            std::cerr << values[0];
        }
        std::cerr << std::endl;
        return Value();
    });
    natives["get {}"] = MakeStrong<Native>([](Value *values) -> Value { return values[0]; });
    natives["read (a) word"] = MakeStrong<Native>([](Value *values) -> Value {
        std::string input;
        std::cin >> input;
        return input;
    });
    natives["read (a) line"] = MakeStrong<Native>([](Value *values) -> Value {
        std::string input;
        std::getline(std::cin, input);
        return input;
    });
    natives["(the) long description (of) {}"] =
        MakeStrong<Native>([](Value *values) -> Value { return values[0].description(); });
    natives["(the) (short) description (of) {}"] = MakeStrong<Native>([](Value *values) -> Value {
        std::ostringstream ss;
        ss << values[0];
        return ss.str();
    });
    natives["(the) size (of) {}"] = MakeStrong<Native>([](Value *values) -> Value {
        size_t size = 0;
        if (auto list = values[0].as<List>()) {
            size = list->values().size();
        } else if (auto dictionary = values[0].as<Dictionary>()) {
            size = dictionary->values().size();
        } else if (auto string = values[0].as<String>()) {
            size = string->string().size();
        }
        return static_cast<long>(size);
    });
    natives["item {} in/of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto index = values[0].asInteger();
        auto list = values[1].as<List>();
        return list->values()[index];
    });
    natives["remove {} from {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto dictionary = values[1].as<Dictionary>();
        dictionary->values().erase(values[0]);
        return values[1];
    });
    natives["(the) type (of) {}"] =
        MakeStrong<Native>([](Value *values) -> Value { return lowercase(values[0].typeName()); });
    natives["(the) sin (of) {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto argument = values[0].castFloat();
        return sin(argument);
    });
    natives["(the) cos (of) {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto argument = values[0].castFloat();
        return cos(argument);
    });
    natives["(the) tan (of) {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto argument = values[0].castFloat();
        return tan(argument);
    });
    natives["char/character {} of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto index = values[0].asInteger();
        auto text = values[1].as<String>();
        return index_chunk(chunk::character, index, text->string()).get();
    });
    natives["char/character {} to {} of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto start = values[0].asInteger();
        auto end = values[1].asInteger();
        auto text = values[2].as<String>();
        return range_chunk(chunk::character, start, end, text->string()).get();
    });
    natives["any char/character of {}"] = MakeStrong<Native>([random](Value *values) -> Value {
        auto text = values[0].as<String>();
        return random_chunk(chunk::character, random, text->string()).get();
    });
    natives["(the) mid/middle char/character of {}"] =
        MakeStrong<Native>([](Value *values) -> Value {
            auto text = values[0].as<String>();
            return middle_chunk(chunk::character, text->string()).get();
        });
    natives["(the) last char/character of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto text = values[0].as<String>();
        return last_chunk(chunk::character, text->string()).get();
    });
    natives["(the) number of chars/characters in {}"] =
        MakeStrong<Native>([](Value *values) -> Value {
            auto text = values[0].as<String>();
            return static_cast<long>(count_chunk(chunk::character, text->string()).count);
        });
    natives["word {} of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto index = values[0].asInteger();
        auto text = values[1].as<String>();
        return index_chunk(chunk::word, index, text->string()).get();
    });
    natives["word/words {} to {} of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto start = values[0].asInteger();
        auto end = values[1].asInteger();
        auto text = values[2].as<String>();
        return range_chunk(chunk::word, start, end, text->string()).get();
    });
    natives["any word of {}"] = MakeStrong<Native>([random](Value *values) -> Value {
        auto text = values[0].as<String>();
        return random_chunk(chunk::word, random, text->string()).get();
    });
    natives["(the) mid/middle word of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto text = values[0].as<String>();
        return middle_chunk(chunk::word, text->string()).get();
    });
    natives["(the) last word of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto text = values[0].as<String>();
        return last_chunk(chunk::word, text->string()).get();
    });
    natives["(the) number of words in {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto text = values[0].as<String>();
        return static_cast<long>(count_chunk(chunk::word, text->string()).count);
    });
    natives["line {} of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto index = values[0].asInteger();
        auto text = values[1].as<String>();
        return index_chunk(chunk::line, index, text->string()).get();
    });
    natives["line/lines {} to {} of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto start = values[0].asInteger();
        auto end = values[1].asInteger();
        auto text = values[2].as<String>();
        return range_chunk(chunk::line, start, end, text->string()).get();
    });
    natives["any line of {}"] = MakeStrong<Native>([random](Value *values) -> Value {
        auto text = values[0].as<String>();
        return random_chunk(chunk::line, random, text->string()).get();
    });
    natives["(the) mid/middle line of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto text = values[0].as<String>();
        return middle_chunk(chunk::line, text->string()).get();
    });
    natives["(the) last line of {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto text = values[0].as<String>();
        return last_chunk(chunk::line, text->string()).get();
    });
    natives["(the) number of lines in {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto text = values[0].as<String>();
        return static_cast<long>(count_chunk(chunk::line, text->string()).count);
    });
    natives["(the) keys (of) {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto dictionary = values[0].as<Dictionary>();
        auto keys = MakeStrong<List>();
        for (const auto &pair : dictionary->values()) {
            keys->values().push_back(pair.first);
        }
        return keys;
    });
    natives["(the) values (of) {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto dictionary = values[0].as<Dictionary>();
        auto valuesList = MakeStrong<List>();
        for (const auto &pair : dictionary->values()) {
            valuesList->values().push_back(pair.second);
        }
        return valuesList;
    });
    natives["append {} to {}"] = MakeStrong<Native>([](Value *values) -> Value {
        auto value = values[0];
        auto list = values[1].as<List>();
        list->values().push_back(value);
        return list;
    });

    return natives;
}();

std::vector<Signature> Core::signatures() const {
    std::vector<Signature> signatures;
    for (const auto &pair : _functions) {
        signatures.push_back(Signature::Make(pair.first));
    }
    return signatures;
}

Mapping<std::string, Strong<Native>> Core::functions() const { return _functions; }

SIF_NAMESPACE_END
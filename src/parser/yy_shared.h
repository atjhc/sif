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

#include "ast/Base.h"

struct ParseLocation {
    Location first;
    Location last;
};

#define YYLLOC_DEFAULT(Cur, Rhs, N)                             \
    do                                                          \
        if (N) {                                                \
            (Cur).first = YYRHSLOC(Rhs, 1).first;               \
            (Cur).last = YYRHSLOC(Rhs, N).last;                 \
        } else {                                                \
            (Cur).first = (Cur).first = YYRHSLOC(Rhs, 0).first; \
            (Cur).last = (Cur).last = YYRHSLOC(Rhs, 0).last;    \
        }                                                       \
    while (0)

typedef void* yyscan_t;

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

#include "Chunk.h"

CH_AST_NAMESPACE_BEGIN

std::string Chunk::ordinalName() const {
	switch (type) {
	case Char: return "char";
	case Word: return "word";
	case Item: return "item";
	case Line: return "line";
	}
}

void Chunk::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << " of ";
	expression->prettyPrint(out, context);
}

void RangeChunk::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << ordinalName() << " ";
	start->prettyPrint(out, context);
	if (end) {
		out << " to ";
		end->prettyPrint(out, context);
	}
	Chunk::prettyPrint(out, context);
}

void LastChunk::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "the last " << ordinalName();
	Chunk::prettyPrint(out, context);
}

void MiddleChunk::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "the middle " << ordinalName();
	Chunk::prettyPrint(out, context);
}

void AnyChunk::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "any " << ordinalName();
	Chunk::prettyPrint(out, context);
}

CH_AST_NAMESPACE_END

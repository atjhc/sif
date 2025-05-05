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

#ifndef SIF_H
#define SIF_H

#include <sif/Common.h>
#include <sif/Error.h>

#include <sif/runtime/ModuleLoader.h>
#include <sif/runtime/Value.h>
#include <sif/runtime/VirtualMachine.h>

#include <sif/compiler/Compiler.h>
#include <sif/compiler/Parser.h>

#include <sif/runtime/modules/Core.h>
#include <sif/runtime/modules/System.h>

#include <sif/runtime/objects/Dictionary.h>
#include <sif/runtime/objects/List.h>
#include <sif/runtime/objects/Native.h>
#include <sif/runtime/objects/String.h>

#endif // SIF_H
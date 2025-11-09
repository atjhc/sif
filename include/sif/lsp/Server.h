//
//  Copyright (c) 2025 James Callender
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

#pragma once

#include <iostream>
#include <sif/Common.h>
#include <sif/lsp/DocumentManager.h>
#include <sif/lsp/JsonRPC.h>
#include <sif/lsp/Protocol.h>
#include <sif/lsp/SemanticTokens.h>

SIF_NAMESPACE_BEGIN
namespace lsp {

class Server {
  public:
    Server(std::istream &in, std::ostream &out);
    void run();

  private:
    JsonRPC _rpc;
    DocumentManager _documentManager;
    bool _shutdownRequested = false;

    void registerHandlers();
    json handleInitialize(const json &params);
    void handleInitialized(const json &params);
    json handleShutdown(const json &params);
    void handleExit(const json &params);
    void handleDidOpen(const json &params);
    void handleDidChange(const json &params);
    void handleDidClose(const json &params);
    json handleSemanticTokensFull(const json &params);
    json handleCompletion(const json &params);

    void publishDiagnostics(const std::string &uri, const std::vector<Error> &errors);
};

} // namespace lsp
SIF_NAMESPACE_END

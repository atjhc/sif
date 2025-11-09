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

#include <extern/json.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <sif/Common.h>
#include <sif/lsp/Protocol.h>
#include <string>

SIF_NAMESPACE_BEGIN
namespace lsp {

using json = nlohmann::json;

class JsonRPC {
  public:
    struct Request {
        std::string jsonrpc = "2.0";
        std::optional<json> id;
        std::string method;
        std::optional<json> params;
    };

    struct Response {
        std::string jsonrpc = "2.0";
        json id;
        std::optional<json> result;
        std::optional<json> error;
    };

    struct Notification {
        std::string jsonrpc = "2.0";
        std::string method;
        std::optional<json> params;
    };

    using RequestHandler = std::function<json(const json &params)>;
    using NotificationHandler = std::function<void(const json &params)>;

    JsonRPC(std::istream &in, std::ostream &out);

    void registerRequestHandler(const std::string &method, RequestHandler handler);
    void registerNotificationHandler(const std::string &method, NotificationHandler handler);
    void sendResponse(const json &id, const json &result);
    void sendError(const json &id, int code, const std::string &message);
    void sendNotification(const std::string &method, const json &params);
    bool processMessage();
    void run();

  private:
    std::istream &_input;
    std::ostream &_output;
    std::map<std::string, RequestHandler> _requestHandlers;
    std::map<std::string, NotificationHandler> _notificationHandlers;

    std::optional<json> readMessage();
    void sendMessage(const Response &response);
    void sendMessage(const Notification &notification);
    void writeMessage(const json &message);
    void handleRequest(const json &message);
    void handleNotification(const json &message);
};

} // namespace lsp
SIF_NAMESPACE_END

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

#include "sif/lsp/JsonRPC.h"
#include <iostream>

SIF_NAMESPACE_BEGIN
namespace lsp {

JsonRPC::JsonRPC(std::istream &in, std::ostream &out) : _input(in), _output(out) {}

void JsonRPC::registerRequestHandler(const std::string &method, RequestHandler handler) {
    _requestHandlers[method] = handler;
}

void JsonRPC::registerNotificationHandler(const std::string &method, NotificationHandler handler) {
    _notificationHandlers[method] = handler;
}

void JsonRPC::sendResponse(const json &id, const json &result) {
    Response response;
    response.id = id;
    response.result = result;
    sendMessage(response);
}

void JsonRPC::sendError(const json &id, int code, const std::string &message) {
    Response response;
    response.id = id;
    response.error = json{{JsonKeys::code, code}, {JsonKeys::message, message}};
    sendMessage(response);
}

void JsonRPC::sendNotification(const std::string &method, const json &params) {
    Notification notification;
    notification.method = method;
    notification.params = params;
    sendMessage(notification);
}

bool JsonRPC::processMessage() {
    auto message = readMessage();
    if (!message) {
        return false;
    }

    try {
        if (message->contains(JsonKeys::id)) {
            handleRequest(*message);
        } else {
            handleNotification(*message);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
        if (message->contains(JsonKeys::id)) {
            sendError((*message)[JsonKeys::id], -32603, "Internal error");
        }
    }

    return true;
}

void JsonRPC::run() {
    while (processMessage()) {
    }
}

std::optional<json> JsonRPC::readMessage() {
    std::string line;
    int contentLength = 0;

    while (std::getline(_input, line)) {
        if (line.empty() || line == "\r") {
            break;
        }

        const std::string prefix = "Content-Length: ";
        if (line.compare(0, prefix.size(), prefix) == 0) {
            contentLength = std::stoi(line.substr(prefix.size()));
        }
    }

    if (contentLength == 0) {
        return std::nullopt;
    }

    std::string content(contentLength, '\0');
    _input.read(&content[0], contentLength);

    if (!_input) {
        return std::nullopt;
    }

    return json::parse(content);
}

void JsonRPC::sendMessage(const Response &response) {
    json j;
    j[JsonKeys::jsonrpc] = response.jsonrpc;
    j[JsonKeys::id] = response.id;
    if (response.result) {
        j[JsonKeys::result] = *response.result;
    }
    if (response.error) {
        j[JsonKeys::error] = *response.error;
    }
    writeMessage(j);
}

void JsonRPC::sendMessage(const Notification &notification) {
    json j;
    j[JsonKeys::jsonrpc] = notification.jsonrpc;
    j[JsonKeys::method] = notification.method;
    if (notification.params) {
        j[JsonKeys::params] = *notification.params;
    }
    writeMessage(j);
}

void JsonRPC::writeMessage(const json &message) {
    std::string content = message.dump();
    std::string header = "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";

    _output << header << content;
    _output.flush();
}

void JsonRPC::handleRequest(const json &message) {
    std::string method = message[JsonKeys::method];
    json id = message[JsonKeys::id];
    json params = message.contains(JsonKeys::params) ? message[JsonKeys::params] : json::object();

    auto it = _requestHandlers.find(method);
    if (it != _requestHandlers.end()) {
        try {
            json result = it->second(params);
            sendResponse(id, result);
        } catch (const std::exception &e) {
            sendError(id, -32603, std::string("Internal error: ") + e.what());
        }
    } else {
        sendError(id, -32601, "Method not found: " + method);
    }
}

void JsonRPC::handleNotification(const json &message) {
    std::string method = message[JsonKeys::method];
    json params = message.contains(JsonKeys::params) ? message[JsonKeys::params] : json::object();

    auto it = _notificationHandlers.find(method);
    if (it != _notificationHandlers.end()) {
        it->second(params);
    }
}

} // namespace lsp
SIF_NAMESPACE_END

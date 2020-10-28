/*-
 * Copyright (c) 2014-2020 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <stdio.h>
#include <cpprest/uri.h>
#include <cpprest/http_listener.h>
#include <cpprest/asyncrt_utils.h>

#include "RestIngestServer.hpp"

using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;

class CommandHandler
{
public:
    CommandHandler(utility::string_t url, void (*json_callback)(web::json::value));
    pplx::task<void> open() { return m_listener.open(); }
    pplx::task<void> close() { return m_listener.close(); }
private:
    void handle_get_or_post(http_request message);
        http_listener m_listener;
    void (*_json_callback)(web::json::value);
};

CommandHandler::CommandHandler(utility::string_t url, void (*json_callback)(web::json::value)) : m_listener(url), _json_callback(json_callback)
{
    m_listener.support(methods::GET, std::bind(&CommandHandler::handle_get_or_post, this, std::placeholders::_1));
    m_listener.support(methods::POST, std::bind(&CommandHandler::handle_get_or_post, this, std::placeholders::_1));
}

void CommandHandler::handle_get_or_post(http_request message)
{
    ucout << "Method: " << message.method() << std::endl;
    ucout << "URI: " << http::uri::decode(message.relative_uri().path()) << std::endl;
    ucout << "Query: " << http::uri::decode(message.relative_uri().query()) << std::endl << std::endl;
    json::value payload;
    message.extract_json().then([&payload](pplx::task<json::value> task)
	{
		payload = task.get();
	})
		.wait();
    ucout << payload.serialize() << std::endl;
    _json_callback(payload);
    /* TODO: Send this information into process_json */
    message.reply(status_codes::OK, "ACCEPTED");
}

int RestIngestServer::start()
{
    try
    {
        uri_builder uri(_address);
        auto addr = uri.to_uri().to_string();
        CommandHandler handler(addr, _json_callback);
        handler.open().wait();
        ucout << utility::string_t(U("Listening for requests at: ")) << addr << std::endl;
        ucout << U("Press ENTER key to quit...") << std::endl;
        std::string line;
        std::getline(std::cin, line);
        handler.close().wait();
    }
    catch (std::exception& ex)
    {
        ucout << U("Exception: ") << ex.what() << std::endl;
        ucout << U("Press ENTER key to quit...") << std::endl;
        std::string line;
        std::getline(std::cin, line);
    }
    return 0;
}

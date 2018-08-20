// file-browse.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#pragma comment(lib, "cpprest_2_10d")

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

#include <iostream>
#include <map>
#include <set>
#include <string>
using namespace std;

#define TRACE(msg)            wcout << msg
#define TRACE_ACTION(a, k, v) wcout << a << L" (" << k << L", " << v << L")\n"

map<utility::string_t, utility::string_t> dictionary;

void display_json(
	json::value const & jvalue,
	utility::string_t const & prefix)
{
	wcout << prefix << jvalue.serialize() << endl;
}

void handle_get(http_request request)
{
	TRACE(L"\nhandle GET\n");

	utility::string_t uri = request.absolute_uri().to_string();
	TRACE("\nGET uri: ");
	TRACE(uri.c_str());
	TRACE("\n");

	auto answer = json::value::object();

	for (auto const & p : dictionary)
	{
		answer[p.first] = json::value::string(p.second);
	}

	display_json(json::value::null(), L"R: ");
	display_json(answer, L"S: ");

	request.reply(status_codes::OK, answer);
}

void handle_request(
	http_request request,
	function<void(json::value const &, json::value &)> action)
{
	auto answer = json::value::object();

	request
		.extract_json()
		.then([&answer, &action](pplx::task<json::value> task) {
		try
		{
			auto const & jvalue = task.get();
			display_json(jvalue, L"R: ");

			if (!jvalue.is_null())
			{
				action(jvalue, answer);
			}
		}
		catch (http_exception const & e)
		{
			wcout << e.what() << endl;
		}
	})
		.wait();


	display_json(answer, L"S: ");

	request.reply(status_codes::OK, answer);
}


int main()
{
	http_listener listener(L"http://localhost/restdemo");

	listener.support(methods::GET, handle_get);

	try
	{
		listener
			.open()
			.then([&listener]() {TRACE(L"\nstarting to listen\n"); })
			.wait();

		while (true);
	}
	catch (exception const & e)
	{
		wcout << e.what() << endl;
	}

	return 0;
}

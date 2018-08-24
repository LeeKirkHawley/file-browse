// file-browse.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <boost/algorithm/string.hpp>
#include <cpprest/filestream.h>


#pragma comment(lib, "cpprest_2_10d")

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;


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

	// parse out id, if any
	wstring id = L"";
	std::vector<std::wstring> strs;
	boost::split(strs, uri, boost::is_any_of("/"));
	if (strs.size() > 2)
	{
		// return a file
		id = strs[2];

		ucout << request.to_string() << endl;

		auto paths = http::uri::split_path(http::uri::decode(request.relative_uri().path()));

		wstring Path = L"C:\\Work\\file-browse\\files\\";
		Path += id;
		concurrency::streams::fstream::open_istream(Path, std::ios::in).then([=](concurrency::streams::istream is)
		{
			request.reply(status_codes::OK, is, U("application/octet-stream"))
				.then([](pplx::task<void> t)
			{
				try {
					t.get();
				}
				catch (...) {
					//
				}
			});
		}).then([=](pplx::task<void>t)
		{
			try {
				t.get();
			}
			catch (...) {
				request.reply(status_codes::InternalError, U("INTERNAL ERROR "));
			}
		});

		return;
	}
	else
	{
		// return file list
	
		// ?limit=5&start=2
		int limit = -1;
		int start = 0;

		std::vector<std::wstring>  paginationsplit;
		boost::split(paginationsplit, uri, boost::is_any_of("?"));
		if (paginationsplit.size() > 1)  // we want a paginated list
		{
			std::vector<std::wstring>  paginationvars;
			boost::split(paginationvars, paginationsplit[1], boost::is_any_of("&"));

			limit = stoi(paginationvars[0].substr(6).c_str());
			start = stoi(paginationvars[1].substr(6).c_str());
		}
		
		wstring files = L"";

		WIN32_FIND_DATA data;
		HANDLE hFind = FindFirstFile(L"C:\\Work\\file-browse\\files\\*.*", &data);      // DIRECTORY

		int counter = 0;
		int firstitem = 0;
		if (limit != -1)  // paginated please
		{
			firstitem = start * limit;
		}

		if (hFind != INVALID_HANDLE_VALUE) 
		{
			do 
			{
				if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))
				{
					if (counter >= firstitem)
					{
						if (files.length())
							files += L",";

						files += data.cFileName;
					}

					counter++;
					if (limit != -1 && counter >= firstitem + limit)  // if we're paginating and we have reached the limit
						break;  
				}

			} while (FindNextFile(hFind, &data));
			FindClose(hFind);
		}

		auto answer = json::value::object();
		answer[L"ReturnValue"] = json::value::string(files);
		request.reply(status_codes::OK, answer);
	}
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

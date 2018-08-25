// started from https://mariusbancila.ro/blog/2017/11/19/revisited-full-fledged-client-server-example-with-c-rest-sdk-2-10/
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
#include <map>
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

void return_file(wstring id, http_request request)
{
	ucout << request.to_string() << endl;

	auto paths = http::uri::split_path(http::uri::decode(request.relative_uri().path()));

	wstring Path = L"C:\\Work\\file-browse\\files\\";
	Path += id;
	concurrency::streams::fstream::open_istream(Path, std::ios::in).then([=](concurrency::streams::istream is)
	{
		request.reply(status_codes::OK, is, U("application/octet-stream"))
			.then([request](pplx::task<void> t)
		{
			try {
				t.get();
			}
			catch (...) {
				request.reply(status_codes::InternalError, U("ERROR"));
			}
		});
	}).then([=](pplx::task<void>t)
	{
		try {
			t.get();
		}
		catch (...) {
			request.reply(status_codes::InternalError, U("ERROR"));
		}
	});
}

void return_unsorted_filelist(wstring uri, http_request request, int limit, int start)
{

	wstring files = L"";
	int counter = 0;
	int firstitem = 0;

	if (limit != -1)  // paginated please
	{
		firstitem = start * limit;
	}

	// get the file list and just write the entries to a comma delimited string
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(L"C:\\Work\\file-browse\\files\\*.*", &data);      // DIRECTORY
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

	// return the file list string
	auto answer = json::value::object();
	answer[L"ReturnValue"] = json::value::string(files);
	request.reply(status_codes::OK, answer);
}

void return_sorted_filelist(wstring uri, http_request request, int limit, int start)
{
	struct cmpByNumber : public std::binary_function<int, int, bool> {
		bool operator()(const int a, const int b) const {
			return a < b;
		}
	};
	typedef map<int, wstring, cmpByNumber> filemaptype;
	filemaptype filemap;

	wstring files = L"";

	// get filelist, put it in sorted map
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(L"C:\\Work\\file-browse\\files\\*.*", &data);      // DIRECTORY
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))
			{
				int filenumber;
				try
				{
					filenumber = std::stoi(data.cFileName);
				}
				catch (...)
				{
					continue;
				}
				wstring s = data.cFileName;
				filemap[filenumber] = s;
			}

		} while (FindNextFile(hFind, &data));

		FindClose(hFind);
	}

	int counter = 0;
	int firstitem = 0;
	if (limit != -1)  // paginated please
	{
		firstitem = start * limit;
	}

	// dump map to comma delimited list
	for (map<int, wstring, cmpByNumber>::const_iterator it = filemap.begin(), end = filemap.end(); it != end; ++it)
	{
		if (counter >= firstitem)
		{
			if (files.length())
				files += L",";

			files += it->second;
		}

		counter++;
		if (limit != -1 && counter >= firstitem + limit)  // if we're paginating and we have reached the limit
			break;
	}

	// return file list
	auto answer = json::value::object();
	answer[L"ReturnValue"] = json::value::string(files);
	request.reply(status_codes::OK, answer);
}

void return_filelist(wstring uri, http_request request)
{
	// ?limit=5&start=2&sorted=1
	int limit = -1;
	int start = 0;
	int sorted = 1;

	std::vector<std::wstring>  paginationsplit;
	boost::split(paginationsplit, uri, boost::is_any_of("?"));
	if (paginationsplit.size() > 1)  // we want a paginated list
	{
		std::vector<std::wstring>  paginationvars;
		boost::split(paginationvars, paginationsplit[1], boost::is_any_of("&"));

		limit = stoi(paginationvars[0].substr(6).c_str());
		start = stoi(paginationvars[1].substr(6).c_str());
		sorted = stoi(paginationvars[2].substr(7).c_str());
	}

	if (!sorted)
	{
		return_unsorted_filelist(uri, request, limit, start);
	}
	else // sort it
	{
		return_sorted_filelist(uri, request, limit, start);
	}
}

void handle_get(http_request request)
{
	TRACE(L"\nhandle GET\n");

	utility::string_t uri = request.absolute_uri().to_string();
	TRACE("\nGET uri: ");
	TRACE(uri.c_str());
	TRACE("\n");

	// parse out id, if any
	std::vector<std::wstring> strs;
	boost::split(strs, uri, boost::is_any_of("/"));
	
	if (strs.size() > 2)
	{
		// return a file
		return_file(strs[2], request);
	}
	else
	{
		// return file list
		return_filelist(uri, request);
	}
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

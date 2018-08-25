// file-broswe-client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/filestream.h>

#pragma comment(lib, "cpprest_2_10d")

using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace std;


void display_json(
	json::value const & jvalue,
	utility::string_t const & prefix)
{
	wcout << prefix << jvalue.serialize() << endl;
}

pplx::task<http_response> make_task_request(
	http_client & client,
	wstring id,
	int limit,
	int page,
	bool sorted
	)
{
	wstring uri = L"/restdemo";
		
	// ?limit=5&start=2&sorted=1
	uri += L"?limit=";
	uri += std::to_wstring(limit);
	uri += L"&start=";
	uri += std::to_wstring(page);
	if (sorted == true)
		uri += L"&sorted=1";
	else
		uri += L"&sorted=0";
	
	return client.request(methods::GET, uri);
}


pplx::task<http_response> make_task_request_file(
	http_client & client,
	concurrency::streams::ostream stream,
	wstring id = L"",
	int limit = -1,
	int page = 0)
{
	wstring uri = L"/restdemo";

	uri += L"/";
	uri += id;

	return client.request(methods::GET, uri);
}

void make_request(
	http_client & client,
	int limit = -1,
	int page = 0,
	bool sorted = true
	)
{
	make_task_request(client, U(""), limit, page, sorted)
		.then([](http_response response)
	{
		std::wcout << L"\nGot a response.\n";

		if (response.status_code() == status_codes::OK)
		{
			return response.extract_json();
		}
		return pplx::task_from_result(json::value());
	})
	.then([](pplx::task<json::value> previousTask)
	{
		try
		{
			display_json(previousTask.get(), L"R: ");
		}
		catch (http_exception const & e)
		{
			wcout << e.what() << endl;
		}
	})
	.wait();
}

void make_request_file(
	http_client & client,
	json::value const & jvalue,
	const wstring id = L"",
	int limit = -1,
	int page = 0)
{
	utility::string_t OutPath = U("C:\\Work\\file-browse\\returned\\");
	OutPath.append(id);

	_wremove(OutPath.c_str());

	pplx::task<void> requestTask = concurrency::streams::fstream::open_ostream(OutPath)
		.then([=](concurrency::streams::ostream stream)
	{
		try
		{
			make_task_request_file((http_client&)client, stream, id, limit, page).then([stream](http_response response)
			{
				status_code code = response.status_code();
				if (code == status_codes::InternalError)
				{
					wcout << U("\nCouldn't transfer file.\n");
					throw exception();
				}

				return response.body().read_to_end(stream.streambuf());
			})
			.then([=](size_t)  // Close the file stream.
			{
				stream.close();
			})
			.wait();

			wcout << U("Finished transferring file.");
		}
		catch (exception e)
		{
			wcout << e.what() << endl;
		}
	});
}

int main()
{
	http_client client(U("http://localhost"));

	auto nullvalue = json::value::null();

	// get all files on server
	make_request(client, -1, 0, false);

	// get files paged
	make_request(client, 5, 0, true);
	make_request(client, 5, 1, true);
	make_request(client, 5, 2, true);
	make_request(client, 5, 3, true);
	make_request(client, 5, 4, true);

	std::wstring filename;
	std::cout << "Enter a file name: ";
	std::getline(std::wcin, filename);

	if (filename.length() > 0)
	{
		make_request_file(client, nullvalue, filename);
	}

	wcout << L"\nEnter a key, any key...";
	std::cin.ignore();

	return 0;
}
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

auto fileStream = std::make_shared<concurrency::streams::ostream>();


void display_json(
	json::value const & jvalue,
	utility::string_t const & prefix)
{
	wcout << prefix << jvalue.serialize() << endl;
}

pplx::task<http_response> make_task_request(
	http_client & client,
	method mtd,
	json::value const & jvalue,
	wstring id = L"",
	int limit = -1,
	int page = 0)
{
	if (mtd == methods::GET)
	{
		wstring uri = L"/restdemo";
		
		if (id.length() > 0)  // file contents
		{
			uri += L"/";
			uri += id;

		}
		else  // file list
		{
			if (limit != -1)
			{
				// ?limit=5&start=2
				uri += L"?limit=";
				uri += std::to_wstring(limit);
				uri += L"&start=";
				uri += std::to_wstring(page);
			}
		}
	
		return client.request(mtd, uri);
	}
	else
		return client.request(mtd, L"/restdemo", jvalue);

}



void make_request(
	http_client & client,
	method mtd,
	json::value const & jvalue,
	wstring id = L"",
	int limit = -1,
	int page = 0)
{
	make_task_request(client, mtd, jvalue, id, limit, page)
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
	method mtd,
	json::value const & jvalue,
	const wstring id = L"",
	int limit = -1,
	int page = 0)
{
	utility::string_t OutPath = U("C:\\Work\\file-browse\\returned\\");
	OutPath.append(id);

	pplx::task<void> requestTask = concurrency::streams::fstream::open_ostream(OutPath)
		.then([=](concurrency::streams::ostream stream)
	{
		*fileStream = stream;

		try
		{
			make_task_request((http_client&)client, mtd, jvalue, id, limit, page).then([](http_response response)
			{
				return response.body().read_to_end(fileStream->streambuf());
			})
			.then([=](size_t)  // Close the file stream.
			{
				fileStream->close();
			})
				.wait();
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

	make_request(client, methods::GET, nullvalue, L"");
	make_request(client, methods::GET, nullvalue, L"", 5, 0);
	make_request(client, methods::GET, nullvalue, L"", 5, 1);
	make_request(client, methods::GET, nullvalue, L"", 5, 2);

	std::wstring filename;
	std::cout << "Enter a file name: ";
	std::getline(std::wcin, filename);

	make_request_file(client, methods::GET, nullvalue, filename);

	wcout << L"\nEnter a key, any key...";
	std::cin.ignore();

	return 0;
}
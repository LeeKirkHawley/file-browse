// file-broswe-client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#pragma comment(lib, "cpprest_2_10d")

using namespace web;
using namespace web::http;
using namespace web::http::client;

#include <iostream>
using namespace std;

void display_json(
	json::value const & jvalue,
	utility::string_t const & prefix)
{
	wcout << prefix << jvalue.serialize() << endl;
}

pplx::task<http_response> make_task_request(
	http_client & client,
	method mtd,
	json::value const & jvalue)
{
	//return (mtd == methods::GET || mtd == methods::HEAD) ?
	//	client.request(mtd, L"/restdemo") :
	//	client.request(mtd, L"/restdemo", jvalue);


	if (jvalue.is_null() == false)
	{
		http_request request(methods::GET);
		request.headers().add(L"MyHeaderField", L"MyHeaderValue");
		request.set_request_uri(L"/restdemo");

		return client.request(request);

	}
	else
	{
		return (mtd == methods::GET || mtd == methods::HEAD) ?
			client.request(mtd, L"/restdemo") :
			client.request(mtd, L"/restdemo", jvalue);
	}
}

void make_request(
	http_client & client,
	method mtd,
	json::value const & jvalue)
{
	make_task_request(client, mtd, jvalue)
		.then([](http_response response)
	{
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

int main()
{
	http_client client(U("http://localhost"));


	auto nullvalue = json::value::null();

	//wcout << L"\nGET (get all values)\n";
	//display_json(nullvalue, L"S: ");
	//make_request(client, methods::GET, nullvalue);

	json::value postData;
	postData[L"id"] = json::value::string(L"1234");
	make_request(client, methods::GET, postData);

	return 0;
}
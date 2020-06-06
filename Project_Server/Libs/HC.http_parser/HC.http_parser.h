#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "../Nlomann.Json/json.hpp"

#define HTTPP_METHOD_GET "GET"
#define HTTPP_METHOD_GETDB "GETDB"
#define HTTPP_METHOD_PUT "PUT"
#define HTTPP_HEADERS_STD "Access-Control-Allow-Origin: null\nConnection: close\n"
#define HTTPP_CL_TYPE_ARD "ARDUINO"
#define HTTPP_CL_TYPE_TERMINAL "TERMINAL"



class HC_http_parser
{
public:
	HC_http_parser();
	~HC_http_parser();
	unsigned short int parse_str(std::string parsing_string);
	std::string	get_json_data();
	std::string get_resp_with_code(std::string json_str, int code = 200);
	std::string get_resp_with_code(std::string json_str, std::string host_id, int code = 200);
	std::string get_resp_with_code(int code);
private:
	std::string JSON_DATA;
};
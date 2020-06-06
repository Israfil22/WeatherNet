#pragma once

#include "HC.http_parser.h"

HC_http_parser::HC_http_parser()
{
	
}

HC_http_parser::~HC_http_parser()
{

}

std::string	HC_http_parser::get_json_data(){
	return this->JSON_DATA;
}

unsigned short int HC_http_parser::parse_str(std::string request){
	std::string delim = "<json_data>";
	if (request.find(delim) == std::string::npos) return 1;
	request = request.substr(request.find(delim) + delim.length());
	if (request.find(delim) == std::string::npos) return 1;
	request = request.substr(0, request.find(delim));
	//std::cout << request;
	this->JSON_DATA = request;
	return 0;
}

std::string HC_http_parser::get_resp_with_code(std::string json_str, std::string host_id, int code){
	std::string NS = "";
	nlohmann::json json_data = nlohmann::json::parse(json_str);
	nlohmann::json json_responce = { {"responce_code", code}, {"host_id", host_id}, {"json_responce", json_data} };
	std::string ret = "HTTP/1.1 200\n" + NS + HTTPP_HEADERS_STD + "\n<json_data>\n" + json_responce.dump() + "\n<json_data>\n";
	return ret;
}

std::string HC_http_parser::get_resp_with_code(std::string json_str, int code){
	std::string NS = "";
	nlohmann::json json_data = nlohmann::json::parse(json_str);
	nlohmann::json json_responce = { {"responce_code", code}, {"json_responce", json_data}};
	std::string ret = "HTTP/1.1 200\n" + NS + HTTPP_HEADERS_STD + "\n<json_data>\n" +  json_responce.dump() + "\n<json_data>\n";
	return ret;
}

std::string HC_http_parser::get_resp_with_code(int code) {
	std::string NS = "";
	std::string ret = "";
	if (code == 418) {
		ret = "HTTP/1.1 418\nConnection: close\n";
		return ret;
	}
	nlohmann::json	json_responce = { {"responce_code", code} };
	ret = "HTTP/1.1 200\n" + NS + HTTPP_HEADERS_STD + "\n<json_data>\n" + json_responce.dump() + "\n<json_data>\n";
	return ret;
}
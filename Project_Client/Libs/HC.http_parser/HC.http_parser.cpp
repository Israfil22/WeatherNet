#pragma once

#include "HC.http_parser.h"

HC_http_parser::HC_http_parser()
{

}

HC_http_parser::~HC_http_parser()
{

}

std::string	HC_http_parser::get_json_data() {
	return this->JSON_DATA;
}


std::vector<std::string> HC_http_parser::split_str(std::string request) {
	std::vector<std::string> splited_strs;
	std::string header;
	while (request != "") {
		if (request.find("\n") == std::string::npos)
			header = request.substr(0, request.length() - 1);
		else
			header = request.substr(0, request.find("\n"));
		request = request.substr(header.length() + 1, request.length() - header.length());
		if (header == "") continue;
		splited_strs.push_back(header);
	}
	return splited_strs;
}

unsigned short int HC_http_parser::parse_str(std::string request) {
	std::vector<std::string> splited_strs = this->split_str(request);
	std::string access_key = "";
	std::string json_data = "";
	bool search_trigger = false;

	for (int i = 0; i < splited_strs.size(); i++) {
		if (splited_strs[i].find("<json_data>") != std::string::npos) {
			search_trigger = !search_trigger; continue;
		}
		if (search_trigger and splited_strs[i].find("<json_data>") == std::string::npos) json_data += splited_strs[i];
		if (!search_trigger and json_data != "") break;
	}
	if (search_trigger) json_data = "";//если не найден закрывающий тег. во избежание ошибок парсинга json.
	this->JSON_DATA = json_data;
	return 0;
}

std::string HC_http_parser::get_resp_with_code(std::string json_str, int code) {
	std::string NS = "";
	nlohmann::json json_responce = { {"responce_code", code}, {"json_data", nlohmann::json::parse(json_str)} };
	std::string ret = "HTTP/1.1 200\n" + NS + HTTPP_HEADERS_STD + "\n<json_data>\n" + json_responce.dump() + "\n<json_data>\n";
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
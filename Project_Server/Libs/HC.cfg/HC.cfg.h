#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>

class HC_cfg
{
	public:
		std::map<std::string, std::string> cfg;
		void process(std::string path_to_file);
		std::vector<std::string> exploded;
	protected:
		std::ifstream fin;
		char Delimiter = '\n';
		std::string pointer = "=";
};
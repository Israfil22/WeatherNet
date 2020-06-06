#include "HC.cfg.h"

void HC_cfg::process(std::string file_path){
	std::string file_content;
	this->fin.open(file_path, std::ifstream::in);
	if (!this->fin.is_open()) throw 1;

	while (!this->fin.eof()) {
		std::getline(this->fin, file_content, this->Delimiter);
		this->exploded.push_back(file_content);
	}
	if (!this->exploded.size()) throw 0;

	for (auto it = this->exploded.begin(); it < this->exploded.end(); it++){
		this->cfg[(*it).substr(0, (*it).find(this->pointer))] = (*it).substr((*it).find(this->pointer)+1, (*it).length() - (*it).find(this->pointer));
	}
}
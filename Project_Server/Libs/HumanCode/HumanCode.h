#pragma once

#include <iostream>
#include <string>
#include <conio.h>

#define HC_arr_size(determined_array) (sizeof(determined_array) / sizeof(determined_array[0]))

class HumanCode
{
public:
	const std::string NS = "";
	HumanCode(int = 10);
	int choose_menu(std::string list[], short int size, int choose_i = 0, std::string header = "");
	bool isequal(double first_var, double second_var);
	bool isequal(double first_var, double second_var, unsigned short int comparison_accuracy);
	void setprecision(double num_for_set);
	void setprecision(unsigned short int precision_size);
	void exit(std::string message = "");
	void pause();
	void cls();
private:
	int EQ_ACC;
};


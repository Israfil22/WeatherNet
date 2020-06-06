#pragma once

#include "HumanCode.h"


HumanCode::HumanCode(int eq_a) {
	this->EQ_ACC = eq_a;
	setlocale(LC_ALL, "Russian");
}

void HumanCode::exit(std::string msg) {
	std::cout << msg << "\n";
	std::cout << "\n\nДля выхода нажмите любую кнопку... ";
	_getch();
}

int HumanCode::choose_menu(std::string list[], short int size, int choose_i, std::string header) {
	const short int KEY_UP = 72;
	const short int KEY_DOWN = 80;
	char act;
	this->cls();
	while (true){
		this->cls();
		if (header != "") {
			std::cout << header << std::endl;
		}
		for (int i = 0; i < size; i++) {
			printf("[%i] ", i + 1);
			std::cout << list[i];
			if (i == choose_i) { printf(" <="); };
			std::cout << std::endl;
		}
		act = _getch();
		if (act == 0 or act == 224) {
			act = _getch();
		}
		switch (act) {
		case KEY_UP:
			if (choose_i > 0) {
				choose_i -= 1;
			}
			break;
		case KEY_DOWN:
			if (choose_i < (size - 1)) {
				choose_i += 1;
			}
			break;
		case 13:
			this->cls();
			return choose_i;
			break;
		}
	}
}

bool HumanCode::isequal(double f_var, double s_var) {
	long double diff;
	long long int comp_var;
	diff = f_var - s_var;
	for (int i = 0; i != EQ_ACC; i++) {
		diff = diff * 10;
	}
	comp_var = diff;
	if (comp_var == 0)	return true;
	else				return false;
}

bool HumanCode::isequal(double f_var, double s_var, unsigned short int comp_acc) {
	long double diff;
	long long int comp_var;
	diff = f_var - s_var;
	for (int i = 0; i != comp_acc; i++) {
		diff = diff * 10;
	}
	comp_var = diff;
	if (comp_var == 0)	return true;
	else				return false;
}

void HumanCode::pause() {
	_getch();
}

void HumanCode::cls() {
	system("CLS");
}

void HumanCode::setprecision(double var) {
	int len, int_var;
	int_var = abs(var);
	len = std::to_string(int_var).length();
	if (!int_var) len -= 1;
	std::cout.setf(std::ios::fixed);
	std::cout.precision(sizeof(var) * 2 - len);
	/*std::cout << var << " : " << len << std::endl;*/
}
void HumanCode::setprecision(unsigned short int var) {
	std::cout.setf(std::ios::fixed);
	std::cout.precision(var);
}

/*setprecision(1.0 / 3.0);
setprecision(-1.0 / 3);
setprecision((1.0 / 3) + 1);
setprecision((-1.0 / 3) - 1);*/


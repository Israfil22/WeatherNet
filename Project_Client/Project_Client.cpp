#include <iostream>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <string>
#include <thread>
#include <chrono>
#include "Libs/HC.http_parser/HC.http_parser.h"
#include "Libs/HumanCode/HumanCode.h"
#include "Libs/Nlomann.Json/json.hpp"

#pragma comment(lib, "ws2_32.lib")

using namespace std;
using nlohmann::json;

HumanCode HC(true);

HC_http_parser http_parser;


/*            НАСТРОЙКИ УСТРОЙСТВА            */
////////////////////////////////////////////////
char	device_name[64]		= "ard_01";
char	device_unique_id[9] = "4e413570";

json	my_indicators	=	{
								{"temp", 35},
								{"fan_rpm", 350}
							};

json	my_required		=	{
								{"temp", 18},
								{"fan_rpm", 1100}
							};

json	my_attributes	=	{
								{"min_temp", 10},
								{"max_temp", 50},
								{"now_temp", my_indicators["temp"]},
								{"min_rpm", 100},
								{"max_rpm", 2000},
								{"now_rpm",	my_indicators["fan_rpm"]},
							};


////////////////////////////////////////////////


/*     НАСТРОЙ ЭТО ПЕРЕД СТАРТОМ     */
int			socket_protocol = IPPROTO_TCP;
int			socket_port		= 9889;
char		socket_ip[15]	= "127.0.0.1";

int			socket_af	= AF_INET;
int			socket_type = SOCK_STREAM;
///////////////////////////////////////
WSADATA			socket_data;
SOCKADDR_IN		socket_addr;
SOCKET			main_socket;
//////////////////////////////////////

const int socket_buffer_size = 2053;

bool handling_trigger = true;

int connect_to_server(string& return_string) {
	char			socket_buffer_cha[socket_buffer_size];
	string			socket_buffer_str;
	string			socket_buffer_str_temp;
	string			connect_request = "";
	int				recv_size = 0;
	int size_of_addr = sizeof(socket_addr);


	if (INVALID_SOCKET == (main_socket = socket(socket_af, socket_type, socket_protocol)))
	{
		return SOCKET_ERROR;
	}

	if (SOCKET_ERROR == connect(main_socket, (SOCKADDR*)& socket_addr, size_of_addr)) {
		return SOCKET_ERROR;
	}
	//////////////////////////////////////////////////
	
	json connect_request_json = 
	{	
		{"access_key", "a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3"}, 
		{"client_type", HTTPP_CL_TYPE_ARD}, 
		{"unique_id", device_unique_id},
		{"name", device_name}
	};

	connect_request = "<json_data>\n" + connect_request_json.dump() + "\n<json_data>\n";

	send(main_socket, connect_request.c_str(), connect_request.length(), NULL);

	json server_responce;

	while (true) {
		recv_size = recv(main_socket, socket_buffer_cha, socket_buffer_size, NULL);
		socket_buffer_str_temp = socket_buffer_cha;
		socket_buffer_str += socket_buffer_str_temp.substr(0, recv_size);
		cout << recv_size;
		if (recv_size != SOCKET_ERROR) {
			if (recv_size < socket_buffer_size) break;
		}
		else {
			cout << recv_size << endl;
		}
	}

	http_parser.parse_str(socket_buffer_str);

	if (http_parser.get_json_data() != ""){
		server_responce = json::parse(http_parser.get_json_data());
		if ((int)server_responce["responce_code"] != 200){
			closesocket(main_socket);
			cout << "\n[ERROR] Ошибка при запросе к серверу: " << server_responce["responce_code"];
			ZeroMemory(&main_socket, sizeof(main_socket));
			HC.pause();
			return 2;
		}
	}
	else{
		closesocket(main_socket);
		cout << "\n[ERROR] Ошибка ответа сервера: не содержит форматированный json";
		ZeroMemory(&main_socket, sizeof(main_socket));
		HC.pause();
		return 1;
	}
	////////////////////////////////////////////////

	cout << endl << socket_buffer_str;
	
	return 0;
}

void output_info() {
	HC.cls();
	cout << "-------Wireless Temp Control-------" << endl << endl;
	cout << "--------------CLIENT---------------" << endl;
	cout << "[Name]   =   " << device_name << endl;
	cout << "[Port]   =   " << socket_port << endl;
	cout << "[IP]     =   " << socket_ip << endl;
	cout << "-----------------------------------" << endl;
}

int main(int argc, char* argv[]) {
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, ".1251");

	system("COLOR 70");
	int				socket_error;
	string			connect_return;
	int				connect_code;
	switch (argc) {
	case 5:
		strcpy_s(device_unique_id, 9, argv[4]);
	case 4:
		strcpy_s(device_name, 64, argv[3]);
	case 3:
		socket_port = atoi(argv[2]);
	case 2:
		strcpy_s(socket_ip, 15, argv[1]);
		break;
	}

	socket_addr.sin_family = socket_af;
	socket_addr.sin_port = htons(socket_port);
	inet_pton(socket_af, socket_ip, &socket_addr.sin_addr);

	bool keep_trigger = true;

	output_info();

	socket_error = WSAStartup(MAKEWORD(2, 2), &socket_data);

	if (socket_error != 0) {
		HC.exit("Произошла ошибка при инциализации сокета");
		WSACleanup();
		return 1;
	}

	if (LOBYTE(socket_data.wVersion) != 2 || HIBYTE(socket_data.wVersion) != 2) {
		HC.exit("Невозможно найти версию указанной библиотеки\n");
		WSACleanup();
		return 2;
	}

	connect_code = connect_to_server(connect_return);
	if (!connect_code) {
		cout << "[INFO] Успешное подключение к серверу" << endl;
		cout << "[SYSTEM] Приветствие сервера: " << connect_return << endl;
	};

	int recv_size = 0;

	string socket_buffer_str_temp, socket_buffer_str;
	char socket_buffer_cha[socket_buffer_size];

	json server_request;
	json responce_json;

	string responce_to_server;

	while (handling_trigger) {
		HC.cls();
		cout << my_required.dump(4) << endl;
		ZeroMemory(&socket_buffer_str, sizeof(socket_buffer_str));
		//------------------Чтение данных из сокета------------------//
		while (true){
			recv_size = recv(main_socket, socket_buffer_cha, socket_buffer_size, NULL);
			if (recv_size != SOCKET_ERROR) {
				socket_buffer_str_temp = socket_buffer_cha;
				socket_buffer_str += socket_buffer_str_temp.substr(0, recv_size);
				if (recv_size < socket_buffer_size) break;
			}
			else {
				break;
			}
		}

		cout << "Данные прочитал\n\n";

		//-----------------Обработка json из сокета-----------------//
		http_parser.parse_str(socket_buffer_str);

		cout << socket_buffer_str;
		if (http_parser.get_json_data() != "") {
			server_request = json::parse(http_parser.get_json_data());
			//------Если отправить индикаторы------//
			if (server_request["request_type"] == HTTPP_METHOD_GET){	
				responce_json = {
					{"responce_code", 200},
					{"indicators", my_indicators.dump()}
				};
			}
			//------Если установить значения------//
			else if (server_request["request_type"] == HTTPP_METHOD_PUT) {

				for (auto jo = my_required.begin(); jo != my_required.end(); ++jo){
					if (server_request["required_values"].contains(jo.key())) {
						my_required[jo.key()] = server_request["required_values"][jo.key()];
					}
				}

				responce_json = {
					{"responce_code", 200}
				};
			}
			else if (server_request["request_type"] == HTTPP_METHOD_GETDB){

				responce_json = {
					{"responce_code", 200},
					{"attributes", my_attributes}
				};
			}
			//----------Обработка ошибки----------//
			else{
				responce_json = {
					{"responce_code", 851}
				};
			}
		}
		else{
			responce_json = {
					{"responce_code", 418}
			};
			cout << "\n[ERROR] Возникла ошибка при обработке запроса: [" << 418 << "]\n";
			cout << "\n\n" << "Нажмите любую кнопку для продолжения\n";
			HC.pause();
		}

		responce_to_server = "<json_data>\n" + responce_json.dump() + HC.NS + "\n<json_data>\n";
		send(main_socket, responce_to_server.c_str(), responce_to_server.length(), NULL);
	}

	HC.exit("");
}

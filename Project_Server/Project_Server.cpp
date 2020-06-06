#include <iostream>
#include <stdio.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <map>

#include "Libs/HumanCode/HumanCode.h"
#include "Libs/Nlomann.Json/json.hpp"
#include "Libs/Sqlite3/sqlite3.h"
#include "Libs/HC.http_parser/HC.http_parser.h"
#include "Libs/HC.mutex/HC.mutex.h"
#include "Libs/HC.cfg/HC.cfg.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

HumanCode HC(false);
HC_http_parser http_parser;

using nlohmann::json;

/*     НАСТРОЙ ЭТО ПЕРЕД СТАРТОМ     */
///////////////////////////////////////
string		HOST_ID = "55b20a4b";

int			socket_port = 9889;
string		socket_ip = "192.168.0.2";

const short int			socket_max_connections = 10;
const short int			socket_buffer_size = 2053;
unsigned    int			socket_timeout = 200;
u_long					block_mode = 1; // !=0 non-blocking mode
DWORD					socket_opt_timeout = 500; //Видимо на системном уровне, при установке параметра задержка n+500мс

unsigned short int	dis_delay_sec		= 7;
unsigned int		dis_delay_microsec	= 0;

///////////////////////////////////////
const char db_file[] = "database.db";

sqlite3* db_handle;

string sql_json_str;

///////////////////////////////////////
int			socket_af = AF_INET;
int			socket_type = SOCK_STREAM;
int			socket_protocol = IPPROTO_TCP;
///////////////////////////////////////
WSADATA			socket_data, notif_main_socket_data;
SOCKET			main_socket, notif_main_socket, notif_client_socket;
SOCKADDR_IN		socket_addr, notif_main_socket_addr;
int				socket_error, notif_main_socket_error;

string info_ip;
string info_port;
string info_notify_ip;
string info_notiry_port;
///////////////////////////////////////

struct client_struct {
	string		UID;
	string		NAME;
	SOCKET		SOCKET;
	SOCKADDR_IN	ADDR;
	string		REQUEST;
	string		RESPONCE;
	string		NOTIFY;
};


vector<client_struct> clients_connections;



vector<json> notification_history;

#define array_size(ar) (sizeof(ar) / sizeof(ar[0]))
#define NULL_SOCKET_PROP ""
#define SYS_NPLWL_DELAY 10

static int sql_exec_callback(void* NotUsed, int argc, char** argv, char** ColName) {
	if (argc == 2) sql_json_str = argv[1];
	//for (int i = 0; i < argc; i++){ cout << i << "\t" << argv[i] << endl; }
	return 0;
}

int accept_connections(bool& trigger_accept, vector<client_struct>& clients_connections) {


	SOCKET			socket_client;
	SOCKADDR_IN		client_addr;
	int				size_of_addr = sizeof(client_addr);
	int				recv_size = 0;

	short int		WSAerror = 0;
	bool			need_close_connection = false;
	/////////////////////////////////////////////
	char			socket_buffer_cha[socket_buffer_size];
	string			socket_buffer_str;
	string			socket_buffer_str_temp;
	string			client_responce;
	json			socket_client_json;
	json			response_clients_db_unit;
	json			sql_json_devices;
	json			request_ard_json;
	string			request_ard_str;
	client_struct	client_unit;

	int db_err;
	char* ErrMsg = 0;
	string client_acess_key;
	string sql_request;
	string responce_to_client;

	while (trigger_accept)
	{
		recv_size = 0;
		socket_client = accept(main_socket, (SOCKADDR*)& client_addr, &size_of_addr);
		if (socket_client != INVALID_SOCKET){
			ZeroMemory(&client_unit, sizeof(client_unit));
			ZeroMemory(&socket_buffer_str, sizeof(socket_buffer_str));
			ZeroMemory(&responce_to_client, sizeof(responce_to_client));
			ZeroMemory(&socket_client_json, sizeof(socket_client_json));
			

			need_close_connection = false;

			setsockopt(socket_client, SOL_SOCKET, SO_RCVTIMEO, (char*)& socket_opt_timeout, sizeof(socket_opt_timeout));

			while (true) {
				recv_size = recv(socket_client, socket_buffer_cha, socket_buffer_size, NULL);
				if (recv_size != SOCKET_ERROR) {
					socket_buffer_str_temp = socket_buffer_cha;
					socket_buffer_str += socket_buffer_str_temp.substr(0, recv_size);
					if (recv_size < socket_buffer_size) break;
				}
				else {
					need_close_connection = true;
					break;
				}
			}
			

			if (need_close_connection) {
				shutdown(socket_client, SD_SEND);
				closesocket(socket_client);
				continue;
			}

			//Проверка присутствия json даты в буфере

			http_parser.parse_str(socket_buffer_str);

			//cout << endl << endl << http_parser.get_json_data();
			
			if (http_parser.get_json_data() == "") {
				send(socket_client, http_parser.get_resp_with_code(418).c_str(), http_parser.get_resp_with_code(418).length(), NULL);
				shutdown(socket_client, SD_SEND);
				closesocket(socket_client);
				continue;
			}

			socket_client_json = json::parse(http_parser.get_json_data());

			//cout << socket_client_json;

			//Проверка является ли наш клиент одним из зарег-ным типом клиента

			if (socket_client_json["client_type"].get<string>() != HTTPP_CL_TYPE_ARD and socket_client_json["client_type"].get<string>() != HTTPP_CL_TYPE_TERMINAL){
				client_responce = http_parser.get_resp_with_code(601);
				send(socket_client, client_responce.c_str(), client_responce.length(), NULL);
				shutdown(socket_client, SD_SEND);
				closesocket(socket_client);
				continue;
			}

			//Проверка на присутствие access_key в БД #1
			sql_json_str = NULL_SOCKET_PROP; //Обнуление поиска в БД.
			sql_request = "SELECT * FROM ACCESS_DB WHERE access_key=\"" + socket_client_json["access_key"].get<string>() + "\"";
			if (sqlite3_exec(db_handle, sql_request.c_str(), sql_exec_callback, NULL, &ErrMsg) != SQLITE_OK) {
				std::cout << "[Ошибка] Sql: " << ErrMsg;
				HC.exit();
				sqlite3_close(db_handle);
				WSACleanup();
				return 1;
			}
			

			//Продолжение проверки #2
			if (sql_json_str == NULL_SOCKET_PROP){
				client_responce = http_parser.get_resp_with_code(602);
				send(socket_client, client_responce.c_str(), client_responce.length(), NULL);
				shutdown(socket_client, SD_SEND);
				closesocket(socket_client);
				continue;
			}


			//Начало обработки

			//Ветвление в зависимости от типа клиента

			//-----------------Обработка подключения терминалов-----------------//
			if (socket_client_json["client_type"].get<string>() == HTTPP_CL_TYPE_TERMINAL){
				if (socket_client_json["method"].get<string>() == HTTPP_METHOD_GET or socket_client_json["method"].get<string>() == HTTPP_METHOD_GETDB){
					sql_json_devices = json::parse(sql_json_str);
					map<string, int> clients_matches;
					//Ветвление методов осуществляется составлением запроса описанного ниже
					json json_request = {
						{"request_type", socket_client_json["method"].get<string>()}
					};	
					string request_str = "<json_data>\n" + json_request.dump() + "\n<json_data>\n";
					//Поиск подключенных арудино в соответствии с db
					//Установка запроса для каждой найденной платы
					try{
						for (auto key = sql_json_devices.begin(); key != sql_json_devices.end(); key++) {
							for (int i = 0; i < clients_connections.size(); i++) {
								if (clients_connections.at(i).UID == *key) {
									clients_connections.at(i).REQUEST = request_str;
									clients_matches.insert(pair<string, int>(clients_connections.at(i).UID, i));
								}
							}
						}
					}
					catch (const std::out_of_range& ex){}
					socket_client_json = {
					};
					//cout << "\n--------------------------------------------------------------\n";
					//cout << socket_client_json << endl << endl;
					int i = 0;
					//cout << "size" << clients_matches.size() << endl;
					try{
						while (clients_matches.size()){
							for (auto key = clients_matches.begin(); key != clients_matches.end(); key++){
								//cout << "da" << endl;
								//cout << "responce: " << clients_connections.at(key->second).RESPONCE << " END" << endl;
								if (clients_connections.at(key->second).RESPONCE != NULL_SOCKET_PROP){
									http_parser.parse_str(clients_connections.at(key->second).RESPONCE);
									if (http_parser.get_json_data() != NULL_SOCKET_PROP){
										json unit_json = json::parse(http_parser.get_json_data());
										//cout << endl << endl << unit_json["attributes"].dump(4) << endl << endl;
										unit_json["attributes"]["device_name"] = clients_connections.at(key->second).NAME;
										socket_client_json[clients_connections.at(key->second).UID] = unit_json;
										ZeroMemory(&clients_connections.at(key->second).RESPONCE, sizeof(clients_connections.at(key->second).RESPONCE));
										clients_matches.erase(key);
										break;
									}
								}
								else{
									this_thread::sleep_for(chrono::microseconds(SYS_NPLWL_DELAY));
								}
							}
						}
					}
					catch (const std::out_of_range& ex){}
					//cout << "exited" << endl;
					//cout << socket_client_json.dump(4) << endl;

					//cout << socket_client_json;
					if (json_request["request_type"] == HTTPP_METHOD_GETDB)
						client_responce = http_parser.get_resp_with_code(socket_client_json.dump(), HOST_ID);
					else
						client_responce = http_parser.get_resp_with_code(socket_client_json.dump());

					//cout << client_responce;
					send(socket_client, client_responce.c_str(), client_responce.length(), NULL);
					shutdown(socket_client, SD_SEND);
					closesocket(socket_client);
				}
				else if (socket_client_json["method"].get<string>() == HTTPP_METHOD_PUT) {
					//Отправить required-значения
					sql_json_devices = json::parse(sql_json_str);
					if (find(sql_json_devices.begin(), sql_json_devices.end(), socket_client_json["unique_id"].get<string>()) != sql_json_devices.end()){
						sql_json_devices = json::parse(sql_json_str);
						map<string, int> clients_matches;
						json json_request = {
							{"request_type", HTTPP_METHOD_PUT},
							{"required_values", socket_client_json["required_values"]}
						};
						string request_str = "<json_data>\n" + json_request.dump() + "\n<json_data>\n";
						//Поиск арудино в соответствии с db и осуществленными подключениями
						//Установка ошибки в ответ, если не будет найдено соответствие
						int client_vectors_i = -1;
						bool break_it = false;
						try{
							for (auto key = sql_json_devices.begin(); key != sql_json_devices.end(); key++) {
								for (int i = 0; i < clients_connections.size(); i++) {
									if (clients_connections.at(i).UID == *key and clients_connections.at(i).UID == socket_client_json["unique_id"].get<string>()) {
										clients_connections.at(i).REQUEST = request_str;
										client_vectors_i = i;
										break_it = true;
										break;
									}
								}
								if (break_it) break;
							}
						}
						catch (const std::out_of_range& ex) {}
						if (client_vectors_i != -1) {
							try {
								while (clients_connections.at(client_vectors_i).RESPONCE == NULL_SOCKET_PROP) this_thread::sleep_for(chrono::microseconds(SYS_NPLWL_DELAY));
								http_parser.parse_str(clients_connections.at(client_vectors_i).RESPONCE);
								ZeroMemory(&clients_connections.at(client_vectors_i).RESPONCE, sizeof(clients_connections.at(client_vectors_i).RESPONCE));
							}
							catch (const std::out_of_range& ex) {
								http_parser.parse_str("");
							}
							if (http_parser.get_json_data() != NULL_SOCKET_PROP){
								socket_client_json = json::parse(http_parser.get_json_data());
								//cout << socket_client_json.dump(4) << endl;
								if (socket_client_json["responce_code"].get<int>() == 200) {
									client_responce = http_parser.get_resp_with_code(200);
								}
								else{
									client_responce = http_parser.get_resp_with_code(792);
								}
							}
							else{
								client_responce = http_parser.get_resp_with_code(791);
							}
						}
						else{
							client_responce = http_parser.get_resp_with_code(703);
						}
						send(socket_client, client_responce.c_str(), client_responce.length(), NULL);
						shutdown(socket_client, SD_SEND);
						closesocket(socket_client);
						continue;
					}
					else{
						client_responce = http_parser.get_resp_with_code(702);
					}
				}
				else {// Некорректный метод
					client_responce = http_parser.get_resp_with_code(701);
					send(socket_client, client_responce.c_str(), client_responce.length(), NULL);
					shutdown(socket_client, SD_SEND);
					closesocket(socket_client);
					continue;
				}
			}
			//-----------------Обработка подключения контроллеров-----------------//
			else if (socket_client_json["client_type"].get<string>() == HTTPP_CL_TYPE_ARD){
				if (clients_connections.size() < socket_max_connections){
					sql_json_devices = json::parse(sql_json_str);	
				//	cout << endl << endl << sql_json_devices.dump(4);
				//	cout << "[" << (find(sql_json_devices.begin(), sql_json_devices.end(), socket_client_json["unique_id"].get<string>()) != sql_json_devices.end()) << "]";
					if (find(sql_json_devices.begin(), sql_json_devices.end(), socket_client_json["unique_id"].get<string>()) == sql_json_devices.end()) {
						responce_to_client = http_parser.get_resp_with_code(802);
						send(socket_client, responce_to_client.c_str(), responce_to_client.length(), NULL);
						shutdown(socket_client, SD_SEND);
						closesocket(socket_client);
						continue;
					}

					responce_to_client = http_parser.get_resp_with_code(200);
					send(socket_client, responce_to_client.c_str(), responce_to_client.length(), NULL);

					client_unit.SOCKET	= socket_client;
					client_unit.ADDR	= client_addr;
					client_unit.NAME	= socket_client_json["name"].get<string>();
					client_unit.UID		= socket_client_json["unique_id"].get<string>();
					client_unit.REQUEST = NULL_SOCKET_PROP;
					client_unit.NOTIFY = NULL_SOCKET_PROP;
					setsockopt(socket_client, SOL_SOCKET, SO_RCVTIMEO, (char*)& socket_opt_timeout, sizeof(socket_opt_timeout));
					//ioctlsocket(client_unit.SOCKET, FIONBIO, &block_mode);
					cout << "\nПодключение нового клиента [" << client_unit.NAME << "]\n";
					clients_connections.push_back(client_unit);
				}
				else{
					client_responce = http_parser.get_resp_with_code(801);
					send(socket_client, client_responce.c_str(), client_responce.length(), NULL);
					shutdown(socket_client, SD_SEND);
					closesocket(socket_client);
					continue;
				}
			}
		}
	}
	std::cout << "Завершение приёма подключений!" << endl;
	return 0;
}

volatile bool locker_tr = false;
volatile bool back_locker_tr = false;

void handle_connections(bool& trigger_handle){
	char			socket_buffer_cha[socket_buffer_size];
	string			socket_buffer_str;
	string			socket_buffer_str_temp;
	int				recv_size;

	timeval timer;
	timer.tv_sec	= dis_delay_sec;
	timer.tv_usec	= dis_delay_microsec;

	fd_set	select_socket;
	int		select_resp = 0;

	int k = 0;

	string socket_alive_mesage = "alive";

	short int		WSAError;
	while (trigger_handle){
		if (!clients_connections.size()) this_thread::sleep_for(chrono::microseconds(SYS_NPLWL_DELAY));
		back_locker_tr = true;
		for (int i = 0; i < clients_connections.size(); i++){
			this_thread::sleep_for(chrono::microseconds(SYS_NPLWL_DELAY)); // No payload
			//cout << "[" << i << "]" << endl;
			//cout << "i:" << i << "\t\tsize:" << clients_connections.size();
			socket_buffer_str.clear();
			select_resp = send(clients_connections[i].SOCKET, socket_alive_mesage.c_str(), socket_alive_mesage.length(), NULL);
			if (select_resp != SOCKET_ERROR){
				//cout << "send: " << select_resp << "\t";
				FD_ZERO(&select_socket);
				FD_SET(clients_connections[i].SOCKET, &select_socket);
				select_resp = select(NULL, &select_socket, NULL, NULL, &timer);
				//cout << "select: " << select_resp << "\t";
				if (select_resp > 0){
					while (true){
						recv_size = recv(clients_connections[i].SOCKET, socket_buffer_cha, socket_buffer_size, NULL);
						if (recv_size != SOCKET_ERROR) {
							socket_buffer_str_temp = socket_buffer_cha;
							socket_buffer_str += socket_buffer_str_temp.substr(0, recv_size);
							if (recv_size < socket_buffer_size) break;
						}
						else{
							break;
						}
					}
					//cout << "recv: " << recv_size << "\t" << "sockbuf: "  << socket_buffer_str << endl << endl;
					if (recv_size > 0){
						if (socket_buffer_str != socket_alive_mesage){
							//
							if (socket_buffer_str.find("<json_data>") != std::string::npos){
								http_parser.parse_str(socket_buffer_str);
								if (http_parser.get_json_data() != NULL_SOCKET_PROP){
									json device_json = json::parse(http_parser.get_json_data());
									//cout << device_json;
									if (device_json.find("notify_code") != device_json.end()){
										clients_connections[i].NOTIFY = device_json["notify_code"].get<string>();
									}
									if (device_json.find("responce_code") != device_json.end()){
										clients_connections[i].RESPONCE = socket_buffer_str;
									}
								}
							}
						}
					}
					else{
						cout << "\nОтключен клиент(нет ответа) " << clients_connections[i].NAME << "\t" << clients_connections[i].UID;
						closesocket(clients_connections[i].SOCKET);
						clients_connections.erase(clients_connections.begin() + i);
						break;
					}
				}
				else{
					cout << "\nОтключен клиент [" << clients_connections[i].NAME << " | " << clients_connections[i].UID << "]";
					closesocket(clients_connections[i].SOCKET);
					clients_connections.erase(clients_connections.begin() + i);
					break;
				}
			}
			else{
				cout << "\nОтключен клиент [" << clients_connections[i].NAME << " | " << clients_connections[i].UID << "]";
				closesocket(clients_connections[i].SOCKET);
				clients_connections.erase(clients_connections.begin() + i);
				break;
			}
			if (clients_connections[i].REQUEST != NULL_SOCKET_PROP) {
				send(clients_connections[i].SOCKET, clients_connections[i].REQUEST.c_str(), clients_connections[i].REQUEST.length(), NULL);
				ZeroMemory(&clients_connections[i].REQUEST, sizeof(clients_connections[i].REQUEST));
			}
		}
		if (notif_client_socket != NULL){
			recv_size = recv(notif_client_socket, socket_buffer_cha, socket_buffer_size, NULL);
			if (recv_size == SOCKET_ERROR) {
				WSAError = WSAGetLastError();
				if (WSAError != WSAEWOULDBLOCK){
					closesocket(notif_client_socket);
					ZeroMemory(&notif_client_socket, sizeof(notif_client_socket));
					cout << "\nСлужба уведомлений отключена." << endl;
				}
			}
			if (recv_size == 0) {
				closesocket(notif_client_socket);
				ZeroMemory(&notif_client_socket, sizeof(notif_client_socket));
				cout << "\nСлужба уведомлений отключена." << endl;
			}
		}
		back_locker_tr = false;
		auto locker = HC_locker(&locker_tr);
	}
}

void handle_notify() {
	while (notif_client_socket != NULL) {
		this_thread::sleep_for(chrono::microseconds(SYS_NPLWL_DELAY));	
		try{
			for (unsigned int it = 0; it < clients_connections.size(); it++) {
				if (clients_connections.at(it).NOTIFY != NULL_SOCKET_PROP) {
					json notification = {
						{"uid", clients_connections.at(it).UID},
						{"name", clients_connections.at(it).NAME},
						{"notify_code", clients_connections.at(it).NOTIFY}
					};

					string notification_str = notification.dump();
					send(notif_client_socket, notification_str.c_str(), notification_str.length(), NULL);
					ZeroMemory(&clients_connections.at(it).NOTIFY, sizeof(clients_connections.at(it).NOTIFY));
				}
			}
		}
		catch (const std::out_of_range& ex) {

		}
	}
}

void accept_notify_listener(bool& trigger_notify_accept){
	SOCKET			socket_client;
	SOCKADDR_IN		client_addr;
	int				size_of_addr = sizeof(client_addr);

	ZeroMemory(&notif_client_socket, sizeof(notif_client_socket));
	while(trigger_notify_accept){
		if (notif_client_socket != NULL) { this_thread::sleep_for(chrono::microseconds(SYS_NPLWL_DELAY));  continue; };
		socket_client = accept(notif_main_socket, (SOCKADDR*)& client_addr, &size_of_addr);
		if (socket_client != INVALID_SOCKET){
			notif_client_socket = socket_client;
			ioctlsocket(notif_client_socket, FIONBIO, &block_mode);
			thread thread_notify (handle_notify);
			thread_notify.detach();
			cout << "\nПодключена служба уведомлений." << endl;
		}
	}
}

void outputinfo(string command, bool ex = true, short int choose = 0) {
	int		clients_counter = 0;
	char	client_ip[INET_ADDRSTRLEN];
	HC.cls();
	cout << "-------------WeatherNet [S]-------------"	<< endl;
	printf("-------------ID  [%s]-------------\n", HOST_ID.c_str());
	cout << "[IP]\t\t=   " << info_port				<< endl;
	cout << "[Port]\t\t=   " << info_ip					<< endl;
	cout << "----------------------------------------"	<< endl;
	if (notif_main_socket != NULL){
		if (notif_client_socket == NULL)	cout << "[Ожидаение подключения]" << endl;
		else								cout << "[Служба подключена]" << endl;
	cout << "[N][IP]\t\t=   " << info_notify_ip		<< endl;
	cout << "[N][Port]\t=   " << info_notiry_port	<< endl;
	cout << "----------------------------------------" << endl;
	}
	for (int i = 0; i < clients_connections.size(); i++) {
			clients_counter++;
			inet_ntop(AF_INET, &(clients_connections[i].ADDR.sin_addr), client_ip, INET_ADDRSTRLEN);
			cout << "[Клиент]" << (clients_connections[i].REQUEST != NULL_SOCKET_PROP ? "(!)" : "") << "ID: " << clients_connections[i].UID << " Name: " << clients_connections[i].NAME << " IP: " << client_ip << "\t";
			if (ex == false and choose == i) {
				cout << "<=";
			}
			cout << endl;
	}
	if (clients_counter > 0){
		cout << "Количество клиентов: " << clients_counter << endl;
		cout << "----------------------------------------" << endl;
	}
	if (ex == true) {
		cout << "Исполнить: " << command;
	}
}

int choose_search(vector<client_struct> clients_connections, int current, short int key) {
	if (key == 72) {//UP
		if (current > 0) --current;
	}
	else { // DOWN
		if (current < clients_connections.size() - 1) ++current;
	}
	return current;
}

int main(int argc, char* argv[]){
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, ".1251");

	system("COLOR 70");
	string title = "TITLE [SERVER] WeatherNet ^|"+HOST_ID+"^|";
	system(title.c_str());

	string path_to_cfg;
	if (argc > 1) path_to_cfg = argv[1];

	path_to_cfg = "D:\\Work Folder\\Project\\Project_Server\\Release\\config.cfg";

	HC_cfg config_file;
	
	cout << "[WeatherNet] Добро пожаловать." << endl;
	cout << "----------------------------------------" << endl;
	/*-------------------------------------------------*/
	//Настройка главного сокета
	socket_addr.sin_family = socket_af;
	socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		info_ip = "Все доступные адреса";
	socket_addr.sin_port = htons(socket_port);
		info_port = socket_port;
	/*-------------------------------------------------*/
	//Настройка сокета оповещений
	notif_main_socket_addr.sin_family = socket_af;
	inet_pton(socket_af, "127.0.0.1", &notif_main_socket_addr.sin_addr);
		info_notify_ip = "127.0.0.1";
	notif_main_socket_addr.sin_port = htons(socket_port + 10);
		info_notiry_port = socket_port;
	try{
		config_file.process(path_to_cfg);
		if (config_file.cfg.find("ip") != config_file.cfg.end()){
			if (config_file.cfg["ip"] == "ANY") {
				socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
				info_ip = "Все доступные адреса";
			}
			else {
				inet_pton(socket_af, config_file.cfg["ip"].c_str(), &socket_addr.sin_addr);
				info_ip = config_file.cfg["ip"];
			}
		}
		if (config_file.cfg.find("port") != config_file.cfg.end()) {
			socket_addr.sin_port = htons(atoi(config_file.cfg["port"].c_str()));
			info_port = config_file.cfg["port"];
		}
		//////////////////////////////////////////////////////////////////////////////////////
		if (config_file.cfg.find("n_ip") != config_file.cfg.end()){
			if (config_file.cfg["n_ip"] == "ANY") {
				notif_main_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
				info_notify_ip = "Все досупные адреса";
			}
			else {
				inet_pton(socket_af, config_file.cfg["n_ip"].c_str(), &notif_main_socket_addr.sin_addr);
				info_notify_ip = config_file.cfg["n_ip"];
			}
		}
		if (config_file.cfg.find("n_port") != config_file.cfg.end()) {
			notif_main_socket_addr.sin_port = htons(atoi(config_file.cfg["n_port"].c_str()));
			info_notiry_port = config_file.cfg["n_port"];
		}
	}
	catch (int ex){
		if (ex == 1) cout << "[INFO] Конфигурационный файл не найден.\n\t\tПрименяются стандартные параметры." <<endl;
		if (ex == 0) cout << "[INFO] Конфигурационный файл не содержит настроек.\n\t\tПрименяются стандартные параметры." << endl;
	}

	ZeroMemory(&notif_main_socket, sizeof(notif_main_socket));
	ZeroMemory(&notif_client_socket, sizeof(notif_client_socket));

	//Главная структура
	bool trigger_main = true;
	bool trigger_accept = true;
	bool trigger_handle = true;
	bool trigger_notify_accept = true;


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
	else {
		cout << "[INFO] WinSock32.dll v" << to_string(LOBYTE(socket_data.wVersion)) << "." << to_string(HIBYTE(socket_data.wVersion)) << " загружена" << endl;
	}

	if (INVALID_SOCKET == (main_socket = socket(socket_af, socket_type, socket_protocol)))
	{
		socket_error = WSAGetLastError();
		HC.exit("[Ошибка] Ошибка при создании сокета(" + to_string(socket_error) + ")");
		WSACleanup();
		return 3;
	}
	else {
		cout << "[INFO] Сокет успешно создан" << endl;
	}

	if (SOCKET_ERROR == ::bind(main_socket, (SOCKADDR*)& socket_addr, sizeof(socket_addr))) {
		socket_error = WSAGetLastError();
		HC.exit("[Ошибка] Ошибка при привязке сокета(" + to_string(socket_error) + ")");
		WSACleanup();
		return 4;
	}
	else {
		cout << "[INFO] Сокет успешно привязан" << endl;
	}

	if (sqlite3_open(db_file, &db_handle) != SQLITE_OK) {
		cout << sqlite3_errmsg(db_handle);
		HC.exit("[ERROR]Ошибка при подключениии ДБ");
		sqlite3_close(db_handle);
		WSACleanup();
	}
	else {
		cout << "[INFO] База данных успешна подключена" << endl;
	}

	if (SOCKET_ERROR == listen(main_socket, socket_max_connections * 2)) {
		socket_error = WSAGetLastError();
		HC.exit("[Ошибка] Ошибка при прослушивании сокета(" + to_string(socket_error) + ")");
		WSACleanup();
		return 5;
	}
	else
	{
		cout << "[INFO] Сокет успешно прослушивается и ожидает пользователей...	" << endl;
	}

	cout << "\n[SYSTEM] Все операции успешно завершены! Нажмите любую кнопку для продолжения!";

	thread th_accept(accept_connections, std::ref(trigger_accept), std::ref(clients_connections));
	th_accept.detach();
	
	thread th_handle(handle_connections, std::ref(trigger_handle));
	th_handle.detach();

	///////////////////////////////////////////////////////
	string	command = "";
	char	input;
	char	act;

	bool	choose_trigger;
	short int choose_i;
	short int cl_arr_size;
	string	act_list[] = {"Вывести буфер", "Отключить", "Запрос[GET]", "Добавить в дамп"};
	short int choosen_act;
	string not_allowed_to_pause = HC.NS + "choose" + "";

	json json_request;
	string request_str;

	int recv_size = 0;
	char socket_buffer_cha[socket_buffer_size];
	string socket_buffer_str_temp;
	string socket_buffer_str;

	string get_req_uid;


	while (trigger_main) {
		input = _getch();
		switch (input) {
		case 8:
			if (command.length() > 0) {
				command = command.substr(0,command.length() - 1);
			}
			break;
		case 35:
			if (command == "cancel_accept"){
				if (trigger_accept) {
					trigger_accept = false;
					cout << "\n[INFO] Принятие подключений приостановлено";
					//th_accept.~thread(); // не работает
				}
				else
					cout << "\n[ERROR] Принятие подключений не активно";
			}
			else if (command == "start_accept") {
				if (!trigger_accept){
					thread th_accept(accept_connections, std::ref(trigger_accept), std::ref(clients_connections));
					th_accept.detach();
					cout << "\n[INFO] Принятие подключений возобновлено";
				}
				else 
					cout << "\n[ERROR] Принятие подключений уже запущено";
			}
			else if (command == "start_handle") {
				if (!trigger_handle){
					thread th_handle(handle_connections, std::ref(trigger_handle));
					th_handle.detach();
					cout << "\n[INFO] Обработка подключений возобновлена";
				}
				else 
					cout << "\n[ERROR] Обработка подключений уже запущена";
			}
			else if (command == "stop_handle"){
				if (trigger_handle){
					trigger_handle = false;
					cout << "\n[INFO] Обработка подключений приостановлена";
				}
				else 
					cout << "\n[ERROR] Обработка подключений не активна";
			}
			else if (command == "exit") {
				trigger_main = false;
			}
			else if (command == "enable_notify"){
				notif_main_socket_error = WSAStartup(MAKEWORD(2, 2), &notif_main_socket_data);
				notif_main_socket = socket(socket_af, socket_type, socket_protocol);
				if (::bind(notif_main_socket, (SOCKADDR*)& notif_main_socket_addr, sizeof(notif_main_socket_addr)) != SOCKET_ERROR) {
					listen(notif_main_socket, 1);
					cout << "\nСокет успешно создан. Ожидание подключения.";
					thread th_notify_accept(accept_notify_listener, std::ref(trigger_notify_accept));
					th_notify_accept.detach();
				}
				else cout << "[ERROR] Ошибка при привязке сокет сообщений: " << WSAGetLastError();
			}
			else if (command == "handle_notify") {
				handle_notify();
			}
			else if (command == "send_notify"){
				if (notif_main_socket != NULL){
					cout << "\nВведите оповещение: ";
					string notify_msg;
					getline(cin, notify_msg);
					if (send(notif_client_socket, notify_msg.c_str(), notify_msg.length(), NULL) != SOCKET_ERROR)
						cout << "\nОповещение отправлено";
					else
						cout << "\nОшибка при отправке оповещения";
				}
				else
					cout << "\nОповещения отключены";
			}
			else if (command == "choose"){
				if (clients_connections.size() == 0) {
					cout << "\n[ERROR] Нет подключенных пользователей!" << endl;
					command = "";
					continue;
				}
				choose_trigger = true;
				choose_i = 0;
				while (choose_trigger) {
					outputinfo(command, false, choose_i);
					act = _getch();
					//cout << "fir_getch:" << (int)act << "\t";
					if (act == 0 or act == 224) {
						act = _getch();
					}
					//cout << "sec_getch:" << (int)act << endl;
					//this_thread::sleep_for(chrono::milliseconds(1000));
					switch (act) {
					case 80:
					case 72:
						choose_i = choose_search(clients_connections, choose_i, act);
						break;
					case 27:
						choose_trigger = false;
						continue;
						break;
					case 13:
						choosen_act = HC.choose_menu(act_list, 4, 0, "Действие с клиентом:");
						//cout << choosen_act;
						if (choosen_act == 0){
							HC.cls();
							cout << clients_connections[choose_i].REQUEST;
						}
						else if (choosen_act == 1) {
							locker_tr = true;	//Включить mutex
							//cout << "Зашёл в мьютекс";
							auto back_locker = HC_locker(&back_locker_tr);

							//cout << "Вышел из мьютекса";
							closesocket(clients_connections[choose_i].SOCKET);
							clients_connections.erase(clients_connections.begin() + choose_i);
							locker_tr = false; //Выключить mutex
						}
						else if (choosen_act == 2) {
							json_request = {
									{"request_type", HTTPP_METHOD_GET}
							};
							request_str = "<json_data>\n" + json_request.dump() + "\n<json_data>\n";
							clients_connections[choose_i].REQUEST = request_str;
							//Ожидание ответа
							//while (clients_connections[choose_i].RESPONCE == NULL_SOCKET_PROP) 
						}
						else if (choosen_act == 3){
							clients_connections[choose_i].NOTIFY = to_string(1201);
						}
						choose_trigger = false;
						break;
					}
				}
			}
			/*else if (command == "") {
			}*/
			else{
				cout << endl << "[ERROR] Неизвестная комманда!";
			}
			if (not_allowed_to_pause.find(command) == string::npos) HC.pause();
			command = "";
			break;
		case 13:
			HC.cls();
			break;
		default:
			command += input;
			break;
		}
		outputinfo(command);
	}

	WSACleanup();
	HC.exit("");
}
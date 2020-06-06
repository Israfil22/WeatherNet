#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Goodweather.h>
#include <OneWire.h>
#include <DallasTemperature.h>

bool _DEBUB = true;

#define IR_LED D2
#define SERVERSTATE_LED D3
#define WIFISTATE_LED LED_BUILTIN

#define TEMPPIN D5

#define HTTPP_CL_TYPE_ARD   "ARDUINO"
#define HTTPP_METHOD_GET    "GET"
#define HTTPP_METHOD_GETDB  "GETDB"
#define HTTPP_METHOD_PUT    "PUT"

#define COM_MET "COM"
#define COM_MET_GET "GET"
#define COM_MET_PUT "PUT"
#define COM_PAR "PAR"
#define COM_PAR_TEMP "TEMP"
#define COM_PAR_FRPM "FANRPM"

//From arduino

#define RESP_TYPE "SUC"
#define RESP_SUC_OK "OK"
#define RESP_SUC_ERR "ERROR"
#define RESP_PAR "PAR"
#define RESP_ERC "ERCODE"

#define BR "\n"

String ALIVE = "alive";
String JSONDELIM = "<json_data>";

char ssid[]     = "NAGLFAR";
char password[] = "israfil22";

const char* host = "192.168.88.111";
const uint16_t port = 9889;

DynamicJsonDocument handshake_request(128);
  
DynamicJsonDocument my_attributes(512);

DynamicJsonDocument my_indicators(512);

DynamicJsonDocument my_required(512);

DynamicJsonDocument my_default(128);

DynamicJsonDocument notification_message(64);

DynamicJsonDocument empty_json(16);

bool fall_asleep = false; //триггер для отключения по комманде

const long get_temp_interval = 4 * 1000;

unsigned long prev_milis = 0;

unsigned long int notif_delay = 30*60*60*1000; //каждые 30 минут оповещения при сбоях

typedef struct mu_i_uli{unsigned short int key; unsigned long int value;} map_unit_int_ulint;

map_unit_int_ulint notif_list[4];


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void sleep(int del, bool out = true){
    if (out) Serial.print(String(1));
    delay(1000);
    for(int i = 1; i < del; i++){
        if (out) Serial.print("..." + String(i+1));
        delay(1000);
    }
    if (out) Serial.println("");
}

void blink_some(int count = 5, int del = 500, int led = LED_BUILTIN){
    for(int i = 0; i < count; i++){
        digitalWrite(led, LOW);
        delay(del);
        digitalWrite(led, HIGH);
        delay(del);
    }
}

String wrap_json(String json){
    return JSONDELIM+json+JSONDELIM;
}

bool is_formated_json(String request){
    int f_ind = request.indexOf(JSONDELIM);
    int l_ind = request.lastIndexOf(JSONDELIM);
    if (f_ind == -1 or l_ind == -1) return false;
    if (f_ind == l_ind) return false;
    return true;
}

String unwrap_json(String request){
    request = request.substring(request.indexOf(JSONDELIM) + JSONDELIM.length());// Serial.print(request);
    request = request.substring(0, request.indexOf(JSONDELIM));// Serial.print(request);
    return request;
}

WiFiClient client;

OneWire oneWire(TEMPPIN);

DallasTemperature t_sens(&oneWire);

IRGoodweatherAc ac(IR_LED);

void ac_send(bool def = false);

void setup(){
  pinMode(WIFISTATE_LED, OUTPUT);
  pinMode(IR_LED, OUTPUT);
  pinMode(SERVERSTATE_LED, OUTPUT);
  digitalWrite(IR_LED, LOW);
  digitalWrite(WIFISTATE_LED, HIGH);
  digitalWrite(SERVERSTATE_LED, HIGH);
  
  ac.begin();
  t_sens.begin();
  Serial.begin(115200);
  Serial.setTimeout(20);
  Serial.setDebugOutput(false);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  delay(5000);
  
  if (_DEBUB)Serial.println("WeatherNet [C]\tДобро пожаловать в консоль отладки");
  if (_DEBUB)Serial.print("Попытка подключения к: ");Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (_DEBUB) Serial.print(".");
  }
  if (_DEBUB) Serial.println("");
  if (_DEBUB) Serial.println("Устройство подключено к сети!");
  blink_some(20,100);
  digitalWrite(IR_LED, HIGH);
  digitalWrite(WIFISTATE_LED, LOW);
  if (_DEBUB) Serial.print("IP узла: ");if (_DEBUB)Serial.println(WiFi.localIP());
  if (_DEBUB) Serial.println("MAC устройства: " + String(WiFi.macAddress().c_str()));
  initiate();
  ac_send(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initiate(){
    //Set attributes
      //Temperature
      my_attributes["min_temp"] = 16;
      my_attributes["max_temp"] = 31; 
      my_attributes["now_temp"] = my_indicators["temp"];
      //Fan rpm
      my_attributes["min_rpm"]  = 0;
      my_attributes["max_rpm"]  = 3;
      my_attributes["now_rpm"] = my_indicators["fan_rpm"];
      //Common
      my_attributes["statecode"] = 200;
    //
    //Set default AC parametrs
    my_default["temp"]      = (int)(my_attributes["min_temp"].as<int>() + ((my_attributes["max_temp"].as<int>() - my_attributes["min_temp"].as<int>()) / 2));
    my_default["fan_rpm"]   = 0;
    my_default["mode"]      = kGoodweatherAuto;
    my_default["turnstate"] = 1;
    my_default["swing"]     = kGoodweatherSwingOff;
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Set handshake parametrs
    handshake_request["access_key"]   = "a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3";
    handshake_request["client_type"]  = HTTPP_CL_TYPE_ARD;
    handshake_request["unique_id"]    = "4e413570";
    handshake_request["name"]         = "ard_01";
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Indcicators DB
    my_indicators["temp"]       = 0;
    my_indicators["fan_rpm"]    = my_default["fan_rpm"];
    my_indicators["turnstate"]  = my_default["turnstate"];
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Required DB
    my_required["temp"]         = my_default["temp"];
    my_required["fan_rpm"]      = my_default["fan_rpm"];
    my_required["turnstate"]    = my_default["turnstate"];
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    notification_message["notify_code"] = 0;
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    notif_list[0].key   = 1201;
    notif_list[0].value = 0;
    //Далее ошибок не придумал
    
    deserializeJson(empty_json, "{}");
    t_sens.requestTemperatures();
    float temp = t_sens.getTempCByIndex(0);
    if (temp != DEVICE_DISCONNECTED_C){
        my_indicators["temp"] = (int)temp;
    }
    else{
        notification_message["notify_code"] = notif_list[0].key;
        notif_list[0].value = millis();
        my_attributes["statecode"] = notif_list[0].key;
    }
}

void ac_send(bool def){
    if (def == false){
        if (my_required["turnstate"] == 1){
            ac.on();
            ac.setTemp(my_required["temp"]);
            switch(my_required["fan_rpm"].as<int>()){
                case 0:
                    ac.setFan(kGoodweatherFanAuto);
                break;
                case 1:
                    ac.setFan(kGoodweatherFanLow);
                break;
                case 2:
                    ac.setFan(kGoodweatherFanMed);
                break;
                case 3:
                    ac.setFan(kGoodweatherFanHigh);
                break;
            }
        }
        else{
            ac.off();
        }
        
        my_indicators["turnstate"]  = my_required["turnstate"];
        my_indicators["fan_rpm"]    = my_required["fan_rpm"];
    }
    else{
      if (my_default["turnstate"] == 1){
        ac.on();
      }
      else{
          ac.off();
      }
      switch(my_default["fan_rpm"].as<int>()){
          case 0:
              ac.setFan(kGoodweatherFanAuto);
          break;
          case 1:
              ac.setFan(kGoodweatherFanLow);
          break;
          case 2:
              ac.setFan(kGoodweatherFanMed);
          break;
          case 3:
              ac.setFan(kGoodweatherFanHigh);
          break;
      }
      ac.setMode(my_default["mode"]);
      ac.setTemp(my_default["temp"]);
      ac.setSwing(my_default["swing"]);
    }
    ac.send();
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop(){
  DynamicJsonDocument json_get_data(2048);
  DynamicJsonDocument json_put_data(2048);
  DynamicJsonDocument json_com_print(128);
  DynamicJsonDocument json_com_read(128);
  
  String socket_data;

  //delay(100);
  
  if (!client.connected())
      digitalWrite(SERVERSTATE_LED, HIGH);
  else
      digitalWrite(SERVERSTATE_LED, LOW);
  
  /*
  if (Serial.available()){
      String notification = Serial.readString();
  }
  */
  if (!client.connected()){
      if (_DEBUB)Serial.println("-----------------------------------");
      if (_DEBUB){Serial.print("[Available mem: ");Serial.print(system_get_free_heap_size());Serial.print("]\n");}
      if (client.connect(host, port)){
          serializeJson(handshake_request, socket_data);
          client.print(JSONDELIM+socket_data+JSONDELIM);
          if (client.connected()){
              while(!client.available()){delay(10);};
              String handshake_reponse;
              while(client.available()){
                  handshake_reponse += (char)client.read();
              };
              if (is_formated_json(handshake_reponse)){
                  handshake_reponse = unwrap_json(handshake_reponse);
                  //Serial.print(socket_data);
                  deserializeJson(json_get_data, handshake_reponse);
                  //Serial.println("Овтет");
                  //serializeJson(json_get_data, Serial);
                  int responce_code = json_get_data["responce_code"].as<int>();
                  
                  if (responce_code == 200){
                      if (_DEBUB)Serial.println("Успешное подключение к серверу!");
                      digitalWrite(WIFISTATE_LED, LOW);
                      Serial.print(D1);
                  }
                  else{
                    if (_DEBUB){
                      Serial.print("Ошибка при подключении\n<--SERVER RESPONSE-->\n");
                      Serial.print(responce_code);
                      Serial.print("\n<--SERVER RESPONSE-->\n");
                    }
                  }
              }
          }
      }
      else{
          if (_DEBUB){
          Serial.print("Не удалось подключиться к "+String(host)+":"+String(port)+"\n\n\n");
          Serial.println("Таймаут в 5 секунд...");
          }
          sleep(5,_DEBUB);
      }
  }else{
      if (client.available()){
          if (Serial.available()){
              String message = Serial.readString();
              if (_DEBUB)Serial.println("\n=> " + message);
              if (message == "notify"){
                serializeJson(notification_message, socket_data);
                client.print(JSONDELIM+socket_data+JSONDELIM);
              }
              else if (message == "setfan"){
                  my_indicators["fan_rpm"]  = 1500;
              }
              else if (message == "settemp"){
                  my_indicators["temp"]     = 15;
              }
              else if (message == "getrequired"){
                  if (_DEBUB){
                  Serial.println("Необходимые значения: ");
                  JsonObject json_object = my_required.as<JsonObject>();
                  for(auto it = json_object.begin();it != json_object.end(); ++it){
                      Serial.print(it->key().c_str());Serial.print(" - ");Serial.println(it->value().as<String>());
                  }
                  Serial.println();
                  }
              }
              else if (message == "settemp"){
                  my_indicators["temp"]     = 15;
              }
              if (message != "notify") client.print(ALIVE);
          }
          socket_data = "";
          while (client.available()){
              socket_data += (char)client.read();
          }
          //Serial.println(socket_data);
          if (socket_data != ALIVE){
              if (socket_data != ""){
                  String response;
                  if (is_formated_json(socket_data)){
                      if (_DEBUB)Serial.println("-----------------------------------");
                      if (_DEBUB){Serial.print("[Available mem: ");Serial.print(system_get_free_heap_size());Serial.print("]\n");}
                      socket_data = unwrap_json(socket_data);
                      if (_DEBUB){
                      Serial.print("readed:");
                      Serial.print(socket_data);
                      }
                      deserializeJson(json_get_data, socket_data);
                      String request_type = json_get_data["request_type"].as<String>();
                      //serializeJson(json_get_data, Serial);
                      
                      if (request_type == HTTPP_METHOD_GET){
                          /*
                          json_com_print["COM"] = COM_MET_GET;
                          String ard_print;
                          serializeJson(json_com_print, ard_print);
                          Serial.print(ard_print);
                          
                          while(!Serial.available()){delay(1);}
                          String arudino_response = Serial.readString();
                          
                          deserializeJson(json_com_read, arudino_response);
                          
                          String indicators;
                          int response_code;
                          
                          if (json_com_read[RESP_TYPE] == RESP_SUC_OK){
                              serializeJson(json_com_read[RESP_PAR], indicators);
                              response_code = 200;
                          }
                          else{
                              serializeJson(empty_json, indicators);
                              response_code = json_com_read[RESP_ERC].as<int>();
                          }
                          */
                          //serializeJson(json_put_data, response);
                          
                          json_put_data["indicators"]     = my_indicators;
                          json_put_data["responce_code"]  = 200;
                          serializeJson(json_put_data, response);
                      }
                      else if (request_type == HTTPP_METHOD_GETDB){
                          //String attributes;
                          //serializeJson(my_attributes, attributes);
                          //refresh my_attributes
                          my_attributes["now_temp"]       = my_indicators["temp"];
                          my_attributes["now_rpm"]        = my_indicators["fan_rpm"];
                          my_attributes["turnstate"]      = my_indicators["turnstate"];
                          
                          json_put_data["attributes"]     = my_attributes;
                          json_put_data["responce_code"]  = my_attributes["statecode"];
                          serializeJson(json_put_data, response);
                      }
                      else if (request_type == HTTPP_METHOD_PUT){
                          deserializeJson(json_get_data, socket_data);
                          my_required["temp"]         = json_get_data["required_values"]["temp"].as<int>();
                          my_required["fan_rpm"]      = json_get_data["required_values"]["fan_rpm"].as<int>();
                          my_required["turnstate"]    = json_get_data["required_values"]["turnstate"].as<int>();
                          ac_send();
                          json_put_data["responce_code"]  = 200;
                          serializeJson(json_put_data, response);
                      }
                      else{
                          json_put_data["responce_code"]  = 851;
                          serializeJson(json_put_data, response);
                      }
                      client.print(wrap_json(response));
                  }
              }
              else{
                  client.print(ALIVE);
              }
          }
          else{
              //Serial.println(notification_message["notify_code"].as<int>());
              if (notification_message["notify_code"].as<int>() != 0){
                  socket_data = "";
                  serializeJson(notification_message, socket_data);
                  client.print(wrap_json(socket_data));
                  notification_message["notify_code"] = 0;
                  Serial.println("!SENDED!");
              }
              else{
                client.print(ALIVE);
              }
          }
      }
  }
 
  //Размещение здесь для увелечения шанса на схему ответил -> обработал
  if (millis() - prev_milis >= get_temp_interval){
      prev_milis = millis();
      t_sens.requestTemperatures();
      float temp = t_sens.getTempCByIndex(0);
      my_indicators["temp"] = (int)temp;
      if (temp != DEVICE_DISCONNECTED_C){
          my_indicators["temp"] = (int)temp;
          my_attributes["statecode"] = 200;
      }
      else{
          if (millis() - notif_list[0].value > notif_delay or notif_list[0].value == 0){
                notif_list[0].value = millis();
                notification_message["notify_code"] = String(notif_list[0].key);
                my_attributes["statecode"] = notif_list[0].key; 
          }
      }
  }
  client.print(ALIVE);

}

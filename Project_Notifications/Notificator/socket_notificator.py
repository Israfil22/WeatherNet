import sys
import os

import http
import http.client
import socket

import urllib
import threading

import random
import json
import time

import codecs


config = {
    'access_token': None,
    'secret_key': None,
    'group_id': None,
    'ip': None,
    'port': None,
    'mode': 0,
    'secure': 0,
    'domain': None,
    'url': "",
    'url_get_users': ""
}

dic = {}

if os.path.exists("config.cfg"):
    file = codecs.open( "config.cfg", "r", "utf-8" )
    lines = file.readlines()
    for line in lines:
        if (line[0] == '#'): continue
        if (line.find("=") != -1): 
            dic[((line[0:line.find("=")]).strip())] = (line[line.find("=") + 1:]).strip()
else:
    print("Файл конфигурации (config.cfg) не найден")
    input()
    exit()

for key in config.keys():
        param = dic.get(key, None)
        if (config[key] == None and param == None):
            print("Не найдены все необходимые конфигурационные настройки")
            input()
            exit()
        elif (param != None):
            config[key] = param

print(config)

input()


def send_request_to_vkapi(method, request_list):
    postfileds = {
        'v': '5.103',
        'access_token': config['access_token']
    }
    request_list.update(postfileds)
    encoded_request = urllib.parse.urlencode(request_list)

    server_connection = http.client.HTTPSConnection('api.vk.com')
    server_connection.request("POST", '/method/'+method, encoded_request)

    raw_response = server_connection.getresponse()
    str_response = raw_response.read()

    str_response = str_response.decode("utf-8")

    return str_response

def get_users_from_domain():
    postfileds = {
        'secret_key': config['secret_key']
    }
    if int(config['secure']) == 0:
        server_connection = http.client.HTTPConnection(config['domain'])
    else:
        server_connection = http.client.HTTPSConnection(config['domain'])

    #encoded_request = urllib.parse.urlencode(postfileds)
    if (config['url'] != ''):
        url = '/'+config['url_get_users']
    else:
        url = '/'+config['url']+'/'+config['url_get_users']

    headers = {
        "Content-type": "application/x-www-form-urlencoded", 
        "Accept": "text/plain"
    }

    server_connection.request("POST", url, urllib.parse.urlencode(postfileds), headers)
    raw_response = server_connection.getresponse()
    str_response = raw_response.read()
    str_response = str_response.decode("utf-8")
    print(str_response)

    if raw_response.status == 418:
        print("Задан неверный secret_key")
        input()
        exit()
    elif raw_response.status != 200:
        print("Неизвестный ответ сервера при попытке получения данных: " + str(raw_response.status()))
        input()
        exit()
    
    return str_response.split("\n")
    
########################################################################################
########################################################################################
def implode(list, glue = "\n"):
    result = ''
    for i in list:
        result+= str(i)+glue
    return result[0:len(result)- len(glue)]

def get_notify_content(code):
    #$000 - элемент
    #0$00 - тип
    #       1 - alert
    #       2 - warning
    #       3 - notice
    #00$$ - код ошибки
    notification_codes = {
        #Температурный датчик
        1201: "Температурный датчик не отвечает",
        #Инфракрасный датчик
        2201: "Инфракрасный датчик не отвечает",
        #Аккумуляторный отсек
        3301: "Заряд аккумулятора менее 60%",
        3302: "Заряд аккумулятора менее 30%",
        3202: "Заряд аккумулятора менее 10%"
    }
    result = notification_codes.get(code)
    if result != None:
        return result
    else:
        return "неизвестный код"
########################################################################################
########################################################################################

def handle_socket():
    try:
        main_socket = socket.socket()
        main_socket.connect((config['ip'],int(config['port'])))
        print("Прослушка начата")
        while True:
            raw_bytes = main_socket.recv(1024)
            msg = raw_bytes.decode("cp1251")
            print(msg)
            if len(msg) > 0:
                notify_msg = ''
                json_notify = json.loads(msg)
                notify_code = str(json_notify["notify_code"])
                if   int(notify_code[1]) == 1:
                        notify_msg += '&#10071;&#10071;&#10071;   Тревога!   &#10071;&#10071;&#10071;' + "\n\n"
                elif int(notify_code[1]) == 2:
                        notify_msg += '&#9888;Критическая ошибка в работе системы!&#9888;' + "\n\n"
                elif int(notify_code[1]) == 3:
                        notify_msg += '&#11093;Предупреждение.&#11093;' + "\n\n"
                notify_msg += 'Отправитель устройство с именем ' + json_notify["name"] + "\n"
                notify_msg += '&#127380;['+ json_notify["uid"] + ']' + "\n"
                notify_msg += '&#10145; ' + get_notify_content(int(notify_code))
                send_notify(notify_msg)
    except ConnectionResetError:
        send_notify("&#10071;&#10071;&#10071;Сервер неожиданно прервал соединение&#10071;&#10071;&#10071;")
        print("Сервер прервал соединение")
    except ConnectionRefusedError:
        print("Подключение по этому адресу недоступно\nreenter | recon")
########################################################################################
########################################################################################
def send_notify(alert_msg):
    request_headers = {
        'group_id': config['group_id'],
        'sort': 'id_asc',
        'count': 100,
        'offset': 0,
    }
    users_list = []
    if (int(config['mode']) == 0):
        str_response = send_request_to_vkapi('groups.getMembers', request_headers)
        json_response = json.loads(str_response)
        if "response" in json_response:
            for i in json_response["response"]["items"]:
                users_list.append(i)
        else:
            print('[ERROR] Ошибка при запросе списка участников с vkapi\n')
            print(json_response)
            input()
            exit()
    else:
        users_list = get_users_from_domain()
    
    print(len(users_list))
    if (not len(users_list)): return
    ########################################################################################
    request_headers = {
        'access_token' : config['access_token'],
        'v': '5.103',
        'user_ids': implode(users_list, ","),
        'message': alert_msg,
        'random_id': random.randrange(1,sys.maxsize)
    }
    ########################################################################################
    str_response = send_request_to_vkapi('messages.send', request_headers)
    print(str_response)

########################################################################################
########################################################################################

socket_thread = threading.Thread(target=handle_socket, daemon=True)
socket_thread.start()

command = ''
os.system('CLS')
while command != 'exit':
    #os.system('CLS')
    command = input()
    if command == "reenter":
        config['ip']      = input('Адрес сокета: ')
        config['port']    = int(input('Порт сокета: '))
        socket_thread = threading.Thread(target=handle_socket, daemon=True)
        socket_thread.start()
    if command == "recon":
        socket_thread = threading.Thread(target=handle_socket, daemon=True)
        socket_thread.start()
print('Нажмите любую кнопку для продолжения')
input()
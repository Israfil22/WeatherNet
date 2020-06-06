import http
import json
import configparser
import urllib
import http.client
import ssl
import random
import sys

access_token    = '932c921ce870b0e81b5133e6ac872485cc90a344a1736445ea04694370f3de3fe56c13e61233ab5f9fc60'
group_id        = '189735720'



def send_request(method, access_token, request_list):
    standart_header = {
        'v': '5.103',
        'access_token': access_token
    }
    request_list.update(standart_header)
    server_connection = http.client.HTTPSConnection('api.vk.com')
    encoded_request = urllib.parse.urlencode(request_list)
    server_connection.request("POST", '/method/'+method, encoded_request)
    raw_response = server_connection.getresponse()
    str_response = raw_response.read()
    str_response = str_response.decode("utf-8")
    return str_response

def impolode(list, glue = ''):
    result = ''
    for i in list:
        result+= str(i)+glue
    return result[0:len(result)-1]
########################################################################################
request_headers = {
    'group_id': '189735720',
    'sort': 'id_asc',
    'count': 100,
    'offset': 0,
}
str_response = send_request('groups.getMembers', access_token, request_headers)
json_response = json.loads(str_response)
users_group_members = []
if "response" in json_response:
    for i in json_response["response"]["items"]:
        users_group_members.append(i)
else:
    print('[ERROR] Request error[1]')
    input()
    sys.exit()
########################################################################################
request_headers = {
    'user_ids': impolode(users_group_members, ','),
    'name_case': 'nom'
}
str_response = send_request('users.get', access_token, request_headers)
json_response = json.loads(str_response)
for i in json_response["response"]:
    print("["+str(i["id"])+ "]\t" + str(i["first_name"]) + ' ' +str(i["last_name"]))
########################################################################################
print('Enter IDs for send. For several use "," delimiter')
users_to_send = input()
users_to_send_list = users_to_send.split(',')
for i in users_to_send_list:
    if int(i) not in users_group_members:
        print("[ERROR] Wrong list of IDs to send")
        input()
        sys.exit()
print("Input message for users:")
msg_to_clients = input()
random.seed()
users_ids_str = ''
########################################################################################
request_headers = {
    'access_token' : access_token,
    'v': '5.103',
    'user_ids': users_to_send,
    'message': msg_to_clients,
    'random_id': random.randrange(1,sys.maxsize)
}
########################################################################################
str_response = send_request('messages.send', access_token, request_headers)
print(str_response)
input()
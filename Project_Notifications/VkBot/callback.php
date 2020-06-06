<?
/*----------------------------------------------------------------------------*/
/*---------------------------VARIABLE   DECLARATION---------------------------*/
/*----------------------------------------------------------------------------*/

$secret_key = file_get_contents('secretkey');
$auth_key   = file_get_contents('authkey');

$vkapi_url = 'https://api.vk.com/method/';

$curl_core = curl_init();

$mysql_core = new mysqli('localhost', 'israfil22_wnet', 'WeatherNet', 'israfil22_wnet');

if ($mysql_core->errno){
    $log_error = date(DATE_RFC822)."\n".$mysql_core->errno.":".$mysql_core->error."\n\n";
    file_put_contents("callback_err.log", $log_error, FILE_APPEND);
    exit(1);
    $mysql_core->close();
}

$std_postfields = array(
    'v'             => '5.103',
    'access_token'  => $auth_key
);

$std_curl_options = array(
    CURLOPT_URL => $vkapi_url,
    CURLOPT_HTTP_VERSION => CURL_HTTP_VERSION_1_0,
    CURLOPT_CONNECTTIMEOUT => 3,
    CURLOPT_POST => TRUE,
    CURLOPT_RETURNTRANSFER => TRUE,
    CURLOPT_ENCODING => '',
    CURLOPT_TIMEOUT => 3,
    CURLOPT_SSL_VERIFYPEER => false
);

$std_keyboard_disabled = array(
    'inline' => false,
    'one_time' => false,
    'buttons' => array(
            array(
                array(
                    'action' => array(
                                        'type'=> 'text',
                                        'label' => 'Включить оповщения',
                                        'payload' => json_encode(array(
                                            'command' => 'unmute'
                                        ))
                                ),
                    'color' => 'positive'
                )
            )
        )
);

$std_keyboard_enabled = array(
    'inline' => false,
    'one_time' => false,
    'buttons' => array(
            array(
                array(
                    'action' => array(
                                        'type'=> 'text',
                                        'label' => 'Отключить оповещения',
                                        'payload' => json_encode(array(
                                            'command' => 'mute'
                                        ))
                                ),
                    'color' => 'negative'
                )
            )
        )
);

$std_message_unknown = "Привет! &#128075;\nК сожалению, мои разработчики очень ленивые\n
                        Поэтому я умею только присылать оповещения!\n
                        НО! Можете использовать кнопки, чтобы я понимал Вас :)";

$std_message_mute   = "Оповщения отключены.";
$std_message_unmute = "Оповщения снова включены!";
/*----------------------------------------------------------------------------*/
/*---------------------------FUNCTION   DECLARATION---------------------------*/
/*----------------------------------------------------------------------------*/

function send_message_to_users($users_array, $json_entity, $message = "", $keyboard = null){
    GLOBAL  $curl_core,
            $vkapi_url, 
            $std_postfields, $std_curl_options;
    
    $post_fields = $std_postfields;
    $post_fields['group_id']        = $json_entity["group_id"];
    $post_fields['user_ids']        = implode(",", $users_array);
    $post_fields['random_id']       = mt_rand();
    if ($keyboard != null)
        $post_fields['keyboard']    = json_encode($keyboard);
    
    $post_fields['message']         = $message;

    $curl_options                       = $std_curl_options;
    $curl_options[CURLOPT_URL]          = $vkapi_url.'messages.send';
    $curl_options[CURLOPT_POSTFIELDS]   = $post_fields; 
    
    
    curl_setopt_array($curl_core, $curl_options);
    $response = curl_exec($curl_core);
    file_put_contents("file", $response);
    curl_close($response);
}

/*----------------------------------------------------------------------------*/
/*---------------------------------MAIN  PART---------------------------------*/
/*----------------------------------------------------------------------------*/

/*
foreach ($_SERVER as $name => $value) {
    $headers_str .= $name . ' = ' . $value . "\n";
}
*/

$entityBody = file_get_contents('php://input');
$json_entity = json_decode($entityBody, true);


if (array_key_exists('secret', $json_entity)){
    if ($json_entity['secret'] == $secret_key){
        if (array_key_exists('type', $json_entity)){
            echo "ok";
            if ($json_entity['type'] == "message_new"){
                $request_headers = array(
                    'group_id' 	=>  $json_entity["group_id"],
                    'sort'  	=>  'id_asc',
                    'count' 	=>  100,
                    'offset'	=>  0
                );

                $post_fields = array_merge($request_headers, $std_postfields);

                $curl_options                       = $std_curl_options;
                $curl_options[CURLOPT_URL]          = $vkapi_url.'groups.getMembers';
                $curl_options[CURLOPT_POSTFIELDS]   = $post_fields; 
                
                curl_setopt_array($curl_core, $curl_options);
                $response = curl_exec($curl_core);
                
                $group_members = array();
                
                $json_object = json_decode($response, true);


                foreach($json_object["response"]["items"] as $key => $value){
                    $group_members[] = $value;
                }
                
                if (in_array($json_entity['object']['message']['from_id'], $group_members)){
                    $keyboard = null;

                    $result = $mysql_core->query('SELECT * FROM UsersMessageAccess WHERE ID='.$json_entity['object']['message']['from_id']);
                    $res_array = $result->fetch_array();
                    if (!count($res_array)){ // Пользователь никогда ранее не писал
                        $message = "Вы подписались на рассылку уведомление!\nДля отключения нажмите соответствующую кнопку.";
                        $keyboard = $std_keyboard_enabled;
                        $result = $mysql_core->query('INSERT INTO UsersMessageAccess VALUES ('.$json_entity['object']['message']['from_id'].','.true.')');
                    }
                    else{   // Пользователь уже писал сообществу
                        if (array_key_exists('payload', $json_entity['object']['message'])){
                            $payload = json_decode($json_entity['object']['message']['payload'], true);
                            if ($payload['command'] == "mute"){
                                $message = $std_message_mute;
                                $keyboard = $std_keyboard_disabled;
                                $allow_state = 0;
                            }
                            elseif ($payload['command'] == "unmute"){
                                $message = $std_message_unmute;
                                $keyboard = $std_keyboard_enabled;
                                $allow_state = 1;
                            }
                            $mysql_core->query('UPDATE UsersMessageAccess SET Allow='.$allow_state.' WHERE ID='.$json_entity['object']['message']['from_id']);
                        }
                        else{
                            $message = $std_message_unknown;
                        }
                    }
                    send_message_to_users(array($json_entity['object']['message']['from_id']), $json_entity, $message, $keyboard);
                }
            }
            elseif ($json_entity['type'] == "group_leave" or $json_entity['type'] == "user_block"){
                $result = $mysql_core->query('DELETE FROM UsersMessageAccess WHERE ID='.$json_entity['object']['user_id']);
            }
        }
        else{
            header("HTTP/1.0 418 I’m a teapot");
        }
    }
    else{
        header("HTTP/1.0 418 I’m a teapot");
    }
}
else{
	header("HTTP/1.0 418 I’m a teapot");
}

curl_close($curl_core);
$mysql_core->close();

?>
<?


if ($_POST['secret_key'] != file_get_contents("secretkey")){
    header("HTTP/1.0 418 I’m a teapot");
    exit();
}

$mysql_core = new mysqli('localhost', 'israfil22_wnet', 'WeatherNet', 'israfil22_wnet');

if ($mysql_core->errno){
    $log_error = date(DATE_RFC822)."\n".$mysql_core->errno.":".$mysql_core->error."\n\n";
    file_put_contents("get_users_err.log", $log_error, FILE_APPEND);
    exit(1);
    $mysql_core->close();
}

$return = $mysql_core->query('SELECT * FROM UsersMessageAccess WHERE Allow=1');

$res_array = $return->fetch_all(); 

$sql_ids_array = array();

foreach($res_array as $key => $value){
    $sql_ids_array[] = $value[0];
}

echo implode("\n", $sql_ids_array);

$mysql_core->close();
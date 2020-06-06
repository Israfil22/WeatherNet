class User_Interface{
	constructor(){
		this.ip 		= _CONSTG.server_ip
		this.port 		= _CONSTG.server_port
		this.access_key = _CONSTG.access_key 
	}
	
	help(){
		console.log("[Interface] - для просмотра всех настроек терминала");
	}
	
	outputstgs(){
		console.log('----------------------------\n'+'Server IP\t\t['+_CONSTG["server_ip"]+']'+"\n"+"PORT\t\t\t["+_CONSTG["server_port"]+"]"+'\n----------------------------');
	}
}
var Interface = new User_Interface();

class User_Interaction{
	static notify(msg, type = 1){
		if (this.UsInt_NOT_type.hasOwnProperty(parseInt(type))){
			console.log(this.UsInt_NOT_type[parseInt(type)] + " " + msg);
		} else return false;
	}
}
User_Interaction.UsInt_NOT_type = {1: "[INFO]", 2: "[WARNING]", 3: "[ERROR]", 4: "[SYSTEM]"};

var UsInt = User_Interaction;
/*---------------------   STANDART FUNCTIONS REDEFINITION    ---------------------*/

function help(){
	console.log('Команда "Interface.help" для помощи');
}

function clear(){
	console.clear();
	Interface.outputstgs();
}

/*----------------------------   INITIAL FUNCTIONS    ----------------------------*/

Interface.outputstgs();

UsInt.notify('Для просмотра ключа доступа: Interface.access_key');
UsInt.notify('Система загружена...');
/*-------------------------------   VARIABLES DEFENITION    ------------------------------*/
GLOBAL_VAR = {
	connected_devices: [],
	connection_state: false,
	active_objests: [],
	get_interval_timer: null
}

var Pages_Objects = {
	Main_Page: 		document.getElementById("Main_page"),
	Contact_Page: 	document.getElementById("Contact_page"),
	State_Page: 	document.getElementById("State_page")
}

var index_list_fanrpm = {
	0: "Авто",
	1: "Низкая",
	2: "Средняя",
	3: "Высокая",
	4: "Турбо",
}

var server=new XMLHttpRequest();

const REQ_GETDB = "GETDB"
const REQ_GET	= "GET"
const REQ_PUT 	= "PUT"

const 	json_wrapper = "<json_data>"

const 	BR = "\n";

const 	msg_critical = "Критическая ошибка. Свяжитесь с администратором.";


var request_type = "";

/*-------------------------------   FUNCTIONS DEFENITION    ------------------------------*/

function handleResponse(status, response){
	let Dessolution_Objects_Error = [
		["Loading_Img", -0.1, 30, null, "none"],
		["Loading_Page", -0.1, 10, null, "none"],
		["Error_Page", 0.1,10, "block", null],
		["Background", -1, 10, "none", null]
	]
	switch(parseInt(status)){
		case 200:
			/*Проверка на присутствие форматированного json*/
			if (response.indexOf(json_wrapper) != -1 && response.lastIndexOf(json_wrapper) != response.indexOf(json_wrapper)){
				var json_str = response.slice(response.indexOf(json_wrapper) + json_wrapper.length+1, response.lastIndexOf(json_wrapper));
				var resp_json = JSON.parse(json_str);
				//UsInt.notify('Response('+request_type+'):' + resp_json.responce_code, 4);
					switch(request_type){
						case REQ_GETDB:
							if (resp_json.responce_code == 200){
								if (GLOBAL_VAR.connection_state == false){
									document.getElementById("label_server_id").innerText 		= resp_json["host_id"];
									document.getElementById("label_connection_state").innerText = "терминал подключен к серверу.";
								}
								build_page(resp_json.json_responce);
							}else{
								document.getElementById("Error_Page_Error_Box").innerText = resp_json.responce_code.toString();
								page_dissolution_effect(Dessolution_Objects_Error);
							}
						break;
						case REQ_GET:
							if (resp_json.responce_code == 200){
								build_page(resp_json.json_responce);
							}else{
								UsInt.notify("Ошибка при получении индикаторов: "+resp_json.responce_code,3);
								document.getElementById("Error_Page_Error_Box").innerText = resp_json.responce_code.toString();
								page_dissolution_effect(Dessolution_Objects_Error);
							}
						break;
						case REQ_PUT:
							if (resp_json.responce_code == 200){
								//UsInt.notify('Response('+request_type+'):' + resp_json.responce_code, 4);
							}else{
								alert("Ошибка при отправке: " +resp_json.responce_code);
							}
						break;
					}
			}
			else{
				document.getElementById("Error_Page_Error_Box").innerText = "903";
				page_dissolution_effect(Dessolution_Objects_Error);
			}
		break;
		case 0:
			document.getElementById("Error_Page_Error_Box").innerText = "901";
			page_dissolution_effect(Dessolution_Objects_Error);
		break;
		default:
			if (status == "418"){
				document.getElementById("Error_Page_Error_Box").innerText = status.toString();
				page_dissolution_effect(Dessolution_Objects_Error);
			}
			else{
				document.getElementById("Error_Page_Error_Box").innerText = "902";
				page_dissolution_effect(Dessolution_Objects_Error);
			}
		break;
	}
	if (status != 200){
		clearTimeout(GLOBAL_VAR.get_interval_timer);
	}
}

var handle_connection_state_change = function () {
	switch (server.readyState){
		case 0 : // UNINITIALIZED
		case 1 : // LOADING
		case 2 : // LOADED
		case 3 : // INTERACTIVE
		break;
		case 4 : // COMPLETED
		handleResponse(server.status, server.responseText);
		break;
		default: alert("error");
	}
}
server.onreadystatechange = handle_connection_state_change;

function send_request_to_server(req_tp, required_values = "", unique_id = ""){
	let request = {
		access_key: _CONSTG.access_key,
		client_type: "TERMINAL",
		method: req_tp
	}
	request_type = req_tp;
	if (req_tp == REQ_PUT){
		request["required_values"] = required_values;
		request["unique_id"] = unique_id;
	}
	let json_request = "<json_data>\n" + JSON.stringify(request) + "\n<json_data>\n";
	server.open("POST","http://"+_CONSTG.server_ip+":"+_CONSTG.server_port,true);
	server.setRequestHeader('Content-Type', 'text/plain');
	server.send(json_request);
}

function build_page(json_data){
	let new_devices_db = []; for(key in json_data){new_devices_db.push(key)};
	if (GLOBAL_VAR.connection_state){
		if (array_is_equal(new_devices_db, GLOBAL_VAR.connected_devices)){
			if (request_type == REQ_GETDB){
				if (json_data == null){
					let parent_element = document.getElementById('Main_page');
					let insert = '<div id="Error_No_Devices">Отсутствуют подключенные к сети устройства</div>';
					GLOBAL_VAR.connected_devices = [];
					parent_element.innerHTML = insert;
				}
				build_devices_with_json(json_data);
			}
			else{
				refresh_indicators(json_data);
			}
		}
		else{
			let Dessolution_Objects_Main_Page = [ 
				["Page", -1.0,10, "none", null],
				["Loading_Img", 1, 1, "block", null],
				["Loading_Page", 1, 1, "flex", null]
			]
			page_dissolution_effect(Dessolution_Objects_Main_Page);
			let parent_element = document.getElementById('Main_page');
			parent_element.innerText = '';
			GLOBAL_VAR.connected_devices = new_devices_db;
			send_request_to_server(REQ_GETDB);
		}
	}
	else{
		build_devices_with_json(json_data);
		GLOBAL_VAR.connection_state = true;
		GLOBAL_VAR.connected_devices = new_devices_db;
		GLOBAL_VAR.get_interval_timer = setInterval(get_indicators_from_server, _CONSTG.refresh_delay);
	}
}

function build_devices_with_json(json_data){
	//if GLOBAL_VAR.connection_state
	let parent_element = document.getElementById('Main_page');
	parent_element.innerText = "";
	let Dessolution_Objects_Main_Page = [
		["Loading_Img", -0.04, 30, null, "none"],
		["Loading_Page", -0.03, 10, null, "none"],
		["Page", 0.03,10, "block"]
	]
	if (json_data != null){
		for(key in json_data){
			if (json_data[key]["responce_code"] == 200){
				//GLOBAL_VAR.connected_devices.push(key);
				let element_attributes = json_data[key]["attributes"];
				
				let element_uid = key;
				let element_html_id = "";
				
				//console.log(element_attributes);
				
				parent_element = document.getElementById('Main_page');
				/*		 						   TEMPERATURE  							 */
				///////////////////////////////////////////////////////////////////////////////
				element_html_id = 'container_'+element_uid;
				let insert = '<div id="'+element_html_id+'"></div>';
				parent_element.insertAdjacentHTML('beforeend', insert);
				
				let inserted_element = document.getElementById(element_html_id);
				inserted_element.setAttribute('class','Container');
				parent_element = inserted_element;
				///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'name_header_'+element_uid;
					insert = '<div id="'+element_html_id+'"></div>';
					parent_element.insertAdjacentHTML('beforeend', insert);

					inserted_element = document.getElementById(element_html_id);

					inserted_element.setAttribute('class','device_name_header');
					inserted_element.insertAdjacentHTML('beforeend', element_attributes["device_name"]);
					inserted_element.insertAdjacentHTML('afterend', '</br>');
					inserted_element.setAttribute('onclick', 'visible_element("'+'uid_header_'+element_uid+'","inline")');
					///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'uid_header_'+element_uid;
					insert = '<div id="'+element_html_id+'"></div>';
					parent_element.insertAdjacentHTML('beforeend', insert);

					inserted_element = document.getElementById(element_html_id);

					inserted_element.setAttribute('class','device_uid_header');
					inserted_element.insertAdjacentHTML('beforeend', "UID " + element_uid);
					inserted_element.insertAdjacentHTML('afterend', '</br>');
					///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'checkbox_turnstate_'+element_uid;
					console.log(element_attributes["turnstate"]);
					insert = '<label class="switch">\n<input type="checkbox" id="'+element_html_id+'">\n<span class="switch_slider" onclick="visible_element(\'form_'+element_uid+'\', \'block\')">\n</span>\n</label>';
					parent_element.insertAdjacentHTML('beforeend', insert);
					if (element_attributes["turnstate"])
						document.getElementById(element_html_id).checked = true;
					///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'form_'+element_uid;
					insert = '<form id="'+element_html_id+'"></form>';
					parent_element.insertAdjacentHTML('beforeend', insert);
					inserted_element = document.getElementById(element_html_id);
					if (element_attributes["turnstate"])
						inserted_element.style.display = 'block';
					else
						inserted_element.style.display = 'none';
					
					inserted_element.setAttribute('class','form');
					parent_element = inserted_element;
					///////////////////////////////////////////////////////////////////////////////
					/*--------------------------------TEMPERATURE--------------------------------*/
					insert = '<div class="name_attribute">Температура</div>';
					parent_element.insertAdjacentHTML('beforeend', insert);
					///////////////////////////////////////////////////////////////////////////////
					insert = '<div class="attribute_box_wrap"></div>';
					parent_element.insertAdjacentHTML('beforeend', insert);
					///////////////////////////////////////////////////////////////////////////////
					insert = '<div class="attribute_box"></div>';
					parent_element.getElementsByClassName("attribute_box_wrap")[0].insertAdjacentHTML('beforeend', insert);
					///////////////////////////////////////////////////////////////////////////////
					insert = '<div class="slidecontainer"></div>';
					parent_element.getElementsByClassName("attribute_box")[0].insertAdjacentHTML('beforeend', insert);
					///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'slider_temp_'+element_uid;
					insert = '<input id="'+element_html_id+'"></input>';
					parent_element.getElementsByClassName("slidecontainer")[0].insertAdjacentHTML('beforeend', insert);
					inserted_element = document.getElementById(element_html_id);

					inserted_element.setAttribute('type', 'range');
					inserted_element.setAttribute('class', 'slider');
					inserted_element.setAttribute('min', element_attributes["min_temp"]);
					inserted_element.setAttribute('max', element_attributes["max_temp"]);
					inserted_element.setAttribute('value', element_attributes["now_temp"]);
					inserted_element.setAttribute('model', "temp");
					inserted_element.setAttribute('oninput', 'change_index_by_slider("'+element_html_id+'","temp_required_'+element_uid+'");');
					///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'temp_required_'+element_uid;
					insert = '<span id="'+element_html_id+'"></input>';
					parent_element.getElementsByClassName("attribute_box")[0].insertAdjacentHTML('beforeend', insert);
					inserted_element = document.getElementById(element_html_id);

					inserted_element.setAttribute('class', 'required_box');
					inserted_element.innerText = element_attributes["now_temp"];
					///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'temp_indicator_'+element_uid;
					insert = '<span id="'+element_html_id+'"></input>';
					parent_element.getElementsByClassName("attribute_box")[0].insertAdjacentHTML('beforeend', insert);
					inserted_element = document.getElementById(element_html_id);

					inserted_element.setAttribute('class', 'indicator_box');
					inserted_element.innerText = element_attributes["now_temp"];
					///////////////////////////////////////////////////////////////////////////////
					/*----------------------------------FAN RPM----------------------------------*/
					insert = '<div class="name_attribute">Скорость вращения</div>';
					parent_element.insertAdjacentHTML('beforeend', insert);
					///////////////////////////////////////////////////////////////////////////////
					insert = '<div class="attribute_box_wrap"></div>';
					parent_element.insertAdjacentHTML('beforeend', insert);
					///////////////////////////////////////////////////////////////////////////////
					insert = '<div class="attribute_box"></div>';
					parent_element.getElementsByClassName("attribute_box_wrap")[1].insertAdjacentHTML('beforeend', insert);
					///////////////////////////////////////////////////////////////////////////////
					insert = '<div class="slidecontainer"></div>';
					parent_element.getElementsByClassName("attribute_box")[1].insertAdjacentHTML('beforeend', insert);
					///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'slider_fanrpm_'+element_uid;
					insert = '<input id="'+element_html_id+'"></input>';
					parent_element.getElementsByClassName("slidecontainer")[1].insertAdjacentHTML('beforeend', insert);
					inserted_element = document.getElementById(element_html_id);

					inserted_element.setAttribute('type', 'range');
					inserted_element.setAttribute('class', 'slider');
					inserted_element.setAttribute('min', element_attributes["min_rpm"]);
					inserted_element.setAttribute('max', element_attributes["max_rpm"]);
					inserted_element.setAttribute('value', element_attributes["now_rpm"]);
					inserted_element.setAttribute('model', "fan_rpm");
					inserted_element.setAttribute('oninput', 'chane_index_list_by_slider(\''+element_html_id+'\',\'fanrpm_required_'+element_uid+'\',\''+ window.btoa(encodeURI(JSON.stringify(index_list_fanrpm)))+'\');');
					///////////////////////////////////////////////////////////////////////////////
					element_html_id = 'fanrpm_required_'+element_uid;
					insert = '<span id="'+element_html_id+'"></input>';
					parent_element.getElementsByClassName("attribute_box")[1].insertAdjacentHTML('beforeend', insert);
					inserted_element = document.getElementById(element_html_id);

					inserted_element.setAttribute('class', 'required_box');
					inserted_element.innerText = index_list_fanrpm[element_attributes["now_rpm"]];
					///////////////////////////////////////////////////////////////////////////////	
					element_html_id = 'fanrpm_indicator_'+element_uid;
					insert = '<span id="'+element_html_id+'"></input>';
					parent_element.getElementsByClassName("attribute_box")[1].insertAdjacentHTML('beforeend', insert);
					inserted_element = document.getElementById(element_html_id);

					inserted_element.setAttribute('class', 'indicator_box');
					inserted_element.innerText = index_list_fanrpm[element_attributes["now_rpm"]];
					///////////////////////////////////////////////////////////////////////////////
				parent_element = document.getElementById("form_"+element_uid);
				insert = '<div id="submit_'+element_uid+'">Отправить</div>'; 
				parent_element.insertAdjacentHTML('afterend', insert);
				inserted_element = document.getElementById('submit_'+element_uid);
				inserted_element.setAttribute("class", "btn_submit");
				inserted_element.setAttribute("onclick", 'put_required_to_server("'+element_uid+'")');
				
			}
		}
	}
	else{
		let insert = '<div id="Error_No_Devices">Отсутствуют подключенные к сети устройства</div>';
		parent_element.innerHTML = insert;
	}
	if (document.getElementById("Page").style.display == "none"){
		page_dissolution_effect(Dessolution_Objects_Main_Page);
	}
	return true;
}

function refresh_indicators(json_data){
	let Dessolution_Objects_Main_Page = [
		["Loading_Img", -0.1, 30, null, "none"],
		["Loading_Page", -0.1, 10, null, "none"],
		["Error_Page", 0.1,10, "block"],
		["Background", -1, 10, "none"]
	]
	let parent_element = document.getElementById('Main_page');
	//console.log(json_data);
	for(key in json_data){
		let device_indicators = json_data[key]["indicators"];
		document.getElementById("temp_indicator_"+key).innerHTML 	= device_indicators["temp"];
		document.getElementById("fanrpm_indicator_"+key).innerHTML 	= index_list_fanrpm[device_indicators["fan_rpm"]];
		/*
		if (device_indicators["turnstate"]){
			document.getElementById("form_"+key).style.display = "block";
			document.getElementById("checkbox_turnstate_"+key).checked = true;
		}
		else{
			document.getElementById("form_"+key).style.display = "none";
			document.getElementById("checkbox_turnstate_"+key).checked = false;
		}
		*/
	}
}

function put_required_to_server(element_uid){
	let objects = document.getElementById('form_'+element_uid).elements;
	json_object = {}
	for(let key = 0; key < objects.length; key++){
		json_object[objects[key].getAttribute('model')] = objects[key].value;
	}
 	json_object["turnstate"] = document.getElementById("checkbox_turnstate_"+element_uid).checked ? 1 : 0;
	send_request_to_server(REQ_PUT, json_object, element_uid);
}

function get_indicators_from_server(){
	send_request_to_server(REQ_GET);
}

/*----------------------------------------------------------------------------------------*/


/*--------------------------------   INITIAL FUNCTIONS    --------------------------------*/
send_request_to_server(REQ_GETDB);
/*--------------------------------   INITIAL PROPERTIES   --------------------------------*/

/*--------------------------------      OTHER ACTIONS     --------------------------------*/
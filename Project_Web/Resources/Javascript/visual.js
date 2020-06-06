var Nav_Objects = {
	Btn_Main: 		document.getElementById("Btn_SelectPage_Main"),
	Btn_Contact: 	document.getElementById("Btn_SelectPage_Contact"),
	Btn_State:		document.getElementById("Btn_SelectPage_State")
}

var Pages_Objects = {
	Main_Page: 		document.getElementById("Main_page"),
	Contact_Page: 	document.getElementById("Contact_page"),
	State_Page: 	document.getElementById("State_page")
}

var Dessolution_Objects_Main_Page = [
	["Loading_Img", -0.04, 30, null, "none"],
	["Loading_Page", -0.03, 10, null, "none"],
	["Page", 0.03, 10, "block"]
]

function set_style_with_width(){
	let device_width = document.documentElement.clientWidth;
	let device_height = document.documentElement.clientHeight;
	if (device_width <= 720)
		for(key in Nav_Objects){Nav_Objects[key].style.display = "block";}
	else
		for(key in Nav_Objects){Nav_Objects[key].style.display = "table-cell";}
}
window.addEventListener("resize", set_style_with_width);

function change_nav_style(){
		for(key in Nav_Objects){Nav_Objects[key].style.backgroundColor	= "transparent";}
		for(key in Pages_Objects){Pages_Objects[key].style.display		= "none";}
		
		document.getElementById(this.getAttribute("id").slice(15)+"_page").style.display = "block";
		this.style.backgroundColor = "#aeb7b7";
}
for (key in Nav_Objects){Nav_Objects[key].onclick = change_nav_style;}


//function(){if (pages_object[index][1] < 0) if (opacity <= 0.005) return true; else return false; else if (opacity >= 0.005) return true; else return false;}

function page_dissolution_effect(pages_object, index = 0){
	let branching = function(pages_object, index, opacity){if (pages_object[index][1] < 0) if (opacity <= 0.005)return true; else return false; else if (opacity >= 1) return true; else return false;}
	let set_limit = function(pages_object, index){if (pages_object[index][1] < 0) return 0; else return 1;};
	let element = document.getElementById(pages_object[index][0]);
	if (pages_object[index][3] != null) element.style.display = pages_object[index][3]
	let opacity = parseFloat(element.style.opacity) + pages_object[index][1];
	//console.log("index: " + index + "        opacity: " + opacity + "" + "      " + element);
	if (branching(pages_object, index, opacity)){
		element.style.opacity = set_limit(pages_object, index);
		if (pages_object[index][4] != null) element.style.display = pages_object[index][4]
		if (index +1 < pages_object.length)
			page_dissolution_effect(pages_object, ++index);
	}
	else{
		element.style.opacity = opacity;
		setTimeout(page_dissolution_effect, pages_object[index][2], pages_object, index);
	}
}

function change_index_by_slider(slider, required_header){
	document.getElementById(required_header).innerText = document.getElementById(slider).value;
}

function chane_index_list_by_slider(slider, required_header, list){
	//console.log(list);
	parsed_list = JSON.parse(decodeURI(window.atob(list)));
	document.getElementById(required_header).innerText = parsed_list[parseInt(document.getElementById(slider).value)];
}

function visible_element(element_id, type){
	let element = document.getElementById(element_id);
	if (element.style.display == type)
		element.style.display = "none";
	else
		element.style.display = type;
}

/*----------------------------------------------------------------------------------------*/

/*--------------------------------   INITIAL FUNCTIONS    --------------------------------*/
set_style_with_width();
/*--------------------------------   INITIAL PROPERTIES   --------------------------------*/
Nav_Objects["Btn_Main"].style.background = "#aeb7b7";
document.getElementById("Page").style.display = "none";

document.getElementById("State_page").style.display="none";
document.getElementById("Contact_page").style.display="none";

Nav_Objects.Btn_Main.onclick 		= change_nav_style;
Nav_Objects.Btn_Contact.onclick 	= change_nav_style;
Nav_Objects.Btn_State.onclick 		= change_nav_style;

function editzone(zone_id){
	request="ajax=1&action=editzone&zone_id="+zone_id;
	sendRequest("zones.php",request,"POST");
};

function addzone(group_id){
	request="ajax=1&action=editzone&zone_id=0&group_id="+group_id;
	sendRequest("zones.php",request,"POST");
};



function rmzone(zone_id){
	zname=document.getElementById("zone_comment").value;
	needreload=0;
	if (confirm(zone_ask_remove+zname+'?')){
		request="ajax=1&action=rmzone&zone_id="+zone_id;
		sendRequest("zones.php",request,"POST");
		needreload=1;
	};
};



function savezone(){
	zip=document.getElementById("zone_ip").value;
	zmask=document.getElementById("zone_mask").value;
	zport=document.getElementById("zone_port").value;
	zcomment=document.getElementById("zone_comment").value;
	zid=document.getElementById("zone_id").value;
	zprio=document.getElementById("zone_priority").value;
	request="ajax=1&action=updatezone&zone_id="+zid+"&zone_ip="+zip+"&zone_mask="+zmask+"&zone_port="+zport+"&zone_comment="+zcomment+"&zone_priority="+zprio;
	if(gid=document.getElementById("group_id")){
		request+="&group_id="+gid.value;
	};
	sendRequest("zones.php",request,"POST");
	editlayer=document.getElementById('edit_layer');
	editlayer.style.visibility='hidden';
	baselayer=document.getElementById('menu');
	baselayer.style.visibility='visible';
	needreload=1;
	
};

function editgroup(group_id){
	request="ajax=1&action=editgroup&group_id="+group_id;
	sendRequest("zones.php",request,"POST");
};

function rmgroup(group_id){
	gname=document.getElementById("groupname").value;
	if (confirm(group_ask_remove+gname+'?')){
		request="ajax=1&action=rmgroup&group_id="+group_id;
		sendRequest("zones.php",request,"POST");
		needreload=1;
	};
};

function savegroup(){
	gid=document.getElementById("group_id").value;
	gname=document.getElementById("groupname").value;
	gpool=document.getElementById("group_pf_tbl").value;
	request="ajax=1&action=updategroup&group_id="+gid+"&group_name="+gname+"&group_table="+gpool;
	if(gprice=document.getElementById("group_price").checked){
		request+="&group_price=1";
	}else if (gprice=document.getElementById("group_price").value>0){
		request+="&group_price="+gprice;
	};
	if(gid=document.getElementById("group_id")){
		request+="&group_id="+gid.value;
	};
	sendRequest("zones.php",request,"POST");
	editlayer=document.getElementById('edit_layer');
	editlayer.style.visibility='hidden';
	baselayer=document.getElementById('menu');
	baselayer.style.visibility='visible';
	//needreload=1;
	
};

function dontsave(){
	editlayer=document.getElementById('edit_layer');
	editlayer.style.visibility='hidden';
	baselayer=document.getElementById('menu');
	baselayer.style.visibility='visible';
	needreload=0;
};

/*
*Dynamic html is here:
*/
function display(data){
	editlayer=document.getElementById('edit_layer');
	baselayer=document.getElementById('menu');
	editlayer.innerHTML=data;
	baselayer.style.visibility='hidden';
	editlayer.style.visibility='visible';
}

function onReadyState(){
	var ready=req.readyState;
	var data=null;
	if (ready==READY_STATE_COMPLETE){
		data=req.responseText;
		if (data){
			display(data);
		};
		if (needreload){
			window.location.reload();
		};
	}
}
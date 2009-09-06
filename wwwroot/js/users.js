function updaterow(rowid){
	row=document.getElementById("row"+rowid);
	active=document.getElementById("active"+rowid).checked;
	login=document.getElementById("login"+rowid).value;
	ip=document.getElementById("ip"+rowid).value;
	debit=document.getElementById("debit"+rowid).value;
	var limit=0
	if (limitfield=document.getElementById("limit"+rowid)) limit=limitfield.value;
	
	var credit=0
	if (creditfield=document.getElementById("credit"+rowid)) credit=creditfield.value;
	
	request="ajax=1&action=update&user="+rowid+"&login="+login+"&ip="+ip+"&debit="+debit+"&credit="+credit+"&mlimit="+limit+"&active="+active;
	//alert(request);
	sendRequest("users.php",request,"POST");
}

function rmuser(rowid){
	row=document.getElementById("row"+rowid);
	request="ajax=1&action=delete&user="+rowid;
	login=document.getElementById("login"+rowid).value;
	if (confirm(rmconfirm+login+'?')){
		sendRequest("users.php",request,"POST");
	}
}

function askpassword(rowid){
	row=document.getElementById("row"+rowid);
	login=document.getElementById("login"+rowid).value;
	if ( pass1=prompt(passwordprompt+login+':') ) {
		if ( pass2=prompt(passwordprompt2+login+':') ){
			if (pass1 == pass2) {
				request="ajax=1&action=password&user="+rowid+"&password="+pass1;
				sendRequest("users.php",request,"POST");
				alert("OK");
			};
		};
	};
}

function adduser(pos){
	tbody=document.getElementById("userstable").tBodies[0];
	row=document.createElement("tr");
	tbody.insertBefore(row,tbody.rows[(tbody.rows.length+1) * pos]);
	request="ajax=1&action=getemptyrow";
	sendRequest("users.php",request,"POST");
}

function doadd(){
	rowid=0;
	//row=document.getElementById("row"+rowid);
	active=document.getElementById("active"+rowid).checked;
	login=document.getElementById("login"+rowid).value;
	ip=document.getElementById("ip"+rowid).value;
	debit=document.getElementById("debit"+rowid).value;
	var limit=0
	if (limitfield=document.getElementById("limit"+rowid)) limit=limitfield.value;
	
	var credit=0
	if (creditfield=document.getElementById("credit"+rowid)) credit=creditfield.value;
	
	request="ajax=1&action=adduser&user="+rowid+"&login="+login+"&ip="+ip+"&debit="+debit+"&credit="+credit+"&mlimit="+limit+"&active="+active;
	//alert(request);
	addnew=1;
	sendRequest("users.php",request,"POST");

}

function dontadd(){
	row.innerHTML='';
}

/*
*Dynamic html is here:
*/
function insertrow(data){
	row.innerHTML=data;
	if (addnew){
		tbody=document.getElementById("userstable").tBodies[0];
		//tbody.removeChild(row);
		//tbody.appendChild(row);
		var rowid=row.cells[0].childNodes[0].value;
		row.id='row'+rowid;
		addnew=0
	};
}

function onReadyState(){
	var ready=req.readyState;
	var data=null;
	if (ready==READY_STATE_COMPLETE){
		data=req.responseText;
		if (data){
		  insertrow(data);
		}
	}
}

function modgroups(rowid){
	row=document.getElementById("row"+rowid);
	request="ajax=1&action=modgroups&user="+rowid;
	sendRequest("users.php",request,"POST");
}

function reloadrow(rowid){
	row=document.getElementById("row"+rowid);
	request="ajax=1&action=getrow&user="+rowid;
	sendRequest("users.php",request,"POST");
}

function savegroups(rowid){
	grouplist=document.getElementById("groupsel");
	row=document.getElementById("row"+rowid);
	var request = "ajax=1&action=savegroups&user="+rowid;
	var reccnt=0;
	for (var i = 0; i < grouplist.length; i++) {
		if (grouplist.options[i].selected) {
			reccnt++;
			request+="&gr"+reccnt+"=" + grouplist.options[i].value;
		}
	}
	request+="&count="+reccnt;
	sendRequest("users.php",request,"POST");
}


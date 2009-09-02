<?
require ('auth.php');

check_admin();

//Function to return row in table-view mode
function getrow($l){
	global $lang;
	global $business_mode;
	$ret='<td>';
	$ret.=$l[0];
	$ret.='</td><td>';
	$ret.=trimstr($l[1],15);
	$ret.='</td><td>';
	$ret.=trimstr($l[2],15);
	$ret.='</td><td>';
	$ret.='<input type="hidden" value="'.$l[3].'" id="gid'.$l[3].'">';
	$ret.='<input type="hidden" value="'.$l[4].'" id="zid'.$l[4].'">';
	$ret.='<a href="javascript:editzone('.$l[4].');">'.'<img src="img/edit.png"  border=0></img>'.'</a>';
	$ret.='</td>';
	return $ret;
};



//Some ajax stuff here
if (isset($_POST['ajax'])){
	if (($_POST['action']=='editzone')&&isset($_POST['zone_id'])){
		//********************************************
		//	We are editing zone, display edit fields.*
		//********************************************
		$zone_id=(int)$_POST['zone_id'];
		$group_id=(int)$_POST['group_id'];
		//Get all data about zone;
		if ($zone_id){
			$query='SELECT INET_NTOA(allzones.ip),allzones.mask,allzones.dstport,allzones.comment,allzones.id,zone_groups.priority FROM allzones,zone_groups WHERE zone_groups.zone_id=allzones.id AND allzones.id='.$zone_id;
			$zres=$mysqli->query($query);
			$zrow=$zres->fetch_row();
		}else {
			$zrow=array('','','','','');
		};
		//Get groups:
		if (!$group_id) {
			$query='SELECT groupnames.caption FROM groupnames,zone_groups WHERE groupnames.id=zone_groups.group_id AND zone_groups.zone_id='.$zone_id;
		}else{
			$query='SELECT groupnames.caption FROM groupnames WHERE groupnames.id='.$group_id;
		};
		$gres=$mysqli->query($query);
		$zgroups='';
		while($grow=$gres->fetch_row()){
			$zgroups.=$grow[0].'<br>';
		};
		echo '<table><tr><td>ip</td><td>';
		echo '<input type="text" value="'.$zrow[0].'" id="zone_ip">';
		echo '</td></tr><tr><td>маска</td><td>';
		echo '<input type="text" value="'.$zrow[1].'" id="zone_mask">';
		echo '</td></tr><tr><td>порт</td><td>';
		echo '<input type="text" value="'.$zrow[2].'" id="zone_port">';
		echo '</td></tr><tr><td>приоритет</td><td>';
		echo '<input type="text" value="'.$zrow[5].'" id="zone_priority">';
		echo '</td></tr><tr><td>комментарий</td><td>';
		echo '<input type="text" value="'.$zrow[3].'" id="zone_comment">';
		echo '<input type="hidden" value="'.$zrow[4].'" id="zone_id">';
		if ($group_id)echo '<input type="hidden" value="'.$group_id.'" id="group_id">';
		echo '</td></tr><tr><td>группа:</td><td>';
		echo $zgroups;
		echo '</td></tr></table>';
		echo '<a href=\'javascript:savezone()\'>сохранить</a> ';
		echo '<a href=\'javascript:dontsave()\'>отменить</a> ';
		echo '<a href=\'javascript:rmzone('.$zrow[4].')\'>удалить</a> ';
		die();
	}else if (($_POST['action']=='updatezone')&&isset($_POST['zone_id'])){
		//******************
		//Update zone data *
		//******************
		$zone_id=(int)$_POST['zone_id'];
		$group_id=(int)$_POST['group_id'];
		if (!($zone_ip=ip2long($_POST['zone_ip'])))$zone_ip=0;
		$zone_port=(int)($_POST['zone_port']);
		$zone_mask=(int)($_POST['zone_mask']);
		$zone_comment=$mysqli->real_escape_string($_POST['zone_comment']);
		$zone_priority=(int)($_POST['zone_priority']);
		if ($zone_id) $query='UPDATE allzones SET ip='.$zone_ip.', dstport='.$zone_port.', mask='.$zone_mask.', comment=\''.$zone_comment.'\' WHERE id='.$zone_id;
		else $query='INSERT INTO allzones(ip,dstport,mask,comment) values('.$zone_ip.','.$zone_port.','.$zone_mask.',\''.$zone_comment.'\')';
		//echo $query;
		$mysqli->query($query);
		if ($zone_id) {
			$query='UPDATE zone_groups SET priority='.$zone_priority.' WHERE zone_id='.$zone_id;
		}else if (!$zone_id && $group_id) {
			$query='INSERT INTO zone_groups(zone_id,group_id,priority) VALUES('.$mysqli->insert_id.','.$group_id.','.$zone_priority.');';
		};
		$mysqli->query($query);
		die();
	}else if (($_POST['action']=='rmzone')&&isset($_POST['zone_id'])){
		//*************
		// Remove zone*
		//*************
		$zone_id=(int)$_POST['zone_id'];
		$query='DELETE FROM allzones WHERE id='.$zone_id;
		$mysqli->query($query);
		$query='DELETE FROM zone_groups WHERE zone_id='.$zone_id;
		$mysqli->query($query);
		die();
	}else if (($_POST['action']=='editgroup')&&isset($_POST['group_id'])){
		//*************************
		// Groups modification    *
		//*************************
		$group_id=(int)$_POST['group_id'];
		$query='SELECT id,caption,mb_cost,ippoolid FROM groupnames where id='.$group_id;
		$gres=$mysqli->query($query);
		$grow=$gres->fetch_row();
		echo '<table><tr><td>Название</td><td>';
		echo '<input type="text" value="'.$grow[1].'" id="groupname">';
		echo '</td></tr><tr><td>';
		if ($business_mode) echo 'Учитывать';
		else echo 'Стоимость МБ';
		echo '</td><td>';
		if ($business_mode){
			echo '<input type="checkbox" ';
			if ($grow[2]>0)	echo 'checked';
			echo ' id="group_price">';
		}else echo '<input type="text" value="'.$grow[2].'" id="group_price">';
		echo '</td></tr><tr><td>Таблица PF</td><td>';
		echo '<input type="text" value="'.$grow[3].'" id="group_pf_tbl">';
		echo '<input type="hidden" value="'.$group_id.'" id="group_id">';
		echo '</td></tr></table>';
		echo '<a href=\'javascript:savegroup()\'>сохранить</a> ';
		echo '<a href=\'javascript:dontsave()\'>отменить</a> ';
		echo '<a href=\'javascript:rmgroup('.$grow[0].')\'>удалить</a> ';
		die();
	}else if (($_POST['action']=='updategroup')&&isset($_POST['group_id'])){
		//************************
		// Save group changes    *
		//************************
		$group_id=(int)$_POST['group_id'];
		$group_name=$mysqli->real_escape_string($_POST['group_name']);
		$group_price=(float)$_POST['group_price'];
		$group_table=$mysqli->real_escape_string($_POST['group_table']);
		if ($group_id) 
			$query='UPDATE groupnames SET caption=\''.$group_name.'\',mb_cost='.$group_price.',ippoolid=\''.$group_table.'\' WHERE id='.$group_id;
		else
			$query='INSERT INTO groupnames (caption,mb_cost,ippoolid) VALUES(\''.$group_name.'\','.$group_price.',\''.$group_table.'\')'
			;
		$mysqli->query($query);
		die();
	}else if (($_POST['action']=='rmgroup')&&isset($_POST['group_id'])){
		//*************************************
		// Delete group and all of it's zones *
		//*************************************
		$group_id=(int)$_POST['group_id'];
		if ($group_id){
			$query='DELETE FROM allzones WHERE allzones.id=zone_groups.zone_id and zone_groups.group_id='.$group_id;
			$mysqli->query($query);
			$query='DELETE FROM zone_groups WHERE group_id='.$group_id;
			$mysqli->query($query);
			$query='DELETE FROM groupnames WHERE id='.$group_id;
			$mysqli->query($query);
		};
		die();
	};
	
};

if (isset($_GET['tree_view'])) $_SESSION['zones_view']='tree';
if (isset($_GET['table_view'])) $_SESSION['zones_view']='table';
if (!$_SESSION['zones_view']) $_SESSION['zones_view']='tree';

	
if ($_SESSION['zones_view']=='tree'){
	//Tree mode is selected
	require_once 'lib/TreeMenu.php';
	$menu  = new HTML_TreeMenu();
	//Load group names
	$query = 'SELECT id,caption FROM groupnames';
	$gres=$mysqli->query($query);
	while ($group=$gres->fetch_row()){
		$menu_zones=$menu->addItem( new HTML_TreeNode(array('text' => $group[1]), array('onclick' => 'editgroup('.$group[0].')')));	
		//Load zones for each group
		$query= 'SELECT allzones.id,allzones.comment FROM allzones,zone_groups WHERE allzones.id=zone_groups.zone_id AND zone_groups.group_id = '.$group[0].' order by zone_groups.priority desc, allzones.comment asc;';
		$zres=$mysqli->query($query);
		while ($zone=$zres->fetch_row()){
			//echo $zone[0].$zone[1];
			$zonenode=$menu_zones->addItem(new HTML_TreeNode(array('text' => $zone[1] ),array('onclick' => 'editzone('.$zone[0].')')));
		};
		$zonenode=$menu_zones->addItem(new HTML_TreeNode(array('text' => 'Добавить зону' ),array( 'onclick' => 'addzone('.$group[0].')')));
	};
	$zonesMenu = &new HTML_TreeMenu_DHTML($menu, array('images' => 'img/tree/', 'defaultClass' => 'treeMenuDefault'));
	$zones="Группы траффика:<br>".$zonesMenu->toHTML();
	$zones.="<a href='javascript:editgroup(0)'>Добавить группу</a>";
}else if ($_SESSION['zones_view']=='table'){
	
	
	
	$zones='<table width=525><thead><th width=50>приор.
			</th><th width=200>группа
			</th><th width=200>зона
			</th><th>
			</th></thead>';
	
	//Table mode is selected
	$query='select zone_groups.priority,groupnames.caption,allzones.comment,groupnames.id,allzones.id from allzones,zone_groups,groupnames where allzones.id=zone_groups.zone_id and groupnames.id=zone_groups.group_id order by zone_groups.priority desc, allzones.comment;';
	$res=$mysqli->query($query);
	while ($l=$res->fetch_row()){
		$zones.='<tr>'.getrow($l).'</tr>';
	};
	$zones.='</table>';
};

if ($_SESSION['zones_view']=='table')$tree_link='<a href="zones.php?tree_view=1">древовидный</a>';
else $tree_link='древовидный';
if ($_SESSION['zones_view']=='tree')$table_link='<a href="zones.php?table_view=1">табличный</a>';
else $table_link='табличный';


?>

<html><head>
	<Meta http-equiv="Content-type" content="Text/html;charset=utf-8">
	<link rel=stylesheet href=style.css>
	<title>Управление направлениями траффика</title>
	<script src="js/ajax.js" language="JavaScript" type="text/javascript"></script>
	<script src="js/zones.js" language="JavaScript" type="text/javascript"></script>
	<script src="js/TreeMenu.js" language="JavaScript" type="text/javascript"></script>
</head><body>
	<script language='JavaScript' type='text/javascript'>
		var zone_ask_remove='Действительно удалить зону ';
		var group_ask_remove='Действительно удалить группу ';
		var needreload=0;
	</script>
	Здравствуйте, <?=$_SESSION['firstname'];?>!
		<p>Режим просмотра:
				<?=$tree_link;?>
				<?=$table_link;?>
				<br><a href="index.php">На главную</a>
		<div id='menu' style="position:relative"><?=$zones;?></div>
		<div id='edit_layer' style="position:absolute; top:100px; left:50px; visibility:hidden">fhfbgh</div>
	
</body></html>

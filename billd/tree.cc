/*
Tree for hostport stats (updated separately from session stats, summary per-session)
Inspired by http://cslibrary.stanford.edu/110/BinaryTrees.html
*/
#include "billing.h"

/*
 Given a binary tree, return pointer to a node
 with the target data. Recurs
 down the tree, chooses the left or right
 branch by comparing the target to each node.
*/
/*
host_node * fs_lookup(host_node* node, uint32_t host) {
	// 1. Base case == empty tree
	// in that case, the target is not found so return false
	if (node == NULL) {
		return(NULL);
	}else {
		// 2. see if found here
		if (host == node->host) return(node);
		else {
			// 3. otherwise recur down the correct subtree
			if (host < node->host) return(fs_lookup(node->left, host));
			else return(fs_lookup(node->right, host));
		}
	}
}
*/
/*
 Helper function that allocates a new node
 with the given data and NULL left and right
 pointers.
*/

host_node *fs_newNode(port_node* port,uint32_t host) {
	host_node* node=new host_node;
	node->host = host;
	node->port = port;
	node->left = NULL;
	node->right = NULL;
	return(node);
}

/*
	`host` int unsigned NOT NULL default 0,
	`port` smallint unsigned NOT NULL default 0,
	`traf_in` bigint unsigned NOT NULL default '0',
	`traf_out` bigint unsigned NOT NULL default '0',
	`packets_in` int unsigned NOT NULL default '0',
	`packets_out` int unsigned NOT NULL default '0',
	`session_id` int unsigned NOT NULL default '0',
	`updatescount` smallint unsigned NOT NULL default '0',
*/
void datarec_dump(stat_record* data, MYSQL *fs_link, uint32_t session_id, char* sql){
	logmsg(DBG_HPSTAT,"Host: %s, port %u, bytes in %u, bytes out %u, packets in %i, packets out %u, new_rec: %i, updated: %i, ",
	ipFromIntToStr(data->host), data->port, data->bytes_in, data->bytes_out, data->packets_in,
	data->packets_out, data->new_rec ? 1 : 0, data->updated ? 1 : 0);
	//Query in case of a new hostport record:
	if (data->new_rec){
		sprintf(sql,"INSERT INTO hostport_stat (host, port, traf_in, traf_out, packets_in, packets_out, session_id) VALUES (%u,%u, %u, %u, %u, %u, %u)",
		data->host, data->port, data->bytes_in, data->bytes_out, data->packets_in, data->packets_out, session_id );
		logmsg(DBG_HPSTAT,"%s", sql);
		mysql_query(fs_link, sql);
	}else if (data->updated){
		//Query in case of hostport record update:
		sprintf(sql,"UPDATE hostport_stat SET traf_in=traf_in+%u, traf_out=traf_out+%u, packets_in=packets_in+%u, packets_out=packets_out+%u, updatescount=updatescount+1 WHERE session_id=%u AND host=%u AND port=%u",
		data->bytes_in, data->bytes_out, data->packets_in, data->packets_out, session_id, data->host, data->port );
		logmsg(DBG_HPSTAT,"%s", sql);
		mysql_query(fs_link, sql);
	};
	data->packets_in=0;
	data->bytes_in=0;
	data->bytes_out=0;
	data->packets_out=0;
	data->new_rec = false;
	data->updated = false;

};

/*
 Given a binary search tree, dump
 its data elements in increasing
 sorted order.
*/
void porttree_dump(port_node* node, MYSQL *fs_link, uint32_t session_id, char* sql) {
	if (node == NULL) return;
	porttree_dump(node->left, fs_link, session_id,sql);
	datarec_dump(node->data,fs_link, session_id,sql);
	porttree_dump(node->right, fs_link, session_id,sql);
};

void fstree_dump(host_node* node, MYSQL *fs_link, uint32_t session_id, char* sql) {
	if (node == NULL) return;
	fstree_dump(node->left, fs_link, session_id,sql);
	porttree_dump(node->port, fs_link, session_id,sql);
	fstree_dump(node->right, fs_link, session_id,sql);
};


/*
	Update user's flow stats for current session:
*/
void * flowstatsupdater(void *threadid) {
	logmsg(DBG_THREADS,"started hostport stats offloading thread");
	MYSQL *fs_link=connectdb();
	char * sql = new char[1024];
	while (cfg.stayalive) {
	if ((time(NULL) - cfg.fs_updated_time) > cfg.fs_update_interval) {
			logmsg(DBG_HPSTAT,"Updating hostport stats in MySql...");
			cfg.fs_updated_time=time(NULL);
			verbose_mutex_lock (&users_table_m);
			for (user * u = firstuser; u != NULL; u = u->next) {
				verbose_mutex_unlock (&users_table_m);
				verbose_mutex_lock (&(u->user_mutex));
				logmsg(DBG_HPSTAT,"Session %s:",u->verbose_session_id.c_str());
				//Update user's hostport stats in MySQL:
				fstree_dump(u->hostport_tree, fs_link, u->session_id, sql);
				verbose_mutex_unlock (&(u->user_mutex));
				verbose_mutex_lock (&users_table_m);
			}
			verbose_mutex_unlock (&users_table_m);
		}
		sleep(1);
	}
	delete sql;
	logmsg(DBG_THREADS,"Finishing flow stats offloading thread");
	mysql_close(fs_link);
	mysql_thread_end();
	logmsg(DBG_THREADS,"Complete flow stats offloading thread");
	pthread_exit(NULL);
};

void port_deleteTree(port_node* node){
	if (node == NULL) return;
	port_deleteTree(node->left);
	delete(node->data);
	port_deleteTree(node->right);
	delete(node);
	return;
};

/*
 Given a binary search tree,
 delete it and it's contents
 btw, it's safe to delete null pointers
*/
void fs_deleteTree(host_node* node){
	if (node == NULL) return;
	fs_deleteTree(node->left);
	port_deleteTree(node->port);
	fs_deleteTree(node->right);
	delete(node);
	return;
};


/*
 Helper function that allocates a new node
 with the given data and NULL left and right
 pointers.
*/
port_node *fs_newPort(stat_record* data) {
	port_node* node = new port_node;
	node->data = data;
	node->left = NULL;
	node->right = NULL;
	return(node);
}

/*
	Find or create node for this port:
*/
port_node *find_port(port_node* pnode, stat_record* data, port_node* parent){
	if (pnode == NULL) {
		//not found record, create new one:
		port_node* myself = fs_newPort(data);
		if (parent != NULL){
			//If not at the top of the tree, attach new record to it's parent node
			if (data->port < parent->data->port) parent->left=myself;
			else parent->right=myself;
		};
		return myself;
	}else{
		// If found record with matching port, return it
		if (data->port == pnode->data->port) return(pnode);
		else {
			//otherwise recur down the correct subtree
			if (data->port < pnode->data->port) return(find_port(pnode->left, data, pnode));
			else return(find_port(pnode->right, data, pnode));
		};
	};
};

/*
 Add or update host-port record in tree:
*/
host_node *fs_update(host_node* node,uint32_t host, stat_record* data,host_node* parent){
	// 1. Empty tree or new record:
	if (node == NULL){
		port_node* port;
		port=find_port(NULL,data,NULL);
		if (parent==NULL) return(fs_newNode(port,host));
		else {
			if (host < parent->host) parent->left=fs_newNode(port,host);
			else parent->right=fs_newNode(port,host);
			return NULL;
		};
	}else {
		// 2. if found here, update record:
		if (host == node->host) {
			//Look for port record:
			port_node* port = find_port(node->port, data, NULL);
			//If first node, update host_node;
			if (node->port == NULL) node->port=port;
			//If found existing, update it and delete temp, else skip;
			if (port->data != data) {
				port->data->bytes_in+= data->bytes_in;
				port->data->bytes_out+= data->bytes_out;
				port->data->packets_in+=data->packets_in;
				port->data->packets_out+=data->packets_out;
				port->data->updated=true;
				delete data;
			}
			return NULL;
		}else {
			// 3. otherwise recur down the correct subtree
			if (host < node->host) fs_update(node->left, host, data, node);
			else fs_update(node->right, host, data,node);
			return NULL;
		};
	};
};

/*
Tree for hostport stats (updated separately from session stats, summary per-session)
Inspired by http://cslibrary.stanford.edu/110/BinaryTrees.html
*/

#ifndef TREE_H_INCLUDED
#define TREE_H_INCLUDED

typedef struct stat_record{
	uint32_t host;		//Peer host
	uint16_t port;		//Peer port
	uint64_t bytes_in;	//overall recieved bytes
	uint64_t bytes_out;	//overall sent bytes
	uint32_t packets_in;	//overall recieved packets
	uint32_t packets_out;	//overall sent packets
	bool new_rec;		//Need insert instead of update
	bool updated;		//Updated since last offload
};

typedef struct port_node {
	stat_record* data;
	struct port_node* left;
	struct port_node* right;
};

typedef struct host_node {
	port_node* port;
	uint32_t host;				//Peer host
    struct host_node* left;
    struct host_node* right;
};

//host_node * fs_lookup(host_node* node, stat_record* record);

//host_node *fs_insert(host_node* node, stat_record* data);
void * flowstatsupdater(void *threadid);
host_node *fs_update(host_node* node,uint32_t host, stat_record* data,host_node* parent);


//void fs_printTree(host_node* node);
void fs_deleteTree(host_node* node);

#endif // TREE_H_INCLUDED

/*
Class for reading & store config options
*/

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "ConfigFile.h"
#include <netinet/in.h>	//sockaddr_in
#include <arpa/inet.h>
#include <sys/socket.h>

class Config {
	private:


	public:
	//listeners configuration (source addr & port moved to NAS configuration)
	//NetFlow Listener
	uint16_t netflow_listen_port;
	in_addr netflow_listen_addr;
	uint16_t netflow_timeout;
	//Event listener
	uint16_t events_listen_port;
	in_addr events_listen_addr;
	uint16_t events_timeout;
	uint16_t events_threads;

	sockaddr_in events_addr;
	//Here we connect to drop users
	uint16_t mpd_shell_port;
	string mpd_shell_addr;
	string mpd_shell_user;
	string mpd_shell_pass;
	//MySql configuration
	string mysql_server;
	uint16_t mysql_port;
	string mysql_username;
	string mysql_password;
	string mysql_database;
	//terminate application
	bool stayalive;
	//put stats into DB
	uint32_t stats_updated_time;
	uint32_t stats_update_interval;
	//host-port flow stats updates:
	uint32_t fs_updated_time;
	uint32_t fs_update_interval;
	//to remove disconnnected users
	uint32_t die_time_interval;
	bool debug_locks;
	bool debug_netflow;
	bool debug_offload;
	bool debug_events;
	bool debug_threads;
	bool debug_hpstat;
	bool verbose_daemonize;
	bool do_fork;
	bool appendlogs;
	bool log_date;
	string logfile;
	string pidfile;
	string lockfile;
	string user;
	string workingdir;

	bool readconfig(const char*);


};

#endif // CONFIG_H_INCLUDED

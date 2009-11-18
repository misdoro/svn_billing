#ifndef NASLIST_H_INCLUDED
#define NASLIST_H_INCLUDED

class NASList{
	private:
		pthread_mutex_t list_mutex;
		pthread_mutex_t id_mutex;
		pthread_mutex_t userSidMutex;
		std::map<uint16_t,C_NAS*> naslist;
		std::map<uint16_t,C_NAS*>::iterator nasit;
		std::map<uint32_t,C_NAS*> listById;
		std::map<uint32_t,C_NAS*>::iterator listByIdIt;
		std::map<uint32_t,C_user*> usersBySID;
		MYSQL *sqllink;

	public:
		NASList();
		uint32_t load();
		C_NAS* getbyport(uint16_t);
		C_NAS* getById(uint32_t);
		void UserConnected (char*);
		void UserDisconnected (char*);
};

class Session{
	private:
		uint64_t unique_id;
		std::string nas_linkname;
		uint32_t user_id;
		uint32_t session_id;
		uint32_t ppp_ip;
		uint32_t nas_id;
		uint32_t start_time;
	public:
		Session(char* ,MYSQL*);
		void filluser(C_user*);
		uint32_t get_nas_id(void);
};

#endif // NASLIST_H_INCLUDED


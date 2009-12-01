#ifndef NASLIST_H_INCLUDED
#define NASLIST_H_INCLUDED

class NASList{
	private:
		pthread_mutex_t list_mutex;
		pthread_mutex_t id_mutex;
		pthread_mutex_t userSidMutex;
		std::map<uint16_t,C_NAS*> naslist;
		std::map<uint32_t,C_NAS*> listById;

		std::map<uint32_t,C_user*> usersBySID;
		MYSQL *sqllink;

	public:
		NASList();
		~NASList();
		uint32_t load();

		C_NAS* getbyport(uint16_t);
		C_NAS* getById(uint32_t);

		void UserConnected (char*);
		void UserDisconnected (char*);
		void UserDeleted(C_user*);
};

#endif // NASLIST_H_INCLUDED


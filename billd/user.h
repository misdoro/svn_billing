#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

class C_user{
	private:
		uint32_t id;
		uint32_t bill_id;
		uint32_t nas_id;
		uint32_t user_ip;
		double user_debit_diff;
		uint32_t session_id;
		uint64_t unique_session_id;
		std::string nasLinkName;
		uint32_t session_start_time;
		uint32_t session_end_time;
		uint32_t die_time;
		bool debit_changed;
		//user_zone *first_user_zone;
		//zone_group *first_zone_group;
		pthread_mutex_t user_mutex;
		pthread_t user_drop_thread;
		//host_node *hostport_tree;
		void getsession(char* sessionid);
		void loadzones();
	public:
		C_user(char* sessionid);
		uint32_t getNASId(void);
		uint32_t getIP(void);
		int droupser();
};

#endif // USER_H_INCLUDED

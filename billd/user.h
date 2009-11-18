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

		pthread_mutex_t user_mutex;
		pthread_t user_drop_thread;

		struct zone_group {
			uint32_t id;
			uint64_t in_bytes;
			uint64_t out_bytes;
			bool group_changed;
			uint64_t in_diff;
			uint64_t out_diff;
			double zone_mb_cost;
		};
		struct zone{
			zone_group *group_ref;
			uint32_t id;
			uint32_t zone_ip;
			uint8_t zone_mask;
			uint16_t zone_dstport;
		};

		std::list<zone*> zones;
		std::map<uint32_t,zone_group*> groups;
		std::map<uint32_t,zone_group*>::iterator groupIt;
		/*Don't need mutexes cause modifying only upon creation, before
		* user becomes accessible to stats updating threads.
		*/
		void loadgroups(MYSQL*);
		void getsession(char*,MYSQL*);
		void loadzones(MYSQL*);
		void init_mutex(void);
	public:
		C_user(char* sessionid);
		C_user(char* sessionid,MYSQL*);
		void load(MYSQL*);
		void load();
		uint32_t getNASId(void);
		uint32_t getSID(void);
		uint32_t getIP(void);
		bool updateTraffic(uint32_t,uint16_t,uint32_t,int8_t);
		void userDisconnected();
		int droupser();
};

#endif // USER_H_INCLUDED

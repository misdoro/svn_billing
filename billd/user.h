#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#ifndef NAS_H_INCLUDED
class C_NAS;
#endif

class C_user: public thread{
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
		bool debit_changed;

		C_NAS* myNAS;

		pthread_mutex_t user_mutex;

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

        rwLock zgstatlock;  //Stats update lock

		void loadgroups(MYSQL*);
		void getsession(char*,MYSQL*);
		void loadzones(MYSQL*);
		void init_mutex(void);

		//Those to separate later to netflow class, netflow related functions
		void* getFlowZone(uint32_t,uint16_t);                   //Get flow zone for selected host and port
		void updateGroupTraffic(zone_group*,uint32_t,int8_t);   //Update traffic details for selected zone group.
		uint32_t mask_ip(uint32_t, uint8_t);                    //mask IP according to mask.


	public:
        //Constructors:
		C_user(char* sessionid);
		C_user(char* sessionid,MYSQL*);
		//Destructor:
		~C_user(void);

		void load(MYSQL*);
		void load();

		void setNAS(C_NAS*);

		uint32_t getNASId(void);
		uint32_t getSID(void);
		uint32_t getIP(void);

		uint32_t updateTraffic(uint32_t,uint16_t,uint32_t,int8_t);

		void updateStats(MYSQL*);

		bool checkDebit(MYSQL*);

		void userDisconnected();

		bool checkDelete(void);//Check if user is ready to be deleted, delete internal structures

		bool checkFlowTimes(uint32_t,uint32_t);

        void runThread();//Thread to disconnect user

		void dropUser();
};

#endif // USER_H_INCLUDED

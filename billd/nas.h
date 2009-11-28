#ifndef NAS_H_INCLUDED
#define NAS_H_INCLUDED

class C_NAS {
	private:
		uint16_t flow_src_port; 			//NetFlow source port
		sockaddr_in flow_src_addr; 			//NetFlow source address
		sockaddr_in event_src_addr;			//Events source address
		std::string name;						//NAS name
		uint32_t id;						//NAS ID
		std::map<uint32_t,C_user*> usersByIP;	//users list
		pthread_mutex_t listByIP_mutex;			//users list mutex
		std::map<uint32_t,C_user*> usersBySID;	//users by session ID
		pthread_mutex_t listBySID_mutex;		//users list mutex
	public:
		static MYSQL *sqllink;
		C_NAS(MYSQL_ROW);
		uint16_t getFlowSrcPort(void);
		uint32_t getId(void);
		void add_user(C_user*);
		C_user* getUserByIP(uint32_t,uint32_t,uint32_t);
		const char* getName();

//		void dropUser(&C_user);
};

#endif // NAS_H_INCLUDED

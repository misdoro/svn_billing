#ifndef NAS_H_INCLUDED
#define NAS_H_INCLUDED
#include "threads.h"

class C_NAS : public thread {
	private:
		//uint16_t flow_src_port; 			//NetFlow source port
		sockaddr_in flow_src_addr; 			//NetFlow source address&port
		sockaddr_in event_src_addr;			//Events source address
		std::string name;						//NAS name
		uint32_t id;						//NAS ID
		std::multimap<uint32_t,C_user*> usersByIP;	//users list
		std::map<uint32_t,C_user*> usersBySID;	//users by session ID
		rwLock mylock;

        pthread_t suThread;
		void * statsUpdater(void *threadid);//users stats updater thread

	public:
		static MYSQL *sqllink;
		C_NAS(MYSQL_ROW);
		uint16_t getFlowSrcPort(void);
		uint32_t getId(void);
		void add_user(C_user*);
		C_user* getUserByIP(uint32_t,uint32_t,uint32_t);
		const char* getName();

        void startOffload();
        void runThread();


};

#endif // NAS_H_INCLUDED

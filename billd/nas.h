#ifndef NAS_H_INCLUDED
#define NAS_H_INCLUDED
#include "threads.h"

class C_NAS : public thread {
	private:
		sockaddr_in flow_src_addr; 			//NetFlow source address&port
		sockaddr_in event_src_addr;			//Events source address
		sockaddr_in shell_addr;              //MPD shell address
		std::string shell_login;             //MPD shell login
		std::string shell_password;          //MPD shell password
		std::string name;					//NAS name
		uint32_t id;						//NAS ID
		std::multimap<uint32_t,C_user*> usersByIP;	//users list
		std::map<uint32_t,C_user*> usersBySID;	//users by session ID
		rwLock mylock;


	public:
		static MYSQL *sqllink;
		C_NAS(MYSQL_ROW);
		~C_NAS();

		uint16_t getFlowSrcPort(void);
		uint32_t getId(void);
		void add_user(C_user*);
		C_user* getUserByIP(uint32_t,uint32_t,uint32_t);
		const char* getName();

        void startOffload();
        void runThread();

        sockaddr_in getShellAddr();
        const char* getShellLogin();
        const char* getShellPassword();


};

#endif // NAS_H_INCLUDED

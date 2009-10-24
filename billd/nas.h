#ifndef NAS_H_INCLUDED
#define NAS_H_INCLUDED

class C_NAS {
	private:
		uint16_t flow_src_port; //NetFlow source port
		uint32_t flow_src_addr; //NetFlow source address
		uint32_t event_src_addr;
		string name;
		uint32_t id;
		list<C_user> users;

	public:
		static MYSQL *sqllink;
		C_NAS();
		dropUser(&C_user);



};

#endif // NAS_H_INCLUDED

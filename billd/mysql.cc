#include "billing.h"

MYSQL * connectdb () {
	MYSQL *lnk;
	lnk = mysql_init(NULL);
	lnk = mysql_real_connect(lnk, cfg.mysql_server, 
                                      cfg.mysql_username, 
                                      cfg.mysql_password, 
                                      cfg.mysql_database,
                                      cfg.mysql_port, NULL, 0);
        cfg.myconn = lnk;
	return lnk;
}

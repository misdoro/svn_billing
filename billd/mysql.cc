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

void * statsupdater(void *threadid) {
   // update values in database
   while (cfg.terminate == 0) {
    pthread_mutex_lock (&users_table_m);
    if ((time(NULL) - cfg.stats_updated_time) > cfg.stats_update_interval) {
        printf("Updating stats in MySql...\n");        
        cfg.stats_updated_time = time(NULL);
        for (user * u = firstuser; u != NULL; u = u->next) {
            for (zone_group * p = u->first_zone_group; p != NULL; p = p->next) {
                if ((p->group_changed == 1) || ((time(NULL) - u->die_time) > cfg.die_time_interval)) {
                    char * sql = new char[1024];
                    uint64_t in_bytes = p->in_bytes;
                    uint64_t out_bytes = p->out_bytes;
                    uint32_t pid = p->id;
                    uint32_t uid = u->id;
                    float traf_in_money = p->in_mb_cost_total;
                    // connected=2 - user disconnected, but not removed from DB
                    sprintf(sql, "UPDATE statistics SET traf_in=%llu, traf_out=%llu, traf_in_money=%f WHERE (user_id=%lu) AND (zone_group_id=%lu) AND ((connected=1) OR (connected=2))", in_bytes, out_bytes, traf_in_money, uid, pid);
                    mysql_query(cfg.myconn, sql);
                    printf("%s\n",sql);
                    delete sql;
                    p->group_changed = 0;
                }
            }
            if ((u->debit_changed == 1) || ((time(NULL) - u->die_time) > cfg.die_time_interval)) {
                 u->debit_changed = 0;
                 char * sql = new char[1024];
                 sprintf(sql, "UPDATE users SET debit=%f WHERE id=%u", u->user_debit,u->id);
                 mysql_query(cfg.myconn, sql);
                 printf("%s\n",sql);
                 delete sql;                
            }
        }
        
    }
    // for remove disconnected users from table
    user * next_u;
    for (user * u = firstuser; u != NULL;) {
        if (u->die_time == 0) continue;
        if ((time(NULL) - u->die_time) > cfg.die_time_interval) {
            // remove died user
            next_u = u->next;
            removeUser(u);
            u = next_u;
            continue;
        }
        u = u->next;
    }
    pthread_mutex_unlock (&users_table_m);
    sleep(1);
   }
}
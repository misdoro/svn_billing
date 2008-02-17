#include "billing.h"

MYSQL * connectdb () {
	MYSQL *lnk;
	lnk = mysql_init(NULL);
	lnk = mysql_real_connect(lnk, cfg.mysql_server.c_str(),
					cfg.mysql_username.c_str(),
					cfg.mysql_password.c_str(),
					cfg.mysql_database.c_str(),
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
						uint32_t sid = u->session_id;
						float traf_in_money = p->in_mb_cost_total;
						// connected=2 - user disconnected, but not removed from DB
						//sprintf(sql, "UPDATE session_statistics SET traf_in=%llu, traf_out=%llu, traf_in_money=%f WHERE (session_id=%lu) AND (zone_group_id=%lu)", in_bytes, out_bytes, traf_in_money, sid, pid);
						mysql_query(cfg.myconn, sql);
						printf("%s\n",sql);
						delete sql;
						p->group_changed = 0;
					}
				}
				if ((u->debit_changed == 1) || ((time(NULL) - u->die_time) > cfg.die_time_interval)) {
					u->debit_changed = 0;
					char * sql = new char[1024];
					sprintf(sql, "UPDATE users SET debit=debit-%f WHERE id=%u", u->user_debit_diff,u->id); /*Its better to update debit this way IMHO*/
					mysql_query(cfg.myconn, sql);
					u->user_debit_diff=0;
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
				// remove dead user
				next_u = u->next;
				removeUser(u);
				u = next_u;
				continue;
			}else{
				u = u->next;
			};
		}
		pthread_mutex_unlock (&users_table_m);
		sleep(1);
	}
	pthread_exit(NULL);
}

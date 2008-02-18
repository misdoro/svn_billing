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
	printf("started stats offloading thread\n");
	// update values in database
	while (cfg.terminate == 0) {
		if ((time(NULL) - cfg.stats_updated_time) > cfg.stats_update_interval) {
			printf("Updating stats in MySql...\n");        
			cfg.stats_updated_time = time(NULL);
			pthread_mutex_lock (&users_table_m);
			for (user * u = firstuser; u != NULL; u = u->next) {
				zone_group * p = u->first_zone_group;
				pthread_mutex_unlock (&users_table_m);
				pthread_mutex_lock (&(u->user_mutex));
				double charged_money=0;
				//Save stat record for each of user's groups:
				while (p != NULL){
					if (p->group_changed == 1) {
						char * sql = new char[1024];
						double money = (double) p->in_diff * p->zone_mb_cost / (double) MB_LENGTH;
						charged_money += money;
						sprintf(sql, "insert into session_statistics (zone_group_id,session_id,traf_in,traf_out,traf_in_money) values (%lu,%lu,%llu,%llu,%f);",p->id,u->session_id,p->in_diff,p->out_diff,money);
						mysql_query(cfg.myconn, sql);
						printf("%s\n",sql);
						delete sql;
						p->in_diff=0;
						p->out_diff=0;
						p->group_changed = 0;
					}
					p = p->next;
				}
				//Update user's debit:
				if ( charged_money>0 ) {
					char * sql = new char[1024];
					sprintf(sql, "UPDATE users SET debit=debit-%f WHERE id=%u", charged_money,u->id); /*Its better to update debit this way IMHO*/
					mysql_query(cfg.myconn, sql);
					printf("%s\n",sql);
					delete sql;
				}
				pthread_mutex_unlock (&(u->user_mutex));
				pthread_mutex_lock (&users_table_m);
			}
			pthread_mutex_unlock (&users_table_m);
		}
		// for remove disconnected users from table
		user * next_u;
		pthread_mutex_lock (&users_table_m);
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

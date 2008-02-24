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
			if (cfg.debug_offload) printf("Updating stats in MySql...\n");        
			cfg.stats_updated_time = time(NULL);
			verbose_mutex_lock (&users_table_m);
			for (user * u = firstuser; u != NULL; u = u->next) {
				if (cfg.debug_offload) printf ("Session %s:\n",u->verbose_session_id.c_str());
				zone_group * p = u->first_zone_group;
				verbose_mutex_unlock (&users_table_m);
				verbose_mutex_lock (&(u->user_mutex));
				double charged_money=0;
				//Save stat record for each of user's groups:
				while (p != NULL){
					if (cfg.debug_offload) printf("   group %i: in %lu out %lu \n",p->id, p->in_diff,p->out_diff);
					if (p->group_changed == 1) {
						char * sql = new char[1024];
						double money = (double) p->in_diff * p->zone_mb_cost / (double) MB_LENGTH;
						charged_money += money;
						sprintf(sql, "insert into session_statistics (zone_group_id,session_id,traf_in,traf_out,traf_in_money) values (%i,%i,%lu,%lu,%f);",p->id,u->session_id,p->in_diff,p->out_diff,money);
						verbose_mutex_lock (&mysql_mutex);
						mysql_query(cfg.myconn, sql);
						verbose_mutex_unlock (&mysql_mutex);
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
					verbose_mutex_lock (&mysql_mutex);
					mysql_query(cfg.myconn, sql);
					verbose_mutex_unlock (&mysql_mutex);
					printf("%s\n",sql);
					delete sql;
				}
				//Check user's debit and drop him if he is out of funds:
				MYSQL_RES *result;
				char * sql = new char[1024];
				sprintf(sql, "SELECT users.debit+users.credit from users where users.id=%i", u->id);
				verbose_mutex_unlock (&(u->user_mutex));
				verbose_mutex_lock (&mysql_mutex);
				mysql_query(cfg.myconn, sql);
				delete sql;
				result = mysql_store_result(cfg.myconn);				
				MYSQL_ROW row = mysql_fetch_row(result);
				//Disconnect user if he has not enough money!
				if (atof(row[0])<0){
					disconnect_user(u);
				};
				mysql_free_result(result);
				verbose_mutex_lock (&mysql_mutex);
				verbose_mutex_lock (&users_table_m);
			}
			verbose_mutex_unlock (&users_table_m);
		}
		// for remove disconnected users from table
		user * next_u;
		verbose_mutex_lock (&users_table_m);
		for (user * u = firstuser; u != NULL;) {
			if (((time(NULL) - u->die_time) > cfg.die_time_interval)&& u->die_time !=0) {
				// remove dead user
				next_u = u->next;
				removeUser(u);
				u = next_u;
				continue;
			}else{
				u = u->next;
			};
		}
		verbose_mutex_unlock (&users_table_m);
		sleep(1);
	}
	pthread_exit(NULL);
}

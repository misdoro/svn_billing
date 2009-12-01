#include "billing.h"


pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void logmsg ( uint8_t flags, const char* message, ...)
{
	//Check if logging category enabled:
	if ((flags & DBG_LOCKS && cfg.debug_locks) ||
		(flags & DBG_EVENTS && cfg.debug_events) ||
		(flags & DBG_NETFLOW && cfg.debug_netflow) ||
		(flags & DBG_OFFLOAD && cfg.debug_offload) ||
		(flags & DBG_DAEMON && cfg.verbose_daemonize) ||
		(flags & DBG_THREADS && cfg.debug_threads) ||
		(flags & DBG_HPSTAT && cfg.debug_hpstat) ||
		(flags & DBG_ALWAYS))
	{
		pthread_mutex_lock(&log_mutex);
		if (cfg.log_date){
			time_t rawtime;
			time(&rawtime);
			char buf[50];
			strftime(buf,50, "%c", localtime ( &rawtime ));
			cout << buf << ": " ;
		};
		switch (flags) {
			case DBG_LOCKS:
				cout<< "Locks     :";
			break;
			case DBG_EVENTS:
				cout<< "Events    :";
			break;
			case DBG_NETFLOW:
				cout<< "NetFlow   :";
			break;
			case DBG_OFFLOAD:
				cout<< "Stats     :";
			break;
			case DBG_DAEMON:
				cout<< "Daemon    :";
			break;
			case DBG_THREADS:
				cout<< "Threads   :";
			break;
			case DBG_HPSTAT:
				cout<< "Det.stats :";
			break;
				cout<< "Debug     :";
		};
		va_list args;
		va_start (args, message);
		vprintf (message, args);
		if (errno && errno!= EAGAIN){
			cout << " " << strerror(errno);
			errno=0;
		};
		cout << endl;
		va_end (args);
		pthread_mutex_unlock(&log_mutex);
		errno=0;
	};
}

int verbose_mutex_lock(pthread_mutex_t *mutex){
	int res = pthread_mutex_trylock(mutex);
	if (res==0)	logmsg(DBG_LOCKS,"lock and go %i",mutex);
	else{
		pthread_mutex_lock(mutex);
		logmsg(DBG_LOCKS,"lock and wait %i",mutex);
	};
	return res;
};

int verbose_mutex_unlock(pthread_mutex_t *mutex){
	int res=pthread_mutex_unlock(mutex);
	if (res==0) logmsg(DBG_LOCKS,"unlocked %i",mutex);
	else logmsg(DBG_ALWAYS,"was not locked %i",mutex);
	return res;
};

MYSQL * connectdb () {
	MYSQL *lnk;
	lnk = mysql_init(NULL);
	lnk = mysql_real_connect(lnk, cfg.mysql_server.c_str(),
					cfg.mysql_username.c_str(),
					cfg.mysql_password.c_str(),
					cfg.mysql_database.c_str(),
					cfg.mysql_port, NULL, 0);
	//enable auto-reconnect feature
	my_bool reconnect = 1;
	mysql_options(lnk, MYSQL_OPT_RECONNECT, &reconnect);
	logmsg(DBG_ALWAYS,"MySQL is thread-safe: %i",mysql_thread_safe());
	return lnk;
}

char *ipFromIntToStr(uint32_t ip)
{
	in_addr a;
	a.s_addr = htonl(ip);
	return inet_ntoa(a);
}

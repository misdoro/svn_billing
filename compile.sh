g++ -pthread -ansi -lmysqlclient -L/usr/local/lib/mysql -I/usr/local/include -O2 -Wall -o bin/billd billd/billing.cc billd/misc.cc billd/mysql.cc billd/netflow.cc billd/userconnect.cc

#include "billing.h"

//Get pointer to packet owner user:
user * getuserbyip(uint32_t psrcaddr, uint32_t pdstaddr) {
    for (user * p = firstuser; (p != NULL); p = p->next) {
        if ((psrcaddr == p->user_ip) || (pdstaddr == p->user_ip)) {
            return p;
        }
    }
    return NULL;
}

//shift given ip number by mask bits:
uint32_t mask_ip(uint32_t unmasked_ip, uint8_t mask) {
    if (mask == 0) return 0;
    return unmasked_ip >> (32 - mask);
}

//Get pointer to matching zone:
user_zone * getflowzone(user * curr_user, uint32_t dst_ip) {
    uint32_t dst_ip_masked;
    uint32_t zone_ip_masked;
    for (user_zone * p = curr_user->first_user_zone; (p != NULL); p = p->next) {
        dst_ip_masked=mask_ip(dst_ip, p->zone_mask);
        zone_ip_masked=mask_ip(p->zone_ip, p->zone_mask);
//        if (curr_user->id == 10)
//        printf("dst_masked: %s, zone_masked %s, dst ip: %s, zone ip:%s\n",ipFromIntToStr(dst_ip_masked), ipFromIntToStr(zone_ip_masked), ipFromIntToStr(dst_ip), ipFromIntToStr(p->zone_ip));
        if (dst_ip_masked==zone_ip_masked) {
            return p;
        }
    }
    return NULL;
}

void addUser(user * u) {
    if (firstuser == NULL) {
        firstuser = u;
        firstuser->next = NULL;
        return;
    }
    user * p;
    for (p = firstuser; (p->next != NULL); p = p->next);
    u->next = NULL;
    p->next = u;
}

char * ipFromIntToStr(uint32_t ip) {
    in_addr a;
    a.s_addr = htonl(ip);
    char * addr = inet_ntoa(a);
    char * myaddr = new char[strlen(addr) + 1];
    strcpy(myaddr, addr);
    return myaddr;
}
    
void makeDBready() {
    char * sql;
    sql = new char[1024];
    sprintf(sql, "UPDATE statistics SET connected = 0");
    mysql_query(cfg.myconn, sql);
    delete sql;
}

user * onUserConnected(uint32_t dst_ip, uint32_t real_ip) {
    char * sql;
    MYSQL_RES * result;
    // get user info
    sql = new char[1024];
    char * str_ip = ipFromIntToStr(dst_ip); // user ip
    sprintf(sql, "SELECT id, debit, kredit FROM users WHERE user_ip='%s' LIMIT 1", str_ip);
    mysql_query(cfg.myconn, sql);
    delete str_ip;  delete sql;
    result = mysql_store_result(cfg.myconn);
    if (mysql_num_rows(result) == 0) {
        printf ("Warning! User with ip %s not found in database\n", ipFromIntToStr(dst_ip));
// !!! - drop user here
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    // create user structure
    user * newuser = new user;
    newuser->next = NULL;
    newuser->id = atoi(row[0]);
    newuser->user_debit = atof(row[1]);
    newuser->user_kredit = atof(row[2]);
    newuser->user_ip = dst_ip;
    newuser->first_user_zone = NULL;
    newuser->first_zone_group = NULL;
    newuser->debit_changed = 0;
        printf("User info - id:%s, debit:%s, kredit:%s\n", row[0], row[1], row[2]);
    mysql_free_result(result);
    
    // get user groups
    sql = new char[1024];
    sprintf(sql, "SELECT group_id FROM usergroups WHERE user_id=%i LIMIT 128", newuser->id);
    mysql_query(cfg.myconn, sql);
    delete sql;
    result = mysql_store_result(cfg.myconn);
    if (mysql_num_rows(result) == 0) {
        printf ("Warning! No groups for user %i found.\n", newuser->id);
// !!! - drop user here
        delete newuser;
        return NULL;
    }
    while ((row = mysql_fetch_row(result)) != NULL) {
        // add zone groups
        zone_group * newgroup = new zone_group;
        newgroup->next = NULL;
        newgroup->id = atoi(row[0]);
        newgroup->in_bytes = 0;
        newgroup->out_bytes = 0;
        newgroup->in_mb_cost_total = 0;
        newgroup->group_changed = 0;
        if (newuser->first_zone_group == NULL) {
            newuser->first_zone_group = newgroup;
        } else {
            zone_group * p;
            for (p = newuser->first_zone_group; (p->next != NULL); p = p->next);
            p->next = newgroup;
        }
        printf ("User %i. Group %i\n", newuser->id, newgroup->id);
        // insert statistics fields
        sql = new char[2048];
        sprintf(sql, "INSERT INTO statistics (`id`, `user_id`, `real_ip`, `taken_ip`, `ip_digit`, `zone_group_id`, `traf_in`, `traf_out`, `traf_in_money`, `sess_start`, `sess_end`, `connected`)");
        sprintf(sql, "%s  VALUES (NULL, %lu, %lu, %lu, %u, %u, 0, 0, 0, CURRENT_TIMESTAMP, '0000-00-00 00:00:00', 1) ",sql, newuser->id, real_ip, dst_ip, 0, newgroup->id );
        mysql_query(cfg.myconn, sql);
        delete sql;
    }
    mysql_free_result(result);
    // load zones - create sql query
    sql = new char[32768];
    sprintf(sql, "SELECT allzones.id, zone_groups.group_id, allzones.ip, allzones.mask, allzones.dstport, zone_groups.mb_cost FROM allzones INNER JOIN zone_groups ON zone_groups.zone_id = allzones.id WHERE ");
    for (zone_group * p = newuser->first_zone_group; (p!=NULL); p = p->next ) {
        sprintf(sql, "%s (zone_groups.group_id = %i)",sql, p->id);
        if (p->next != NULL) sprintf(sql,"%s OR ", sql);
    }
    sprintf(sql, "%s ORDER BY zone_groups.priority DESC",sql);
    mysql_query(cfg.myconn, sql);
    delete sql;
    result = mysql_store_result(cfg.myconn);
    while ((row = mysql_fetch_row(result)) != NULL) {
        // create new cost zone
        user_zone * newzone = new user_zone;
        newzone->next = NULL;
        newzone->id = atoi(row[0]);
        newzone->zone_group_id = atoi(row[1]);
        newzone->zone_ip = atoll(row[2]);
        newzone->zone_mask = atoi(row[3]);
        newzone->zone_dstport = atoi(row[4]);
        newzone->zone_mb_cost = atof(row[5]);
        newzone->zone_in_bytes = 0;
        newzone->zone_out_bytes = 0;
        printf("Loaded zone. id=%i, zone_group_id=%i, ip=%s\n", newzone->id, newzone->zone_group_id, ipFromIntToStr(newzone->zone_ip) );
        if (newuser->first_user_zone == NULL) {
            newuser->first_user_zone = newzone;
        } else {
            user_zone * p;
            for (p = newuser->first_user_zone; (p->next != NULL); p = p->next);
            p->next = newzone;
        }        
    }
    mysql_free_result(result);    
    
    // add user to list
    pthread_mutex_lock (&users_table_m);
    addUser(newuser);
    pthread_mutex_unlock (&users_table_m);
    return NULL;
}
/*
void onUserDisconnected(uint32_t user_ip) {
    pthread_mutex_lock (&users_table_m);
    // save user information
    for (user * u = firstuser; u != NULL; u = u->next) {
        if (u->user_ip == user_ip) {
            
        }
    }
        for (zone_group * p = u->first_zone_group; p != NULL; p = p->next) {
            if (p->group_changed == 1) {
                char * sql = new char[1024];
                uint64_t in_bytes = p->in_bytes;
                uint64_t out_bytes = p->out_bytes;
                uint32_t pid = p->id;
                uint32_t uid = u->id;
                float traf_in_money = p->in_mb_cost_total;
                sprintf(sql, "UPDATE statistics SET traf_in=%lu, traf_out=%lu, traf_in_money=%f WHERE (user_id=%lu) AND (zone_group_id=%lu) AND (connected=1)", in_bytes, out_bytes, traf_in_money, uid, pid);
                mysql_query(cfg.myconn, sql);
                printf("%s\n",sql);
                delete sql;
                p->group_changed = 0;
            }
        }
        if (u->debit_changed == 1) {
            u->debit_changed = 0;
            char * sql = new char[1024];
            sprintf(sql, "UPDATE users SET debit=%f WHERE id=%u", u->user_debit,u->id);
            mysql_query(cfg.myconn, sql);
            printf("%s\n",sql);
            delete sql;
        }
    }
    // remove user here
    
    pthread_mutex_unlock (&users_table_m);
}*/

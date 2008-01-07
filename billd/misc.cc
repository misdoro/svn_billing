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

void removeUser(user * current_u) {
    // free memory stored for zone_groups
    zone_group * previous_zone_group;
    for (zone_group * p = current_u->first_zone_group; p != NULL; ) {
        previous_zone_group = p;
        p = p->next;
        delete previous_zone_group;
    }
    // free memory stored for user_zones
    user_zone * previous_zone;
    for (user_zone * p = current_u->first_user_zone; p != NULL; ) {
        previous_zone = p;
        p = p->next;
        delete previous_zone;
    }
    printf("Removing user %i...", current_u->id);
    // remove user
    if (firstuser == current_u) {
        user * previous_u = firstuser;
        firstuser = firstuser->next;
        delete previous_u;
        printf("OK\n");
    } else {
        for (user * u = firstuser; u != NULL;) {
            if (u == current_u) {
                user * previous_u = u;
                u = u->next;
                if (u != NULL) {
                    previous_u = u->next;
                }
                delete u;
                printf("OK\n");
                break;
            }
        }        
    }    
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

user * onUserConnected(char * username, char * user_ip, uint32_t real_ip) {
    uint32_t u_ip = htonl(inet_addr(user_ip));
    for (user * u = firstuser; u != NULL; u = u->next) {
        if (u->user_ip == u_ip ) {
            printf("User with ip %s already connected (id=%i)\n", user_ip, u->id);
            return NULL;
        }
    }
    
    char sql[32768];
    MYSQL_RES * result;
    // get user info
    sprintf(sql, "SELECT id, debit, credit, user_ip FROM users WHERE login='%s' LIMIT 1", username);
    mysql_query(cfg.myconn, sql);
    result = mysql_store_result(cfg.myconn);
    if (mysql_num_rows(result) == 0) {
        printf ("Warning! User with name %s not found in database\n", username);
// !!! - drop user here
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (inet_addr(user_ip) != inet_addr(row[3])) {
        printf ("Warning! User ip %s for connected user '%s' is not correct. (%s in database).\n", user_ip, username, row[3]);
// !!! - drop user here
        mysql_free_result(result);
        return NULL;
    }
    // create user structure
    user * newuser = new user;
    newuser->next = NULL;
    newuser->id = atoi(row[0]);
    newuser->user_debit = atof(row[1]);
    newuser->user_credit = atof(row[2]);
    newuser->user_ip = htonl(inet_addr(row[3]));    
    newuser->first_user_zone = NULL;
    newuser->first_zone_group = NULL;
    newuser->debit_changed = 0;
    newuser->die_time = 0;
        printf("User info - id:%s, debit:%s, credit:%s\n", row[0], row[1], row[2]);
    mysql_free_result(result);
    
    // get user groups
    sprintf(sql, "SELECT group_id FROM usergroups WHERE user_id=%i LIMIT 128", newuser->id);
    mysql_query(cfg.myconn, sql);
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
        sprintf(sql, "INSERT INTO statistics (`id`, `user_id`, `real_ip`, `taken_ip`, `ip_digit`, `zone_group_id`, `traf_in`, `traf_out`, `traf_in_money`, `sess_start`, `sess_end`, `connected`)");
        sprintf(sql, "%s  VALUES (NULL, %lu, %lu, %lu, %u, %u, 0, 0, 0, CURRENT_TIMESTAMP, '0000-00-00 00:00:00', 1) ",sql, newuser->id, real_ip, newuser->user_ip, 0, newgroup->id );
        mysql_query(cfg.myconn, sql);
    }
    mysql_free_result(result);
    // load zones - create sql query
    sprintf(sql, "SELECT allzones.id, zone_groups.group_id  AS group_id, allzones.ip, allzones.mask, allzones.dstport, (SELECT mb_cost FROM groupnames WHERE group_id = groupnames.id) AS mb_cost FROM allzones INNER JOIN zone_groups ON zone_groups.zone_id = allzones.id WHERE ");
    for (zone_group * p = newuser->first_zone_group; (p!=NULL); p = p->next ) {
        sprintf(sql, "%s (zone_groups.group_id = %i)",sql, p->id);
        if (p->next != NULL) sprintf(sql,"%s OR ", sql);
    }
    sprintf(sql, "%s ORDER BY zone_groups.priority DESC",sql);
    mysql_query(cfg.myconn, sql);
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
        //printf("Loaded zone. id=%i, zone_group_id=%i, ip=%s\n", newzone->id, newzone->zone_group_id, ipFromIntToStr(newzone->zone_ip) );
        if (newuser->first_user_zone == NULL) {
            newuser->first_user_zone = newzone;
        } else {
            user_zone * p;
            for (p = newuser->first_user_zone; (p->next != NULL); p = p->next);
            p->next = newzone;
        }        
    }
    mysql_free_result(result);    
    printf("Zones loaded.\n");
    // add user to list
    pthread_mutex_lock (&users_table_m);
    addUser(newuser);
    pthread_mutex_unlock (&users_table_m);
    printf("User added.\n");
    return newuser;
}


void onUserDisconnected(uint32_t user_ip) {
    pthread_mutex_lock (&users_table_m);
    char sql[1024];
    // get user with this ip
    user * current_u = NULL;
    int user_found = 0;
    for (current_u = firstuser; current_u != NULL; current_u = current_u->next) {
        if (current_u->user_ip == user_ip) {
            user_found = 1;
            break;
        }
    }
        
    if (user_found == 0) {
        printf("Warning! User with ip %lu not found!\n", user_ip);
        return;
    }
    // store user information in database
    for (zone_group * p = current_u->first_zone_group; p != NULL; p = p->next) {
        if (p->group_changed == 1) {
            uint64_t in_bytes = p->in_bytes;
            uint64_t out_bytes = p->out_bytes;
            uint32_t pid = p->id;
            uint32_t uid = current_u->id;
            float traf_in_money = p->in_mb_cost_total;
            sprintf(sql, "UPDATE statistics SET traf_in=%llu, traf_out=%llu, traf_in_money=%f, sess_end=CURRENT_TIMESTAMP, connected=2 WHERE (user_id=%lu) AND (zone_group_id=%lu) AND (connected=1)", in_bytes, out_bytes, traf_in_money, uid, pid);
            printf("%s\n",sql);
            mysql_query(cfg.myconn, sql);
        }
    }
    sprintf(sql, "UPDATE users SET debit=%f WHERE id=%u", current_u->user_debit, current_u->id);
    mysql_query(cfg.myconn, sql);
    
    current_u->die_time = time(NULL);
    pthread_mutex_unlock (&users_table_m);
}


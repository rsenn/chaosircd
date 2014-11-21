/* chaosircd - CrowdGuard IRC daemon
 *
 * Copyright (C) 2003  Roman Senn <r.senn@nexbyte.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: userdb.c,v 1.3 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <stralloc.h>

#include "libchaos/defs.h"
#include "libchaos/io.h"
#include "libchaos/timer.h"
#include "libchaos/log.h"
#include "libchaos/mem.h"
#include "libchaos/net.h"
#include "libchaos/str.h"

#include "servauth/userdb.h"

/* -------------------------------------------------------------------------- *
 * System headers                                                             *
 * -------------------------------------------------------------------------- */

#if 0
/* -------------------------------------------------------------------------- *
 * callback which is executed when an userdb client times out                   *
 * -------------------------------------------------------------------------- */
static int
userdb_timeout(struct userdb_client *userdb) {
	/*userdb->recvbuf[0] = '\0';

	 io_shutup(userdb_fd(userdb));
	 */
	if(userdb->callback)
	userdb->callback(userdb);

	if(userdb->timer)
	{
		timer_remove(userdb->timer);
		userdb->timer = NULL;
	}

	return 0;
}

/* -------------------------------------------------------------------------- *
 * builds and sends userdb request to socket                                    *
 * -------------------------------------------------------------------------- */
static int
userdb_send(struct userdb_client *userdb) {

	//userdb->status = USERDB_ST_SENT;
	return 0;
}

/* -------------------------------------------------------------------------- *
 * parse userdb reply                                                           *
 * -------------------------------------------------------------------------- */
static int
userdb_parse(struct userdb_client *userdb) {
	/* null terminate reply */
	userdb->reply[0] = '\0';

	if(userdb->callback)
		userdb->callback(userdb);

	return 1;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void
userdb_event_rd(int fd, void *ptr) {
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void
userdb_event_cn(int fd, void *ptr) {
	struct userdb_client *userdb = ptr;

	if(0) {
		if(userdb->callback)
		userdb->callback(userdb);
	}
	else
	{
		//int64_t timeout = timer_systime - userdb->deadline;

		//if(timeout < 0LL)      timeout = 1LL;

	}
}
#endif

/* -------------------------------------------------------------------------- *
 * zero authentication client struct                                          *
 * -------------------------------------------------------------------------- */
void
userdb_zero(struct userdb_client *userdb) {
	memset(userdb, 0, sizeof(struct userdb_client));
}

/* -------------------------------------------------------------------------- *
 * close userdb client socket and zero                                          *
 * -------------------------------------------------------------------------- */
void
userdb_clear(struct userdb_client *userdb) {
	if(userdb->handle) {
		db_close(userdb->handle);
	}

	timer_push(&userdb->timer);

	userdb_zero(userdb);
}

/* -------------------------------------------------------------------------- *
 * start userdb client                                                *
 * -------------------------------------------------------------------------- */
int
userdb_connect(struct userdb_client *userdb, const char* host, const char* user, const char* password) {
	userdb->handle = db_new(DB_TYPE_MYSQL);

	return db_connect(userdb->handle, host, user, password, "chaosircd");
}

/* -------------------------------------------------------------------------- *
 * start userdb client                                                *
 * -------------------------------------------------------------------------- */
int
userdb_lookup(struct userdb_client *userdb, const char* uid) {
	userdb->result = db_query(userdb->handle, "SELECT * FROM users WHERE uid='%s' SORT BY uid;", uid);

	if(userdb->result) {

		return 0;
	}

	return -1;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void
quote_escape(stralloc* sa, const char* str, char sep, char q) {

	int quote = (sep != '\0' && strchr(str, sep));
	int escape = (q != '\0' && strchr(str, q));

	if(quote) stralloc_catb(sa, &q, 1);

	if(str) {

		int n = str_len(str);
		if(escape) {

			while(n >= 0) {
				if(*str == q) stralloc_catb(sa, "\\", 1);
				stralloc_catb(sa, str, 1);
				--n;
				++str;
			}
		} else
			stralloc_catb(sa, str, n);
	}

	if(quote) stralloc_catb(sa, &q, 1);
}
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static int
build_values(struct db* db, stralloc* sa, char** v, size_t n, char sep, char q, char stop, char start) {
	size_t i;

	stralloc_init(sa);

	for(i = 0; i < n; ++i) {
		if(!v[i]) v[i] = "";
		size_t len = str_len(v[i]);
		if(sa->len) stralloc_catb(sa, &sep, 1);
		int quote = (q != '\0') && (((sep == ',') || (!len || strchr(v[i], sep) != NULL)) || (q == '`'));
		if(quote) stralloc_catb(sa, &q, 1);

		char *str;
		int sql = (q == '\'' && sep == ',' && start == '=');

		if(sql) {
			str = malloc(len * 2 + 1);
			db_escape_string(db, str, v[i], len);
			len = str_len(str);
		} else
			str = strdup(v[i]);

		if(start) {
			while(len) {
				if(*str == start) {
					++str;
					break;
				}
				++str;
				--len;
			}
		}

		while(len) {
			if(*str == stop) break;
			if(*str == q && !sql) stralloc_catb(sa, "\\", 1);
			stralloc_catb(sa, str, 1);
			++str;
			--len;
		}

		if(quote) stralloc_catb(sa, &q, 1);
	}
	n = sa->len;
	stralloc_catb(sa, "\0", 1);
	return n;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void free_fields(char** fields) {
	size_t i;
	for(i = 0; fields[i]; ++i)
		free(fields[i]);
	free(fields);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
char**
userdb_fields(struct userdb_client *userdb, size_t* nfields, const char* exception) {

	if(nfields) *nfields = 0;

	struct db_result* result = db_query(userdb->handle, "SHOW FIELDS FROM users;");

	if(userdb->handle->error) return NULL;

	size_t n = db_num_rows(result);
	if(nfields) *nfields = n;

	if(!n) return NULL;

	char** ret = calloc(sizeof(char*), *nfields + 1);
	char **row;
	size_t i = 0;

	while((row = db_fetch_row(result))) {
		const char* f = row[0] ? row[0] : "";
		if(exception && !str_cmp(f, exception)) continue;

    size_t l = str_len(f);
    if(l > 9 && !str_cmp(f+l-9, "_location")) {
      char* tmp = malloc(l + 8 + 1);
      if(tmp) {
        strcpy(tmp, "AsText(");
        strcat(tmp, f);
        strcat(tmp, ")");
        ret[i++] = tmp;
        continue;
      }
    }
		ret[i++] = str_dup(f);
	}
	ret[i] = NULL;
	*nfields = i;

	db_free_result(result);

	return ret;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
//static int
//is_var(char* a) {
//  if(!(str_isalpha(*a) || *a == '_'))
//    return 0;
//
//  do {
//    ++a;
//    if(!(str_isalnum(*a) || *a == '_'))
//      return 0;
//  } while(*a && *a != '=');
//
//  return 1;
//}
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static char*
get_varname(char* a, size_t *n) {
	char* p;
	if((p = strchr(a, '='))) {
		*n = p - a;
		return a;
	}
	return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static char*
get_varvalue(char* a) {
	char *p, *d;
	if((p = strchr(a, '=')))
		++p;
	else
		p = a;

	d = p;

	int dequote = (*d == '\'');

	if(dequote) {
		char *e;
		d += dequote;
		char *s = p = d;
		e = p + str_len(p) - dequote;
		while(*s && s < e) {
			if(*s == '\\' && s[1] == '\'') {
				++s;
			}
			*d++ = *s++;
		}
		*d = '\0';
	}
	return p;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static char*
where_part(struct db*db, stralloc* sa, char** v, size_t num_values) {

	size_t i;
	char varname[64];
	stralloc_init(sa);

	for(i = 0; i < num_values; ++i) {
		if(sa->len) stralloc_catb(sa, " AND ", 5);

		char* s;
		size_t n;
		s = get_varname(v[i], &n);
		if(s)
			strlcpy(varname, s, n + 1);
		else
			strcpy(varname, "uid");

		stralloc_catb(sa, varname, str_len(varname));
		stralloc_catb(sa, "='", 2);
		s = db_escape_string_dup(db, get_varvalue(v[i]));
		//quote_escape(sa, get_varvalue(v[i]), ',', '\'');
		stralloc_catb(sa, s, str_len(s));
		stralloc_catb(sa, "'", 1);
	}
	stralloc_catb(sa, "\0", 1);
	return sa->s;
}
/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int
userdb_search(struct userdb_client *userdb, char** v, size_t n, char** retstr) {
	int ret = 0;
	size_t nfields;
	char** fields;
	stralloc sa, where;

	*retstr = 0;
	fields = userdb_fields(userdb, &nfields, NULL);
	build_values(userdb->handle, &sa, fields, nfields, ',', 0, 0, 0);

	/* stralloc_init(&where);
	 stralloc_copys(&where, "uid='");
	 quote_escape(&where, uid, ',', '\'');*/

	char *w = where_part(userdb->handle, &where, v, n);

	struct db_result* result = db_query(userdb->handle, "SELECT %s FROM users WHERE %s;", sa.s, w);
	stralloc_free(&sa);

	if(result == NULL) { //userdb->handle->error) {
		ret = -1;
	} else {
		char** row;
		size_t i;
		ret = db_num_rows(result);
		if((row = db_fetch_row(result))) {

			stralloc_init(&sa);
			for(i = 0; i < nfields; ++i) {
				if(sa.len) stralloc_catb(&sa, " ", 1);
				stralloc_catb(&sa, fields[i], str_len(fields[i]));
				stralloc_catb(&sa, "=", 1);

				if(row[i]) quote_escape(&sa, row[i], ' ', '\'');
				//else stralloc_catb(&sa, "''", 2);
			}
			stralloc_catb(&sa, "\0", 1);
			*retstr = sa.s;
			sa.s = 0;
		}
	}
	if(result) db_free_result(result);
	free_fields(fields);
	return ret;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int
userdb_verify(struct userdb_client *userdb, const char* uid, const char* password, char** retstr) {
	int r;
	*retstr = 0;
	userdb->result =
	        db_query(
	                userdb->handle,
	                "SELECT msisdn,imsi,imei,flag_level,last_login,last_location FROM chaosircd.users WHERE uid='%s' AND password='%s';",
	                uid, password);

	if(!userdb->result) {
		return 0;
	}
	if((r = db_num_rows(userdb->result))) {
		stralloc sa;
		char **row = db_fetch_row(userdb->result);
		build_values(userdb->handle, &sa, row, db_num_fields(userdb->result), ' ', '\'', 0, 0);
		*retstr = sa.s;
		sa.s = 0;
	}

	db_free_result(userdb->result);

	return r;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int
userdb_register(struct userdb_client *userdb, char* uid, char** v, size_t num_values) {

	stralloc fields, values;

	build_values(userdb->handle, &fields, v, num_values, ',', '`', '=', '\0');
	build_values(userdb->handle, &values, v, num_values, ',', '\'', '\0', '=');

	db_query(userdb->handle, "INSERT INTO users (uid,%s) VALUES ('%s',%s);", fields.s, uid, values.s);

	if(userdb->handle->error) return -1; //db_affected_rows(userdb->handle);

	return 0;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int
userdb_mutate(struct userdb_client *userdb, const char* uid, char** v, size_t num_values) {
	stralloc values;
  stralloc_init(&values);
	size_t i;
	for(i = 0; i < num_values; ++i) {
		char *s = (char *)v[i];
		char *eq = strchr(s, '=');
		size_t n = str_len(s);
		if(eq) {
			n = eq - s;
		}
		if(values.len) stralloc_catb(&values, ",", 1);
		stralloc_catb(&values, s, n);
		stralloc_catb(&values, "='", 2);
		s += n;
		if(*s) ++s;
		stralloc_catb(&values, s, str_len(s));
		stralloc_catb(&values, "'", 1);
	}
	stralloc_catb(&values, "\0", 1);

	db_query(userdb->handle, "UPDATE users SET %s WHERE uid='%s';", values.s,
	         db_escape_string_dup(userdb->handle, uid));

	return db_affected_rows(userdb->handle);

//  INTO users (uid,%s) VALUES ('%s',%s);", fields.s, uid, values.s);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void
userdb_set_userarg(struct userdb_client *userdb, void *arg) {
	userdb->userarg = arg;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void*
userdb_get_userarg(struct userdb_client *userdb) {
	return userdb->userarg;
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
void
userdb_set_callback(struct userdb_client *userdb, userdb_callback_t *cb, uint64_t timeout) {
	userdb->callback = cb;
	userdb->timeout = timeout;
}

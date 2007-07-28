/* chaosircd - pi-networks irc server
 *
 * Copyright (C) 2003-2005  Roman Senn <smoli@paranoya.ch>
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
 * $Id: ircd.y,v 1.2 2006/09/28 06:44:36 roman Exp $
 */

%{
 
#undef YYSTYPE

#define YY_NO_UNPUT

#include <stdio.h>
#include <stdarg.h>

#include <libchaos/defs.h>
#include <libchaos/dlink.h>
#include <libchaos/str.h>
#include <libchaos/ini.h>
#include <libchaos/log.h>
#include <libchaos/child.h>
#include <libchaos/mfile.h>
#include <libchaos/module.h>
#include <libchaos/listen.h>
#include <libchaos/connect.h>
#include <libchaos/ssl.h>

#include <chaosircd/ircd.h>
#include <chaosircd/conf.h>
#include <chaosircd/oper.h>

  
#define YY_FATAL_ERROR(x) log(conf_log, L_warning, x)

/*#include "y.tab.h"*/

int yyparse();

static struct dlog        conf_tmp_log;
static struct class       conf_tmp_class;
static struct listen      conf_tmp_listen;
static struct connect     conf_tmp_connect;
static struct child       conf_tmp_child;
static struct oper        conf_tmp_oper;
static struct ssl_context conf_tmp_ssl;
static char               conf_str_class[IRCD_CLASSLEN + 1];
static char               conf_str_password[IRCD_CLASSLEN + 1];
static char               conf_str_cipher[IRCD_CLASSLEN + 1];
static char               conf_str_key[IRCD_PATHLEN + 1];
static int                conf_int_cryptlink;
static int                conf_int_ziplink;
static char               conf_str_protocol[IRCD_PROTOLEN + 1];
static char               conf_str_name[IRCD_HOSTLEN + 1];
%}

%union {
  int   number;
  char *string;
};

%token T_ADDRESS
%token T_ADMIN
%token T_ALL
%token T_AUTH
%token T_AUTO
%token T_ARGV
%token T_BACKLOG
%token T_CERT
%token T_CHANNELS
%token T_CHILD
%token T_CIPHER
%token T_CIPHERS
%token T_CLASS
%token T_CLIENT
%token T_CLIENTS_PER_IP
%token T_COMPRESSED
%token T_CONFIG
%token T_CONNECT
%token T_CONTEXT
%token T_DEBUG
%token T_DETACH
%token T_DB
%token T_DIE
%token T_ENCRYPTED
%token T_ENFORCE
%token T_FACILITIES
%token T_FATAL
%token T_FILE
%token T_FILTER
%token T_FLOOD_TRIGGER
%token T_FLOOD_INTERVAL
%token T_FS
%token T_GLINE
%token T_GLOBAL
%token T_HUB
%token T_IMAP
%token T_IN
%token T_INFO
%token T_INIS
%token T_INTERVAL
%token T_IO
%token T_KEY
%token T_KLINE
%token T_LEVEL
%token T_LEVELS
%token T_LISTEN
%token T_LOAD
%token T_LOG
%token T_MAILDIR
%token T_MAX_CLIENTS
%token T_MEM
%token T_MFILES
%token T_MODULE
%token T_MODULES
%token T_NAME
%token T_NET
%token T_NICKCHANGES
%token T_NO
%token T_NODETACH
%token T_OPER
%token T_OUT
%token T_PASSWORD
%token T_PATH
%token T_PID
%token T_PING_FREQ
%token T_POP3
%token T_PORT
%token T_PREFIX
%token T_PROTOCOL
%token T_RECVQ
%token T_REHASH
%token T_REMOTE
%token T_SENDQ
%token T_SERVER
%token T_SOURCES
%token T_SSL
%token T_STATUS
%token T_SYSLOG
%token T_THROTTLE_TRIGGER
%token T_THROTTLE_INTERVAL
%token T_TIMEOUT
%token T_TIMER
%token T_TRUNCATE
%token T_UNGLINE
%token T_UNKLINE
%token T_VERBOSE
%token T_WARNING
%token T_YES
%token NUMBER
%token QLIST
%token QSTRING

%type <string> QSTRING
%type <number> NUMBER
 
%%
conf:
	| conf conf_item
	;

conf_item:	  global_entry
		| log_entry
		| inis_entry
		| modules_entry
		| mfiles_entry
		| child_entry
		| class_entry
		| listen_entry
		| connect_entry
		| oper_entry
                | ssl_entry
		| error ';'
		| error '}'
	;

/* -------------------------------------------------------------------------- *
 * section global                                                             *
 * -------------------------------------------------------------------------- */

global_entry:		T_GLOBAL
	{
          if(conf_current.global.name[0])
            strcpy(conf_new.global.name, conf_current.global.name);
          else
            strcpy(conf_new.global.name, "chaosircd.com");
          
          if(conf_current.global.info[0])
            strcpy(conf_new.global.info, conf_current.global.info);
          else
            strcpy(conf_new.global.info, "chaosircd");
          conf_current.global.nodetach = 1;
	}
			'{' global_items '}' ';';

global_items:		global_items global_item |
			global_item;

global_item:		global_name |
			global_info |
			global_pid |
			global_detach |
			global_hub |
			error;

/* We got the server name, free if present and then duplicate str */
global_name:		T_NAME '=' QSTRING ';'
	{
          strlcpy(conf_new.global.name, yylval.string, 
                  sizeof(conf_new.global.name));
	};
global_info:		T_INFO '=' QSTRING ';'
	{
          strlcpy(conf_new.global.info, yylval.string, 
                  sizeof(conf_new.global.info));
	};
global_pid:		T_PID '=' QSTRING ';'
	{
          strlcpy(conf_new.global.pidfile, yylval.string,
                  sizeof(conf_new.global.pidfile));
	};
/* Won't detach */
global_detach:		T_DETACH '=' T_NO ';'
	{
	  conf_new.global.nodetach = 1;
	};
/* Will detach */
global_detach:		T_DETACH '=' T_YES ';'
	{
          
/*	  conf_new.global.nodetach = 0;*/
	};
global_hub:		T_HUB '=' T_NO ';'
	{
	  conf_new.global.hub = 0;
	};
global_hub:		T_HUB '=' T_YES ';'
	{
	  conf_new.global.hub = 1;
	};

/* -------------------------------------------------------------------------- *
 * section log                                                                *
 * -------------------------------------------------------------------------- */
log_entry:		T_LOG 
	{
          log_drain_default(&conf_tmp_log);
	}
			'{' log_items '}'
        {
          struct dlog *drain;
          
          drain = log_drain_find_path(conf_tmp_log.path);
          
          if(drain == NULL)
          {
            drain = log_drain_open(conf_tmp_log.path,
                                   conf_tmp_log.sources,
                                   conf_tmp_log.level,
                                   conf_tmp_log.prefix,
                                   conf_tmp_log.truncate);
          }
          else
          {
            if(log_drain_update(drain,
                                conf_tmp_log.sources,
                                conf_tmp_log.level,
                                conf_tmp_log.prefix))
              drain = NULL;
          }
          
          if(drain)
            log_drain_pop(drain);
        }
                        ';';
log_items:		log_items log_item |
			log_item;

log_item:		log_path |
			log_sources |
			log_level |
			log_prefix |
			log_truncate |
			error;

log_path:		T_PATH '=' QSTRING ';'
	{
          strlcpy(conf_tmp_log.path, yylval.string, sizeof(conf_tmp_log.path));
	};
log_level:		T_LEVEL '=' QSTRING ';'
        {
          conf_tmp_log.level = log_level_parse(yylval.string);
        };
log_sources:		T_SOURCES '=' QSTRING ';'
	{
          conf_tmp_log.sources = log_source_parse(yylval.string);
	};
log_prefix:		T_PREFIX '=' T_YES ';'
	{
          conf_tmp_log.prefix = 1;
	};
log_prefix:		T_PREFIX '=' T_NO ';'
	{
          conf_tmp_log.prefix = 0;
	};
log_truncate:		T_TRUNCATE '=' T_YES ';'
	{
          conf_tmp_log.truncate = 1;
	};
log_truncate:		T_TRUNCATE '=' T_NO ';'
	{
          conf_tmp_log.truncate = 0;
	};

/* -------------------------------------------------------------------------- *
 * section inis                                                            *
 * -------------------------------------------------------------------------- */
inis_entry:		T_INIS
			'{' inis_items '}' ';';
inis_items:		inis_items inis_item |
			inis_item;

inis_item:		inis_load |
			error;

inis_load:		T_LOAD QSTRING ';'
	{
          struct ini  *ini;
          
          ini = ini_find_path(yylval.string);
          
          if(ini == NULL)
          {
            ini = ini_add(yylval.string);
          }
          else
          {
            if(ini_update(ini))
              ini = NULL;
          }
          
          if(ini)
            ini_pop(ini);
	};

/* -------------------------------------------------------------------------- *
 * section modules                                                            *
 * -------------------------------------------------------------------------- */
modules_entry:		T_MODULES
			'{' modules_items '}' ';';
modules_items:		modules_items modules_item |
			modules_item;

modules_item:		modules_load |
			error;

modules_load:		T_LOAD QSTRING ';'
	{
          struct module *module;
          
          module = module_find_path(yylval.string);
          
          if(module == NULL)
          {
            module = module_add(yylval.string);
          }
          else
          {
            if(module_update(module))
              module = NULL;
          }
          
          if(module)
            module_pop(module);
	};

/* -------------------------------------------------------------------------- *
 * section mfiles                                                             *
 * -------------------------------------------------------------------------- */
mfiles_entry:		T_MFILES
			'{' mfiles_items '}' ';';
mfiles_items:		mfiles_items mfiles_item |
			mfiles_item;

mfiles_item:		mfiles_load |
			error;

mfiles_load:		T_LOAD QSTRING ';'
	{
          struct mfile *mfile;
          
          mfile = mfile_find_path(yylval.string);
          
          if(mfile == NULL)
          {
            mfile = mfile_add(yylval.string);
          }
          else
          {
            if(mfile_update(mfile))
              mfile = NULL;
          }
          
          if(mfile)
            mfile_pop(mfile);
	};

/* -------------------------------------------------------------------------- *
 * section child                                                              *
 * -------------------------------------------------------------------------- */
child_entry:		T_CHILD
	{
          child_default(&conf_tmp_child);
          
          conf_str_name[0] = '\0';
	}
			'{' child_items '}'
        {
          struct child *child = NULL;
          
          if(conf_str_name[0])
            child = child_find_name(conf_str_name);

          if(child == NULL)
            child = child_find(conf_tmp_child.path);
          
          if(child == NULL)
          {
            child = child_new(conf_tmp_child.path,
                              conf_tmp_child.chans,
                              conf_tmp_child.argv,
                              conf_tmp_child.interval,
                              conf_tmp_child.autostart);
          }
          else
          {
            if(child_update(child,
                            conf_tmp_child.chans,
                            conf_tmp_child.argv,
                            conf_tmp_child.interval,
                            conf_tmp_child.autostart))
              child = NULL;
          }
          
          if(child)
          {
            child_pop(child);
            
            if(conf_str_name[0])
              child_set_name(child, conf_str_name);
          }
        }
                        ';';
child_items:		child_items child_item |
			child_item;

child_item:		child_name |
			child_path |
			child_channels |
			child_argv |
			child_interval |
			child_auto |
			error;

child_name:		T_NAME '=' QSTRING ';'
	{
          strlcpy(conf_str_name, yylval.string, 
                  sizeof(conf_str_name));
	};
child_path:		T_PATH '=' QSTRING ';'
	{
          strlcpy(conf_tmp_child.path, yylval.string, 
                  sizeof(conf_tmp_child.path));
	};
child_argv:		T_ARGV '=' QSTRING ';'
	{
          strlcpy(conf_tmp_child.argv, yylval.string, 
                  sizeof(conf_tmp_child.argv));
	};
child_auto:		T_AUTO '=' T_YES ';'
	{
          conf_tmp_child.autostart = 1;
	};
child_auto:		T_AUTO '=' T_NO ';'
	{
          conf_tmp_child.autostart = 0;
	};
child_channels:		T_CHANNELS '=' NUMBER ';'
	{
          conf_tmp_child.chans = yylval.number;
	};
child_interval:	T_INTERVAL '=' NUMBER ';'
	{
          conf_tmp_child.interval = yylval.number;
	};

/* -------------------------------------------------------------------------- *
 * section class                                                              *
 * -------------------------------------------------------------------------- */
class_entry:		T_CLASS
	{
          class_default(&conf_tmp_class);
          
          conf_str_class[0] = '\0';
	}
			'{' class_items '}'
        {
          struct class *classptr;

          classptr = class_find_name(conf_tmp_class.name);

          if(classptr == NULL)
          {
            classptr = class_add(conf_tmp_class.name,
                                 conf_tmp_class.ping_freq,
                                 conf_tmp_class.max_clients,
                                 conf_tmp_class.clients_per_ip,
                                 conf_tmp_class.recvq,
                                 conf_tmp_class.sendq,
                                 conf_tmp_class.flood_trigger,
                                 conf_tmp_class.flood_interval,
                                 conf_tmp_class.throttle_trigger,
                                 conf_tmp_class.throttle_interval);
          }
          else
          {
            if(class_update(classptr,
                            conf_tmp_class.ping_freq,
                            conf_tmp_class.max_clients,
                            conf_tmp_class.clients_per_ip,
                            conf_tmp_class.recvq,
                            conf_tmp_class.sendq,
                            conf_tmp_class.flood_trigger,
                            conf_tmp_class.flood_interval,
                            conf_tmp_class.throttle_trigger,
                            conf_tmp_class.throttle_interval))
              classptr = NULL;
          }          
          
          if(classptr)
            class_pop(classptr);
        }
                        ';';
class_items:		class_items class_item |
			class_item;

class_item:		class_name |
			class_ping_freq |
			class_max_clients |
			class_clients_per_ip |
			class_recvq |
			class_sendq |
			class_flood_trigger |
			class_flood_interval |
			class_throttle_trigger |
			class_throttle_interval |
			error;

class_name:		T_NAME '=' QSTRING ';'
	{
          strlcpy(conf_tmp_class.name, yylval.string, 
                  sizeof(conf_tmp_class.name));
	};
class_ping_freq:	T_PING_FREQ '=' NUMBER ';'
	{
          conf_tmp_class.ping_freq = yylval.number;
	};
class_max_clients:	T_MAX_CLIENTS '=' NUMBER ';'
	{
          conf_tmp_class.max_clients = yylval.number;
	};
class_clients_per_ip:	T_CLIENTS_PER_IP '=' NUMBER ';'
	{
          conf_tmp_class.clients_per_ip = yylval.number;
	};
class_recvq:		T_RECVQ '=' NUMBER ';'
	{
          conf_tmp_class.recvq = yylval.number;
	};
class_sendq:		T_SENDQ '=' NUMBER ';'
	{
          conf_tmp_class.sendq = yylval.number;
	};
class_flood_trigger:	T_FLOOD_TRIGGER '=' NUMBER ';'
	{
          conf_tmp_class.flood_trigger = yylval.number;
	};
class_flood_interval:	T_FLOOD_INTERVAL '=' NUMBER ';'
	{
          conf_tmp_class.flood_interval = yylval.number;
	};
class_throttle_trigger:	T_THROTTLE_TRIGGER '=' NUMBER ';'
	{
          conf_tmp_class.throttle_trigger = yylval.number;
	};
class_throttle_interval:T_THROTTLE_INTERVAL '=' NUMBER ';'
	{
          conf_tmp_class.throttle_interval = yylval.number;
	};

/* -------------------------------------------------------------------------- *
 * section listen                                                             *
 * -------------------------------------------------------------------------- */
listen_entry:		T_LISTEN
	{
          listen_default(&conf_tmp_listen);

          conf_str_name[0] = '\0';
          strcpy(conf_str_protocol, "noproto");
          strcpy(conf_str_class, "default");
          strcpy(conf_str_password, "default");
          strcpy(conf_tmp_listen.context, "listen");
	}
			'{' listen_items '}'
        {
          struct listen *listen;
          
          listen = listen_find(conf_tmp_listen.address, conf_tmp_listen.port);
          
          if(listen == NULL)
          {
            listen = listen_add(conf_tmp_listen.address,
                                conf_tmp_listen.port,
                                conf_tmp_listen.backlog,
                                conf_tmp_listen.ssl,
                                conf_tmp_listen.context,
                                conf_str_protocol);
          }
          else
          {
            if(listen_update(listen,
                             conf_tmp_listen.backlog,
                             conf_tmp_listen.ssl,
                             conf_tmp_listen.context,
                             conf_str_protocol))
              listen = NULL;
          }
          
          if(listen)
          {
            struct conf_listen conf_args_listen;
            
            strcpy(conf_args_listen.password, conf_str_password);
            strcpy(conf_args_listen.class, conf_str_class);

            listen_pop(listen);
            
            listen_set_args(listen, &conf_args_listen, 
                            sizeof(struct conf_listen));
            
            if(conf_str_name[0])
              listen_set_name(listen, conf_str_name);
          }
        }
                        ';';
listen_items:		listen_items listen_item |
			listen_item;

listen_item:		listen_name |
			listen_address |
			listen_port |
			listen_backlog |
			listen_ssl |
			listen_context |
			listen_protocol |
			listen_class |
			error;

listen_name:		T_NAME '=' QSTRING ';'
	{
          strlcpy(conf_str_name, yylval.string, 
                  sizeof(conf_str_name));
	};
listen_address:		T_ADDRESS '=' QSTRING ';'
	{
          strlcpy(conf_tmp_listen.address, yylval.string, 
                  sizeof(conf_tmp_listen.address));
	};
listen_port:		T_PORT '=' NUMBER ';'
	{
          conf_tmp_listen.port = yylval.number;
	};
listen_protocol:	T_PROTOCOL '=' QSTRING ';'
	{
          strlcpy(conf_str_protocol, yylval.string,
                  sizeof(conf_str_protocol));
	};
listen_backlog:		T_BACKLOG '=' NUMBER ';'
	{
          conf_tmp_listen.backlog = yylval.number;
	};
listen_ssl:		T_SSL '=' T_YES ';'
	{
          conf_tmp_listen.ssl = 1;
	};
listen_ssl:		T_SSL '=' T_NO ';'
	{
          conf_tmp_listen.ssl = 0;
	};
listen_context:		T_CONTEXT '=' QSTRING ';'
	{
          strlcpy(conf_tmp_listen.context, yylval.string, 
                  sizeof(conf_tmp_listen.context));
	};
listen_class:		T_CLASS '=' QSTRING ';'
	{
          strlcpy(conf_str_class, yylval.string, sizeof(conf_str_class));
	};

/* -------------------------------------------------------------------------- *
 * section connect                                                            *
 * -------------------------------------------------------------------------- */
connect_entry:		T_CONNECT
	{
          connect_default(&conf_tmp_connect);
          
          strcpy(conf_str_name, "default");
          strcpy(conf_str_protocol, "noproto");
          strcpy(conf_str_class, "default");
          strcpy(conf_str_cipher, "AES/256");
          
          conf_str_password[0] = '\0';
          conf_str_key[0] = '\0';
          conf_int_cryptlink = 0;
          strcpy(conf_tmp_connect.context, "connect");
	}
			'{' connect_items '}'
        {
          struct connect  *connect;
          struct protocol *proto;
          
          connect = connect_find_name(conf_str_name);
          
          proto = net_find(NET_CLIENT, conf_str_protocol);
          
          if(proto == NULL)
          {
            log(conf_log, L_warning, "invalid protocol '%s'!", 
                conf_str_protocol);
          }
          
          if(connect == NULL)
          {
            connect = connect_add(conf_tmp_connect.address,
                                  conf_tmp_connect.port_remote,
                                  proto,
                                  conf_tmp_connect.timeout,
                                  conf_tmp_connect.interval,
                                  conf_tmp_connect.autoconn,
                                  conf_tmp_connect.ssl,
                                  conf_tmp_connect.context);
          }
          else
          {
            if(connect_update(connect,
                              conf_tmp_connect.address,
                              conf_tmp_connect.port_remote,
                              proto,
                              conf_tmp_connect.timeout,
                              conf_tmp_connect.interval,
                              conf_tmp_connect.autoconn,
                              conf_tmp_connect.ssl,
                              conf_tmp_connect.context))
              connect = NULL;
          }
          
          if(connect)
          {
            struct conf_connect conf_args_connect;
            
            strcpy(conf_args_connect.passwd, conf_str_password);
            strcpy(conf_args_connect.class, conf_str_class);
            strcpy(conf_args_connect.cipher, conf_str_cipher);
            strcpy(conf_args_connect.key, conf_str_key);
            conf_args_connect.cryptlink = conf_int_cryptlink;

            connect_pop(connect);
            
            connect_set_args(connect, &conf_args_connect, 
                             sizeof(struct conf_connect));
            
            if(conf_str_name[0])
              connect_set_name(connect, conf_str_name);
          }
        }
                        ';';
connect_items:		connect_items connect_item |
			connect_item;

connect_item:		connect_name |
			connect_address |
			connect_port |
			connect_ssl |
			connect_protocol |
			connect_interval |
			connect_timeout |
			connect_auto |
			connect_class |
			connect_password |
			connect_cipher |
			connect_key |
			connect_context |
			connect_encrypted |
                        connect_compressed |
			error;

connect_name:		T_NAME '=' QSTRING ';'
	{
          strlcpy(conf_str_name, yylval.string, 
                  sizeof(conf_str_name));
	};
connect_address:	T_ADDRESS '=' QSTRING ';'
	{
          strlcpy(conf_tmp_connect.address, yylval.string, 
                  sizeof(conf_tmp_connect.address));
	};
connect_port:		T_PORT '=' NUMBER ';'
	{
          conf_tmp_connect.port_remote = yylval.number;
	};
connect_protocol:	T_PROTOCOL '=' QSTRING ';'
	{
          strlcpy(conf_str_protocol, yylval.string,
                  sizeof(conf_str_protocol));
	};
connect_ssl:		T_SSL '=' T_YES ';'
	{
          conf_tmp_connect.ssl = 1;
	};
connect_ssl:		T_SSL '=' T_NO ';'
	{
          conf_tmp_connect.ssl = 0;
	};
connect_auto:		T_AUTO '=' T_YES ';'
	{
          conf_tmp_connect.autoconn = 1;
	};
connect_auto:		T_AUTO '=' T_NO ';'
	{
          conf_tmp_connect.autoconn = 0;
	};
connect_class:		T_CLASS '=' QSTRING ';'
	{
          strlcpy(conf_str_class, yylval.string, sizeof(conf_str_class));
	};
connect_interval:	T_INTERVAL '=' NUMBER ';'
	{
          conf_tmp_connect.interval = (uint64_t)yylval.number;
	};
connect_timeout:	T_TIMEOUT '=' NUMBER ';'
	{
          conf_tmp_connect.timeout = (uint64_t)yylval.number;
	};
connect_password:	T_PASSWORD '=' QSTRING ';'
	{
          strlcpy(conf_str_password, yylval.string, sizeof(conf_str_password));
	};
connect_cipher:		T_CIPHER '=' QSTRING ';'
	{
          strlcpy(conf_str_cipher, yylval.string, sizeof(conf_str_cipher));
	};
connect_key:		T_KEY '=' QSTRING ';'
	{
          strlcpy(conf_str_key, yylval.string, sizeof(conf_str_key));
	};
connect_context:	T_CONTEXT '=' QSTRING ';'
	{
          strlcpy(conf_tmp_connect.context, yylval.string, sizeof(conf_tmp_connect.context));
	};
connect_encrypted:	T_ENCRYPTED '=' T_YES ';'
	{
          conf_int_cryptlink = 1;
	};
connect_encrypted:	T_ENCRYPTED '=' T_NO ';'
	{
          conf_int_cryptlink = 0;
	};
connect_compressed:	T_COMPRESSED '=' T_YES ';'
	{
          conf_int_ziplink = 1;
	};
connect_compressed:	T_COMPRESSED '=' T_NO ';'
	{
          conf_int_ziplink = 0;
	};

/* -------------------------------------------------------------------------- *
 * section oper                                                               *
 * -------------------------------------------------------------------------- */
oper_entry:		T_OPER
	{
          oper_default(&conf_tmp_oper);
          
          conf_str_class[0] = '\0';
	}
			'{' oper_items '}'
        {
          struct oper  *optr = NULL;
          
          conf_tmp_oper.clptr = class_find_name(conf_str_class);

          if(conf_tmp_oper.clptr == NULL)
          {
            log(conf_log, L_warning, "Invalid class %s in oper{} block %s",
                conf_str_class, conf_tmp_oper.name);
          }
          else
          {
            if(conf_tmp_oper.name[0])
              optr = oper_find(conf_tmp_oper.name);
            
            if(optr == NULL)
            {
              optr = oper_add(conf_tmp_oper.name,
                              conf_tmp_oper.passwd,
                              conf_tmp_oper.clptr,
                              conf_tmp_oper.level,
                              conf_tmp_oper.sources,
                              conf_tmp_oper.flags);
            }
            else
            {
              if(oper_update(optr,
                             conf_tmp_oper.passwd,
                             conf_tmp_oper.clptr,
                             conf_tmp_oper.level,
                             conf_tmp_oper.sources,
                             conf_tmp_oper.flags))
                optr = NULL;
            }
            
            if(optr)
              oper_pop(optr);
          }
        }
                        ';';
oper_items:		oper_items oper_item |
			oper_item;

oper_item:		oper_name |
			oper_passwd |
			oper_class |
			oper_sources |
			oper_level |
			oper_remote |
			oper_kline |
			oper_gline |
			oper_unkline |
			oper_ungline |
			oper_nickchanges |
			oper_rehash |
			oper_die |
			oper_admin |
			oper_enforce |
			error;
oper_name:		T_NAME '=' QSTRING ';'
	{
          strlcpy(conf_tmp_oper.name, yylval.string, 
                  sizeof(conf_tmp_oper.name));
	};
oper_class:		T_CLASS '=' QSTRING ';'
	{
          strlcpy(conf_str_class, yylval.string,
                  sizeof(conf_str_class));
	};
oper_passwd:		T_PASSWORD '=' QSTRING ';'
	{
          strlcpy(conf_tmp_oper.passwd, yylval.string,
                  sizeof(conf_tmp_oper.passwd));
	};
oper_level:		T_LEVEL '=' QSTRING ';'
        {
          conf_tmp_oper.level = log_level_parse(yylval.string);
        };
oper_sources:		T_SOURCES '=' QSTRING ';'
	{
          conf_tmp_oper.sources = log_source_parse(yylval.string);
	};
oper_remote:		T_REMOTE '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_REMOTE;
        };
oper_remote:		T_REMOTE '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_REMOTE;
        };
oper_kline:		T_KLINE '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_KLINE;
        };
oper_kline:		T_KLINE '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_KLINE;
        };
oper_gline:		T_GLINE '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_GLINE;
        };
oper_gline:		T_GLINE '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_GLINE;
        };
oper_unkline:		T_UNKLINE '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_UNKLINE;
        };
oper_unkline:		T_UNKLINE '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_UNKLINE;
        };
oper_ungline:		T_UNGLINE '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_UNGLINE;
        };
oper_ungline:		T_UNGLINE '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_UNGLINE;
        };
oper_nickchanges:	T_NICKCHANGES '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_NICKCHG;
        };
oper_nickchanges:	T_NICKCHANGES '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_NICKCHG;
        };
oper_rehash:		T_REHASH '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_REHASH;
        };
oper_rehash:		T_REHASH '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_REHASH;
        };
oper_die:		T_DIE '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_DIE;
        };
oper_die:		T_DIE '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_DIE;
        };
oper_admin:		T_ADMIN '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_ADMIN;
        };
oper_admin:		T_ADMIN '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_ADMIN;
        };
oper_enforce:		T_ENFORCE '=' T_YES ';'
	{
          conf_tmp_oper.flags |= OPER_FLAG_ENFORCE;
        };
oper_enforce:		T_ENFORCE '=' T_NO ';'
	{
          conf_tmp_oper.flags &= ~OPER_FLAG_ENFORCE;
        };

/* -------------------------------------------------------------------------- *
 * section ssl                                                                *
 * -------------------------------------------------------------------------- */
ssl_entry:		T_SSL
	{
          ssl_default(&conf_tmp_ssl);
	}
			'{' ssl_items '}'
        {
          struct ssl_context *scptr = NULL;
          
          if(conf_tmp_ssl.name[0])
            scptr = ssl_find_name(conf_tmp_ssl.name);
            
          if(scptr == NULL)
          {
            scptr = ssl_add(conf_tmp_ssl.name,
                            conf_tmp_ssl.context,
                            conf_tmp_ssl.cert,
                            conf_tmp_ssl.key,
                            conf_tmp_ssl.ciphers);
          }
          else
          {
            if(ssl_update(scptr,
                          conf_tmp_ssl.name,
                          conf_tmp_ssl.context,
                          conf_tmp_ssl.cert,
                          conf_tmp_ssl.key,
                          conf_tmp_ssl.ciphers))
              scptr = NULL;
          }
          
          if(scptr)
            ssl_pop(scptr);
        }
                        ';';
ssl_items:		ssl_items ssl_item |
			ssl_item;

ssl_item:		ssl_name |
			ssl_context |
			ssl_cert |
			ssl_key |
			ssl_ciphers |
			error;
ssl_name:		T_NAME '=' QSTRING ';'
	{
          strlcpy(conf_tmp_ssl.name, yylval.string, 
                  sizeof(conf_tmp_ssl.name));
	};
ssl_context:		T_CONTEXT '=' T_SERVER ';'
	{
          conf_tmp_ssl.context = SSL_CONTEXT_SERVER;
	};
ssl_context:		T_CONTEXT '=' T_CLIENT ';'
	{
          conf_tmp_ssl.context = SSL_CONTEXT_CLIENT;
	};
ssl_cert:		T_CERT '=' QSTRING ';'
	{
          strlcpy(conf_tmp_ssl.cert, yylval.string, 
                  sizeof(conf_tmp_ssl.cert));
	};
ssl_key:		T_KEY '=' QSTRING ';'
	{
          strlcpy(conf_tmp_ssl.key, yylval.string, 
                  sizeof(conf_tmp_ssl.key));
	};
ssl_ciphers:		T_CIPHERS '=' QSTRING ';'
	{
          strlcpy(conf_tmp_ssl.ciphers, yylval.string, 
                  sizeof(conf_tmp_ssl.ciphers));
	};

/* ircd-pi - pi-networks irc server
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
 * $Id: ircd.l,v 1.1.1.1 2006/09/27 10:08:58 roman Exp $
 */

%option nounput

%{
#if 0 //def __i386__
#undef errno
#endif

#include <fcntl.h>
#include <stdarg.h>

#include "y.tab.h"

#include "../config.h"

#include <libchaos/dlink.h>
#include <libchaos/str.h>
#include <libchaos/syscall.h>
#include <libchaos/log.h>
  
#include <chaosircd/ircd.h>
#include <chaosircd/conf.h>
  
#undef isatty
#undef YY_INPUT

#define exit syscall_exit
  
/* Debug stuff */
#define YYDEBUG 0
#define YYERROR_VERBOSE 0
#define YY_NEVER_INTERACTIVE 1
#define YY_MALLOC_DECL
/*#define YY_FATAL_ERROR(x) log(conf_log, L_warning, x)*/
#undef ECHO
#define ECHO ;

/* Defines the function to get input */
#define YY_INPUT(buf, res, size) \
    res = io_gets(conf_fd, buf, size);
  

#define MAX_INCLUDE_DEPTH 8

int  lineno = 0;
char linebuf[IRCD_BUFSIZE];
char conffile[PATHLEN];
  
static int             include_sp = 0;
static YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
static char            conffile_stack[MAX_INCLUDE_DEPTH][PATHLEN];
static int             lineno_stack[MAX_INCLUDE_DEPTH];
static int             file_stack[MAX_INCLUDE_DEPTH];
static void            include(void);
static int             ieof(void);
%}

digit    [0-9]
alpha    [A-Za-z]
alnum    [A-Za-z0-9]
ws       [, \t]
delim    [\+\,\|\&]  
word     {alpha}{alnum}+
number   [0-9]+{alpha}*
fname    [A-Za-z0-9\_\.\-]+
path     (\/{fname})+
string   [\"][^\"\n]*[\"\n]
list     \[.*\]
include  \.include{ws}\".*\"
comment  \#.*\n
    
%%
  
 /* Silently skip any comments and increment line count */  
{comment} { lineno++; }

 /* Same with whitespace */
{ws} ;

\n.* { 
  strlcpy(linebuf, &yytext[1], sizeof(linebuf)); 
  lineno++; 
  yyless(1); 
}
  

{include}       { include(); }

{number}        { yylval.number = str_toi(yytext); return NUMBER; }

{list}          {
  
  int i;
  
  yylval.string = &yytext[1];

  for(i = 0; yylval.string[i] && yylval.string[i] != ']'; i++);
  yylval.string[i] = '\0';  
  
  return QLIST;
                }

{string}        { 
  
  if(yytext[yyleng - 2] == '\\')
  {
    yyless(yyleng - 1);
    yymore();
  }
  else
  {
    yylval.string = &yytext[1];
    
    if(yylval.string[yyleng - 2] != '"' &&
       yylval.string[yyleng - 2] != ']')
    {
      log(conf_log, L_warning, 
          "Unterminated character string");
    }
    else
    {
      int i, j;
      
      yylval.string[yyleng - 2] = '\0';
      
      for(j = i = 0; yylval.string[i]; i++, j++)
      {
        if(yylval.string[i] != '\\')
        {
          yylval.string[j] = yylval.string[i];
        }
        else
        {
          i++;
          
          if(yylval.string[i] == '\0')
          {
            log(conf_log, L_warning, 
                "Unterminated character string");
            break;
          }
          
          yylval.string[j] = yylval.string[i];
        }
      }
      
      yylval.string[j] = '\0';
      return QSTRING;
    }
  }  
}

address           { return T_ADDRESS; }
admin             { return T_ADMIN; }
all               { return T_ALL; }
auth              { return T_AUTH; }
auto              { return T_AUTO; }
argv              { return T_ARGV; }
backlog           { return T_BACKLOG; }
cert              { return T_CERT; }
channels          { return T_CHANNELS; }
child             { return T_CHILD; }
cipher            { return T_CIPHER; }
ciphers           { return T_CIPHERS; }
class             { return T_CLASS; }
client            { return T_CLIENT; }  
clients_per_ip    { return T_CLIENTS_PER_IP; }
compressed        { return T_COMPRESSED; }  
config            { return T_CONFIG; }
connect           { return T_CONNECT; }
context           { return T_CONTEXT; }
db                { return T_DB; }
debug             { return T_DEBUG; }
detach            { return T_DETACH; }
die               { return T_DIE; }
encrypted         { return T_ENCRYPTED; }
enforce           { return T_ENFORCE; }
facilities        { return T_FACILITIES; }
fatal             { return T_FATAL; }
file              { return T_FILE; }
filter            { return T_FILTER; }
flood_trigger     { return T_FLOOD_TRIGGER; }
flood_interval    { return T_FLOOD_INTERVAL; }
fs                { return T_FS; }
gline             { return T_GLINE; }
global            { return T_GLOBAL; }
hub               { return T_HUB; }
imap              { return T_IMAP; }
in                { return T_IN; }
inis              { return T_INIS; }
info              { return T_INFO; }
interval          { return T_INTERVAL; }  
io                { return T_IO; }
key               { return T_KEY; }
kline             { return T_KLINE; }
level             { return T_LEVEL; }
levels            { return T_LEVELS; }
load              { return T_LOAD; }
listen            { return T_LISTEN; }
log               { return T_LOG; }
max_clients       { return T_MAX_CLIENTS; }
mem               { return T_MEM; }
mfiles            { return T_MFILES; }
module            { return T_MODULE; }
modules           { return T_MODULES; }
name              { return T_NAME; }
net               { return T_NET; }
nickchanges       { return T_NICKCHANGES; }
no                { return T_NO; }
oper              { return T_OPER; }
out               { return T_OUT; }
password          { return T_PASSWORD; }
path              { return T_PATH; }
pid               { return T_PID; }  
ping_freq         { return T_PING_FREQ; }  
pop3              { return T_POP3; }
port              { return T_PORT; }
prefix            { return T_PREFIX; }
protocol          { return T_PROTOCOL; }
recvq             { return T_RECVQ; }
rehash            { return T_REHASH; }
remote            { return T_REMOTE; }
sendq             { return T_SENDQ; }  
server            { return T_SERVER; }
sources           { return T_SOURCES; }  
ssl               { return T_SSL; }
status            { return T_STATUS; }
syslog            { return T_SYSLOG; }
throttle_trigger  { return T_THROTTLE_TRIGGER; }
throttle_interval { return T_THROTTLE_INTERVAL; }
timeout           { return T_TIMEOUT; }
timer             { return T_TIMER; }
truncate          { return T_TRUNCATE; }
ungline           { return T_UNGLINE; }
unkline           { return T_UNKLINE; }
verbose           { return T_VERBOSE; }
warning           { return T_WARNING; }
yes               { return T_YES; }
number            { return NUMBER; }
qstring           { return QSTRING; }
.                 { return yytext[0]; }
<<EOF>>           { if(ieof()) yyterminate(); }
%%
  
#define INCLUDE "#include"
  
static void include(void)
{
  char *c;
  
  *strchr((c = strchr(yytext, '"') + 1), '"') = '\0';
  
  if(include_sp >= MAX_INCLUDE_DEPTH)
  {
    log(conf_log, L_warning, "Includes nested too deep in %s", c);
  }
  else
  {
    int tmp_file;
    
    /* cheating */
    tmp_file = io_open(c, IO_OPEN_READ);
    
    if(tmp_file < 0)
    {
      log(conf_log, L_warning, "Cannot include %s", c);
      return;
    }
    
    io_queue_control(tmp_file, ON, OFF, ON);
    io_list[tmp_file].status.events = IO_READ;
    
    while(io_queued_read(tmp_file) > 0);
    
    io_shutup(tmp_file);
    
    lineno_stack[include_sp] = lineno;
    lineno = 1;
    file_stack[include_sp] = conf_fd;
    strlcpy(conffile_stack[include_sp], conffile, PATHLEN);
    include_stack[include_sp++] = YY_CURRENT_BUFFER;
    conf_fd = tmp_file;
    strlcpy(conffile, c, sizeof(conffile));
    yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));    
  }
}

static int ieof(void)
{
  if(include_sp)
    io_close(conf_fd);
  
  if(--include_sp < 0)
  {
    include_sp = 0;
    lineno = 1;
    return 1;
  }
  
  yy_delete_buffer(YY_CURRENT_BUFFER);
  
  lineno = lineno_stack[include_sp];
  conf_fd = file_stack[include_sp];
  strlcpy(conffile, conffile_stack[include_sp], PATHLEN);
  
  yy_switch_to_buffer(include_stack[include_sp]);
  
  return 0;
}

int yywrap(void)
{
  return 1;
}

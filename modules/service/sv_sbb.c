/* chaosircd - pi-networks irc server
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
 * $Id: sv_sbb.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/log.h>
#include <libchaos/timer.h>
#include <libchaos/hook.h>
#include <libchaos/str.h>
#include <libchaos/httpc.h>
#include <libchaos/htmlp.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/ircd.h>
#include <chaosircd/numeric.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/service.h>
#include <chaosircd/chanuser.h>
#include <chaosircd/user.h>
#include <chaosircd/msg.h>

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
#define SV_SBB_LOCLEN      128
#define SV_SBB_TRAINLEN     32
#define SV_SBB_TRACKLEN     16
#define SV_SBB_REMARKSLEN   32
#define SV_SBB_REQUEST_SIZE 16
#define SV_SBB_RESPONSE_SIZE (SV_SBB_REQUEST_SIZE * 4)
#define SV_SBB_RECORDS_SIZE  (SV_SBB_RESPONSE_SIZE * 4)
#define SV_SBB_AGENT_NAME   "RailServ"
#define SV_SBB_AGENT_USER   "timetable"
#define SV_SBB_AGENT_HOST   "sbb.ch"
#define SV_SBB_AGENT_REAL   "SBB Timetable Agent"

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
struct sv_sbb_request
{
  struct node          node;
  struct client       *client;
  struct channel      *channel;
  char                 rsrc[IRCD_LINELEN + 1];
  char                 rdst[IRCD_LINELEN + 1];
  char                 error[IRCD_LINELEN + 1];
  uint64_t             rtime;
  uint64_t             rdate;
  struct list          responses;
};

struct sv_sbb_response
{
  struct node          node;
  struct list          records;
};

struct sv_sbb_record
{
  struct node          node;
  char                 origin[SV_SBB_LOCLEN];
  char                 destination[SV_SBB_LOCLEN];
  char                 track[SV_SBB_TRACKLEN];
  char                 train[SV_SBB_TRAINLEN];
  char                 remarks[SV_SBB_REMARKSLEN];
  uint64_t             date;
  uint64_t             departure;
  uint64_t             arrival;
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_handle_invite(struct lclient        *lcptr, struct client *cptr,
                                 struct channel        *chptr, const char     *msg);
static void sv_sbb_handle_msg   (struct lclient        *lcptr, struct client *cptr,
                                 struct channel        *chptr, const char     *msg);
static void sv_sbb_remove       (struct lclient        *lcptr, struct client *cptr);
static void sv_sbb_query        (struct sv_sbb_request *rqst);
static void sv_sbb_cancel       (struct sv_sbb_request *query);
static void sv_sbb_send_error   (struct sv_sbb_request *query);
static void sv_sbb_send_response(struct sv_sbb_request *query);
static void mo_sbb              (struct lclient        *lcptr, struct client *cptr,
                                 int                    argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static struct service *sv_sbb_service;
static struct sheap    sv_sbb_request_heap;
static struct sheap    sv_sbb_response_heap;
static struct sheap    sv_sbb_record_heap;
static struct list     sv_sbb_requests;
static struct httpc   *sv_sbb_httpc;
static int             sv_sbb_idle;
static struct htmlp   *sv_sbb_htmlp;

static const char     *sv_sbb_help[] =
{
  "SBB Timetable Services",
  "----------------------",
  " ",
  "Queries are sent by /msg to the Agent or a channel the Agent is in:",
  "/msg "SV_SBB_AGENT_NAME" <query>",
  "/msg #channel "SV_SBB_AGENT_NAME" <query>",
  " ",
  "Query format is:",
  "<origin> -> <destination> [time] [date]",
  " ",
  "Examples:",
  "/msg "SV_SBB_AGENT_NAME" bern -> zürich 16:20",
  "/msg #channel "SV_SBB_AGENT_NAME" basel -> olten 18:30 17.12.2004",
  NULL
};

static char           *mo_sbb_help[] =
{
  "SBB <list|clear|del> [arg]",
  "",
  "Administration frontend for the "SV_SBB_AGENT_NAME" service.",
  NULL
};

static struct msg      m_sbb_msg =
{
  "SBB", 1, 2, MFLG_OPER,
  { NULL, NULL, mo_sbb, mo_sbb },
  mo_sbb_help
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
int sv_sbb_load(void)
{
  /* Create the service and register some message handlers */
  sv_sbb_service = service_new(SV_SBB_AGENT_NAME, SV_SBB_AGENT_USER,
                               SV_SBB_AGENT_HOST, SV_SBB_AGENT_REAL);

  service_register(sv_sbb_service, "INVITE", sv_sbb_handle_invite);
  service_register(sv_sbb_service, "PRIVMSG", sv_sbb_handle_msg);

  /* Create requests for our queue and responses */
  mem_static_create(&sv_sbb_request_heap, sizeof(struct sv_sbb_request),
                    SV_SBB_REQUEST_SIZE);
  mem_static_create(&sv_sbb_response_heap, sizeof(struct sv_sbb_response),
                    SV_SBB_RESPONSE_SIZE);
  mem_static_create(&sv_sbb_record_heap, sizeof(struct sv_sbb_record),
                    SV_SBB_RECORDS_SIZE);
  mem_static_note(&sv_sbb_request_heap, "sv_sbb request heap");
  mem_static_note(&sv_sbb_response_heap, "sv_sbb response heap");
  mem_static_note(&sv_sbb_record_heap, "sv_sbb record heap");

  dlink_list_zero(&sv_sbb_requests);

  /* Create a HTML parser */
  sv_sbb_htmlp = htmlp_new("sv_sbb");

  /* Create a HTTP client */
  sv_sbb_httpc = httpc_add("http://fahrplan.sbb.ch/bin/query.exe/dn?OK");

  /* Register a message handler */
  msg_register(&m_sbb_msg);

  sv_sbb_idle = 1;

  /* Hook at client exit so we can clean requests associated with the client */
  hook_register(client_exit, HOOK_DEFAULT, sv_sbb_remove);

  return 0;
}

void sv_sbb_unload(void)
{
  hook_unregister(client_exit, HOOK_DEFAULT, sv_sbb_remove);

  msg_unregister(&m_sbb_msg);

  htmlp_delete(sv_sbb_htmlp);
  httpc_delete(sv_sbb_httpc);

  mem_static_destroy(&sv_sbb_request_heap);
  mem_static_destroy(&sv_sbb_response_heap);
  mem_static_destroy(&sv_sbb_record_heap);

  service_delete(sv_sbb_service);
}

/* -------------------------------------------------------------------------- *
 * An error event occurs when origin or/and destination are invalid           *
 * -------------------------------------------------------------------------- */
static void sv_sbb_parse_error(struct sv_sbb_request *query)
{
  struct sv_sbb_response  resp;
  struct sv_sbb_response *pptr;
  struct sv_sbb_record   *rptr;
  struct htmlp_tag       *htptr;
  struct htmlp_var       *name;
  const char             *unknown = "error";

  /* Skip to next tag */
  htmlp_tag_next(sv_sbb_htmlp);

  /* Find the attribute named 'name' */
  if((name = htmlp_var_find(sv_sbb_htmlp, "name")))
  {
    /* If the origin is invalid then its this way */
    if(!str_icmp(name->value, "REQ0JourneyStopsSK1"))
      unknown = "origin";
    /* If the destination is invalid then its this way */
    if(!str_icmp(name->value, "REQ0JourneyStopsZK1"))
      unknown = "destination";
  }

  str_snprintf(query->error, sizeof(query->error),
           "You specified an inaccurate %s, please use one of those:",
           unknown);

  /* Clear The record list */
  dlink_list_zero(&resp.records);

  /* Find the <option> tags containing possible locations */
  if((htptr = htmlp_tag_find(sv_sbb_htmlp, "option")))
  {
    do
    {
      /* Spotted closing <option> tag, continue */
      if(htptr->closing)
        continue;

      /* It's not an <option> ag, abort */
      if(str_icmp(htptr->name, "option"))
        break;

      /* Allocate a record */
      rptr = mem_static_alloc(&sv_sbb_record_heap);

      /* Decode the tag text and put it to the record */
      strlcpy(rptr->origin, htmlp_decode(htptr->text), sizeof(rptr->origin));

      /* Add every possible location to the record list */
      dlink_add_tail(&resp.records, &rptr->node, rptr);
    }
    while((htptr = htmlp_tag_next(sv_sbb_htmlp)));

    /* We got a record, now lets allocate a response */
    pptr = mem_static_alloc(&sv_sbb_response_heap);

    /* Copy prepared response in the allocated one */
    *pptr = resp;

    /* Add it to the response list */
    dlink_add_tail(&query->responses, &pptr->node, pptr);
  }

  /* Send the error reply to the client */
  sv_sbb_send_error(query);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_send_error(struct sv_sbb_request *query)
{
  struct sv_sbb_response *pptr;
  struct sv_sbb_record   *rptr;

  /* Send the header */
  if(client_is_local(query->client))
  {
    client_send(query->client, ":%N!%U@%H NOTICE %C : %s",
                sv_sbb_service->client,
                sv_sbb_service->client,
                sv_sbb_service->client,
                query->client,
                query->error);
    client_send(query->client, ":%N!%U@%H NOTICE %C : "
                "----------------------------------"
                "----------------------------------",
                sv_sbb_service->client,
                sv_sbb_service->client,
                sv_sbb_service->client,
                query->client);
  }
  else
  {
    client_send(query->client, ":%C NOTICE %C : %s",
                sv_sbb_service->client,
                query->client,
                query->error);
    client_send(query->client, ":%C NOTICE %C : "
                "----------------------------------"
                "----------------------------------",
                sv_sbb_service->client,
                query->client);
  }


  dlink_foreach(&query->responses, pptr)
  {
    dlink_foreach(&pptr->records, rptr)
    {
      if(client_is_local(query->client))
      {
        client_send(query->client,
                    ":%N!%U@%H NOTICE %C : %s",
                    sv_sbb_service->client,
                    sv_sbb_service->client,
                    sv_sbb_service->client,
                    query->client,
                    rptr->origin);
      }
      else
      {
        client_send(query->client,
                    ":%C NOTICE %C : %s",
                    sv_sbb_service->client,
                    query->client,
                    rptr->origin);
      }
    }
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_send_response(struct sv_sbb_request *query)
{
  struct sv_sbb_response *pptr;
  struct sv_sbb_record   *rptr;

  if(client_is_local(query->client))
  {
    client_send(query->client, ":%N!%U@%H NOTICE %C : "
                "================================================"
                " "SV_SBB_AGENT_NAME" response "
                "================================================ ",
                sv_sbb_service->client,
                sv_sbb_service->client,
                sv_sbb_service->client,
                query->client);
    client_send(query->client,
                ":%N!%U@%H NOTICE %C : "
                "From                           "
                "To                             "
                "Date       "
                "Depart. "
                "Arrival "
                "Train      "
                "Remarks",
                sv_sbb_service->client,
                sv_sbb_service->client,
                sv_sbb_service->client,
                query->client);
  }
  else
  {
    client_send(query->client,
                ":%C NOTICE %C : "
                "================================================"
                " "SV_SBB_AGENT_NAME" response "
                "================================================ ",
                sv_sbb_service->client,
                query->client);
    client_send(query->client,
                ":%N!%U@%H NOTICE %C : "
                "From                           "
                "To                             "
                "Date       "
                "Depart. "
                "Arrival "
                "Train      "
                "Remarks",
                sv_sbb_service->client,
                query->client);
  }

  dlink_foreach(&query->responses, pptr)
  {
    if(client_is_local(query->client))
      client_send(query->client,
                  ":%N!%U@%H NOTICE %C : "
                  "---------------------------------------------------------"
                  "---------------------------------------------------------",
                  sv_sbb_service->client,
                  sv_sbb_service->client,
                  sv_sbb_service->client,
                  query->client);
    else
      client_send(query->client,
                  ":%C NOTICE %C : "
                  "---------------------------------------------------------"
                  "---------------------------------------------------------",
                  sv_sbb_service->client,
                  query->client);

    dlink_foreach(&pptr->records, rptr)
    {
      if(client_is_local(query->client))
      {
        client_send(query->client,
                    ":%N!%U@%H NOTICE %C : %-30s %-30s %10D %t   %t   %-10s %s",
                    sv_sbb_service->client,
                    sv_sbb_service->client,
                    sv_sbb_service->client,
                    query->client,
                    rptr->origin,
                    rptr->destination,
                    &rptr->date,
                    &rptr->departure,
                    &rptr->arrival,
                    rptr->train,
                    rptr->remarks);
      }
      else
      {
        client_send(query->client,
                    ":%C NOTICE %C : %-30s %-30s %10D %t   %t   %-10s %s",
                    sv_sbb_service->client,
                    query->client,
                    rptr->origin,
                    rptr->destination,
                    &rptr->date,
                    &rptr->departure,
                    &rptr->arrival,
                    rptr->train,
                    rptr->remarks);
      }
    }
  }

  if(client_is_local(query->client))
  {
    client_send(query->client,
                ":%N!%U@%H NOTICE %C : "
                "============================================"
                " End of "SV_SBB_AGENT_NAME" response "
                "============================================ ",
                sv_sbb_service->client,
                sv_sbb_service->client,
                sv_sbb_service->client,
                query->client);
  }
  else
  {
    client_send(query->client, ":%C NOTICE %C : "
                "============================================"
                " End of "SV_SBB_AGENT_NAME" response "
                "============================================ ",
                sv_sbb_service->client,
                query->client);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_parse_response(struct sv_sbb_request *query)
{
  struct htmlp_tag       *htptr;
  struct htmlp_var       *hvptr;
  char                   *product = NULL;
  struct sv_sbb_record    rec;
  struct sv_sbb_response  resp;
  struct sv_sbb_record   *rptr;
  struct sv_sbb_response *pptr;

  dlink_list_zero(&resp.records);
  dlink_list_zero(&query->responses);

  if((htptr = htmlp_tag_find(sv_sbb_htmlp, "table")) == NULL)
    return;

  do
  {
    if((hvptr = htmlp_var_find(sv_sbb_htmlp, "summary")) == NULL)
    {
      htmlp_tag_next(sv_sbb_htmlp);
      continue;
    }

    /* check connection */
    if(str_nicmp(hvptr->value, "Details", 7))
    {
      htmlp_tag_next(sv_sbb_htmlp);
      continue;
    }

    nextrec:
    /* get the station */
    if((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")) == NULL)
      continue;

    do
    {
      if((hvptr = htmlp_var_find(sv_sbb_htmlp, "headers")) == NULL)
      {
        htmlp_tag_next(sv_sbb_htmlp);
        continue;
      }

      if(!str_icmp(hvptr->value, "stops"))
        break;

      htmlp_tag_next(sv_sbb_htmlp);
    }
    while((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")));

    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;

    while(htptr->text[0] == '\0' || htptr->text[0] == '&')
    {
      if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
        return;
    }

    strlcpy(rec.origin, htmlp_decode(htptr->text), sizeof(rec.origin));

    /* get the date */
    if((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")) == NULL)
      continue;

    do
    {
      if((hvptr = htmlp_var_find(sv_sbb_htmlp, "align")) == NULL)
      {
        htmlp_tag_next(sv_sbb_htmlp);
        continue;
      }

      if(!str_icmp(hvptr->value, "left"))
        break;

      htmlp_tag_next(sv_sbb_htmlp);
    }
    while((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")));

    rec.date = timer_parse_date(htmlp_decode(htptr->text));

    htmlp_tag_next(sv_sbb_htmlp);

    /* get the time */
    if((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")) == NULL)
      continue;

    do
    {
      if((hvptr = htmlp_var_find(sv_sbb_htmlp, "align")) == NULL)
      {
        htmlp_tag_next(sv_sbb_htmlp);
        continue;
      }

      if(!str_icmp(hvptr->value, "left"))
        break;

      htmlp_tag_next(sv_sbb_htmlp);
    }
    while((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")));

    htptr = htmlp_tag_next(sv_sbb_htmlp);
    htptr = htmlp_tag_next(sv_sbb_htmlp);

    rec.departure = timer_parse_time(htmlp_decode(htptr->text));

    /* get product */
    if((htptr = htmlp_tag_find(sv_sbb_htmlp, "table")) == NULL)
      break;

    while((htptr = htmlp_tag_next(sv_sbb_htmlp)))
    {
      if(!str_icmp(htptr->name, "tr") && htptr->closing)
        break;

      if(htptr->text[0] && str_len(htptr->text) > 1)
        product = htptr->text;
    }

    if(product)
      strlcpy(rec.train, htmlp_decode(product), sizeof(rec.train));
    else
      rec.train[0] = '\0';

    product = NULL;

    /* get remarks */
    if((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")) == NULL)
      continue;

    do
    {
      if((hvptr = htmlp_var_find(sv_sbb_htmlp, "valign")) == NULL)
      {
        htmlp_tag_next(sv_sbb_htmlp);
        continue;
      }

      if(!str_icmp(hvptr->value, "top"))
        break;

      htmlp_tag_next(sv_sbb_htmlp);
    }
    while((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")));

    strlcpy(rec.remarks, htmlp_decode(htptr->text), sizeof(rec.remarks));

    /* get destination */
    if((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")) == NULL)
      continue;

    do
    {
      if((hvptr = htmlp_var_find(sv_sbb_htmlp, "headers")) == NULL)
      {
        htmlp_tag_next(sv_sbb_htmlp);
        continue;
      }

      if(!str_icmp(hvptr->value, "stops"))
        break;

      htmlp_tag_next(sv_sbb_htmlp);
    }
    while((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")));

    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;
    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;
    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;
    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;

    strlcpy(rec.destination, htmlp_decode(htptr->text), sizeof(rec.destination));

    /* get the time */
    if((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")) == NULL)
      continue;

    do
    {
      if((hvptr = htmlp_var_find(sv_sbb_htmlp, "align")) == NULL)
      {
        htmlp_tag_next(sv_sbb_htmlp);
        continue;
      }

      if(!str_icmp(hvptr->value, "left"))
        break;

      htmlp_tag_next(sv_sbb_htmlp);
    }
    while((htptr = htmlp_tag_find(sv_sbb_htmlp, "td")));

    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;
    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;

    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;
    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;

    rec.arrival = timer_parse_time(htmlp_decode(htptr->text));

    /* check if we have another record */
    if((htptr = htmlp_tag_find(sv_sbb_htmlp, "table")) == NULL)
      continue;

    do
    {
      if(htptr->closing)
        break;

      htmlp_tag_next(sv_sbb_htmlp);
    }
    while((htptr = htmlp_tag_find(sv_sbb_htmlp, "table")));

    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;
    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;
    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;
    if((htptr = htmlp_tag_next(sv_sbb_htmlp)) == NULL)
      break;

    /* add result record */
    rptr = mem_static_alloc(&sv_sbb_record_heap);

    memcpy(rptr, &rec, sizeof(struct sv_sbb_record));

    dlink_add_tail(&resp.records, &rptr->node, rptr);

    str_trim(rptr->origin);
    str_trim(rptr->destination);
    str_trim(rptr->train);
    str_trim(rptr->remarks);

    if(!str_icmp(htptr->name, "td") && htmlp_var_find(sv_sbb_htmlp, "headers"))
      goto nextrec;

    /* add response */
    pptr = mem_static_alloc(&sv_sbb_response_heap);

    memcpy(pptr, &resp, sizeof(struct sv_sbb_response));

    dlink_add_tail(&query->responses, &pptr->node, pptr);

    dlink_list_zero(&resp.records);

    htmlp_tag_next(sv_sbb_htmlp);
  }
  while((htptr = htmlp_tag_find(sv_sbb_htmlp, "table")));
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_parse(void)
{
  struct htmlp_tag *htptr;
  struct htmlp_var *hvptr;

  /* check for error */
  htmlp_tag_first(sv_sbb_htmlp);

  while((htptr = htmlp_tag_find(sv_sbb_htmlp, "img")))
  {
    if((hvptr = htmlp_var_find(sv_sbb_htmlp, "src")))
    {
      if(!str_icmp(hvptr->value, "/img/vs_sbb/error_arrow.gif"))
      {
        sv_sbb_parse_error(sv_sbb_requests.head ? sv_sbb_requests.head->data : NULL);
        return;
      }
    }

    htmlp_tag_next(sv_sbb_htmlp);
  }

  /* check for response */
  htmlp_tag_first(sv_sbb_htmlp);

  while((htptr = htmlp_tag_find(sv_sbb_htmlp, "form")))
  {
    if((hvptr = htmlp_var_find(sv_sbb_htmlp, "name")))
    {
      if(!str_icmp(hvptr->value, "tp_results_form"))
      {
        sv_sbb_parse_response(sv_sbb_requests.head ? sv_sbb_requests.head->data : NULL);
        sv_sbb_send_response(sv_sbb_requests.head ? sv_sbb_requests.head->data : NULL);
        return;
      }
    }

    htmlp_tag_next(sv_sbb_htmlp);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_request_delete(struct sv_sbb_request *query)
{
  struct sv_sbb_response *resp;
  struct sv_sbb_record   *rec;
  struct node            *next1;
  struct node            *next2;

  log(service_log, L_status, "deleting request for client %s", query->client->name);

  dlink_foreach_safe(&query->responses, resp, next1)
  {
    dlink_foreach_safe(&resp->records, rec, next2)
    {
      dlink_delete(&resp->records, &rec->node);
      mem_static_free(&sv_sbb_record_heap, rec);
    }

    dlink_delete(&query->responses, &resp->node);
    mem_static_free(&sv_sbb_response_heap, resp);
  }

  dlink_delete(&sv_sbb_requests, &query->node);
  mem_static_free(&sv_sbb_request_heap, query);

  if(!sv_sbb_requests.size)
  {
    sv_sbb_idle = 1;
  }
  else
  {
    sv_sbb_query(sv_sbb_requests.head->data);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_cancel(struct sv_sbb_request *query)
{
  httpc_clear(sv_sbb_httpc);
  htmlp_clear(sv_sbb_htmlp);

  sv_sbb_request_delete(query);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_remove(struct lclient *lcptr, struct client *cptr)
{
  struct sv_sbb_request *query;
  struct node           *next;

  dlink_foreach_safe(&sv_sbb_requests, query, next)
  {
    if(query->client == cptr)
    {
      if(&query->node == sv_sbb_requests.head)
      {
        sv_sbb_cancel(query);
        return;
      }

      sv_sbb_request_delete(query);
    }
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_data(struct httpc *hcptr)
{
  log(service_log, L_status, "Content-Length: %u", hcptr->data_length);

  if(htmlp_parse(sv_sbb_htmlp, hcptr->data, hcptr->data_length) == -1)
  {
    log(service_log, L_status, "Parse error in HTML");
  }
  else
  {
    log(service_log, L_status, "HTML Parsed. Got %u tags.", sv_sbb_htmlp->tags.size);

    sv_sbb_parse();
  }

  sv_sbb_cancel(sv_sbb_requests.head ? sv_sbb_requests.head->data : NULL);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_query(struct sv_sbb_request *rqst)
{
  httpc_var_set(sv_sbb_httpc, "queryPageDisplayed", "yes");
  httpc_var_set(sv_sbb_httpc, "REQ0JourneyStopsSG1", rqst->rsrc);
  httpc_var_set(sv_sbb_httpc, "REQ0JourneyStopsSID1", NULL);
  httpc_var_set(sv_sbb_httpc, "REQ0JourneyStopsSA1", "%u", 7);

  httpc_var_set(sv_sbb_httpc, "REQ0JourneyStopsZG1", rqst->rdst);
  httpc_var_set(sv_sbb_httpc, "REQ0JourneyStopsZID1", NULL);
  httpc_var_set(sv_sbb_httpc, "REQ0JourneyStopsZA1", "%u", 7);

  httpc_var_set(sv_sbb_httpc, "REQ0JourneyDate", "%D", &rqst->rdate);
  httpc_var_set(sv_sbb_httpc, "REQ0JourneyTime", "%t", &rqst->rtime);

  /* 1 = departure, 0 = arrival */
  httpc_var_set(sv_sbb_httpc, "REQ0HafasSearchForw", "%i", 1);

  httpc_var_set(sv_sbb_httpc, "REQ0HafasSkipLongChanges", "0");
  httpc_var_set(sv_sbb_httpc, "REQ0HafasMaxChangeTime", "120");

  httpc_var_set(sv_sbb_httpc, "start", "Verbindung suchen");

  httpc_var_build(sv_sbb_httpc);

  sv_sbb_httpc->type = HTTPC_TYPE_POST;

  sv_sbb_idle = 0;

  httpc_connect(sv_sbb_httpc, sv_sbb_data);
}

/* -------------------------------------------------------------------------- *
 * When the service gets invited to a channel it will join.                   *
 * -------------------------------------------------------------------------- */
static void sv_sbb_handle_invite(struct lclient *lcptr, struct client *cptr,
                                 struct channel *chptr, const char    *msg)
{
  struct chanuser *cuptr;

  /* We're already in. */
  if(channel_is_member(chptr, sv_sbb_service->client))
    return;

  /* Add to channel member list */
  cuptr = chanuser_add(chptr, sv_sbb_service->client);

  /* Send out netjoin */
  chanuser_introduce(NULL, NULL, &cuptr->gnode);

  /* Send the join command to the channel */
  chanuser_send_joins(NULL, &cuptr->gnode);
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void sv_sbb_handle_msg(struct lclient *lcptr, struct client *cptr,
                              struct channel *chptr, const char    *msg)
{
  char    *p;
  char    *timestr;
  char    *datestr;
  uint64_t q_time = timer_mtime - timer_today;
  uint64_t q_date = timer_today;

  if(!str_nicmp(msg, "help", 4))
  {
    uint32_t i;

    for(i = 0; sv_sbb_help[i]; i++)
    {
      if(client_is_local(cptr))
        client_send(cptr, ":%N!%U@%H PRIVMSG %C :%s",
                    sv_sbb_service->client,
                    sv_sbb_service->client,
                    sv_sbb_service->client,
                    cptr, sv_sbb_help[i]);
      else
        client_send(cptr, ":%C PRIVMSG %C :%s",
                    sv_sbb_service->client,
                    cptr, sv_sbb_help[i]);
    }

    return;
  }

  if((p = strstr(msg, "->")))
  {
    struct sv_sbb_request *rqst;

    *p++ = '\0';
    *p++ = '\0';

    if((timestr = str_chr(p, ':')))
    {
      while(*timestr != ' ' && timestr > p)
        timestr--;

      if(timestr == p)
      {
        timestr = NULL;
      }
      else
      {
        *timestr++ = '\0';

        if((q_time = timer_parse_time(timestr)) == (uint64_t)-1LL)
          q_time = timer_mtime - timer_today;

        if((datestr = str_chr(timestr, ' ')))
        {
          *datestr++ = '\0';

          if((q_date = timer_parse_date(datestr)) == (uint64_t)-1LL)
          {
            q_date = timer_today;
          }
        }
      }
    }

    rqst = mem_static_alloc(&sv_sbb_request_heap);

    memset(rqst, 0, sizeof(struct sv_sbb_request));

    rqst->client = cptr;
    rqst->channel = chptr;

    strlcpy(rqst->rsrc, msg, sizeof(rqst->rsrc));
    strlcpy(rqst->rdst, p, sizeof(rqst->rdst));

    str_trim(rqst->rsrc);
    str_trim(rqst->rdst);

    rqst->rtime = q_time;
    rqst->rdate = q_date;

    if(client_is_local(cptr))
      client_send(cptr, ":%N!%U@%H PRIVMSG %C :Query: %s -> %s %T %D",
                  sv_sbb_service->client,
                  sv_sbb_service->client,
                  sv_sbb_service->client,
                  cptr, rqst->rsrc, rqst->rdst, &q_time, &q_date);
    else
      client_send(cptr, ":%C PRIVMSG %C :Query: %s -> %s %T %D",
                  sv_sbb_service->client, cptr, rqst->rsrc, rqst->rdst, &q_time, &q_date);

    dlink_add_tail(&sv_sbb_requests, &rqst->node, rqst);

    if(sv_sbb_idle)
      sv_sbb_query(rqst);
  }
}

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static void mo_sbb(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  if(argc > 3)
  {
    if(argv[4])
    {
      if(server_relay_maybe(lcptr, cptr, 2, ":%C PROXY %s %s :%s", &argc, argv))
        return;
    }
    else
    {
      if(server_relay_maybe(lcptr, cptr, 1, ":%C PROXY %s :%s", &argc, argv))
        return;
    }
  }

  if(!str_icmp(argv[2], "list"))
  {
    struct sv_sbb_request *rqst;

    client_send(cptr, ":%C NOTICE %C : =================== "SV_SBB_AGENT_NAME" request list ==================== ",
                client_me, cptr);
    client_send(cptr, ":%C NOTICE %C : client     channel    from       to         date      time",
                client_me, cptr);
    client_send(cptr, ":%C NOTICE %C : --------------------------------------------------------------",
                client_me, cptr);

    dlink_foreach(&sv_sbb_requests, rqst)
    {
      client_send(cptr, ":%C NOTICE %C : %-10s %-10s %-10s %-10s %D %T",
                  client_me, cptr,
                  rqst->client->name, rqst->channel ? rqst->channel->name : "<none>",
                  rqst->rsrc, rqst->rdst, &rqst->rdate, &rqst->rtime);
    }

    client_send(cptr, ":%C NOTICE %C : ================ end of "SV_SBB_AGENT_NAME" request list ================ ",
                client_me, cptr);
  }
  else if(!str_nicmp(argv[2], "del", 3))
  {
  }
}


/* chaosircd - pi-networks irc server
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
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
 * $Id: m_info.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

/* -------------------------------------------------------------------------- *
 * Library headers                                                            *
 * -------------------------------------------------------------------------- */
#include <libchaos/defs.h>
#include <libchaos/io.h>
#include <libchaos/log.h>
#include <libchaos/module.h>

/* -------------------------------------------------------------------------- *
 * Core headers                                                               *
 * -------------------------------------------------------------------------- */
#include <chaosircd/config.h>
#include <chaosircd/msg.h>
#include <chaosircd/user.h>
#include <chaosircd/chars.h>
#include <chaosircd/client.h>
#include <chaosircd/server.h>
#include <chaosircd/lclient.h>
#include <chaosircd/numeric.h>
#include <chaosircd/channel.h>
#include <chaosircd/chanuser.h>

/* -------------------------------------------------------------------------- *
 * Prototypes                                                                 *
 * -------------------------------------------------------------------------- */
static void m_info (struct lclient *lcptr, struct client *cptr,
                    int             argc,  char         **argv);

/* -------------------------------------------------------------------------- *
 * Message entries                                                            *
 * -------------------------------------------------------------------------- */
static char *m_info_help[] = {
  "INFO [server]",
  "",
  "Displays information about the given server.",
  "If used without parameters, the information",
  "from the local server is displayed.",
  NULL
};

static struct msg m_info_msg = {
  "INFO", 0, 1, MFLG_CLIENT,
  { NULL, m_info, m_info, m_info },
  m_info_help
};

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
static const char *m_info_text[] = {
  PROJECT_NAME " v" PROJECT_VERSION " - " PROJECT_RELEASE,
  "",
  "Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>",
  "                         Manuel Kohler <maenu.kohler@bluewin.ch>",
  "",
  "This program is free software; you can redistribute it and/or modify",
  "it under the terms of the GNU General Public License as published by",
  "the Free Software Foundation; either version 2 of the License, or",
  "(at your option) any later version.",
  "",
  " Created: " CREATION,
  "Platform: " PLATFORM,
  "",
  "               O      CH---CH      "    "LSD is an abbreviation of the German chemical",
  "               \\\\    /  2    3     "  "compound, Lysergsäure-diethylamid.",
  "                C---N              "    "It was first synthesized in 1938 by the Swiss",
  "               /     \\             "   "chemist Albert Hofmann in Basel at the Sandoz",
  "           ___/       CH           "    "Laboratories as part of a large research prog-",
  "          /   \\      /  2          "   "ram dealing with ergot alkaloid derivatives.",
  "   H C---N     |  H C              "    "D-Lysergic Acid Diethylamide, commonly called",
  "    3     \\__//    3               "   "acid, LSD, or LSD-25, is a powerful semisynth-",
  "          /   \\                    "   "etic hallucinogen and psychedelic entheogen.",
  "         |     |---.               "    "A typical dose of LSD is only 100 micrograms,",
  "          \\___/  `` \\              "  "a tiny amount equal to 1/10th the weight of a",
  "          /   \\\\   //              "  "grain of sand.  LSD causes a powerful intensi-",
  "          \\   / ---'               "   "fication and alteration of feelings, memories,",
  "           `N'                     "    "senses and self-awareness for 6-12 hours.  In",
  "            |                      "    "addition, LSD usually produces visual effects",
  "            H                      "    "such as brilliant colors, geometric patterns,",
  "                                   "    "and \"trails\" behind moving objects.",
  "",
  "LSD usually does not produce hallucinations in the strict sense, but instead ill-",
  "usions and vivid daydream-like fantasies.  At higher concentrations it can cause",
  "synaesthesia. The pharmacological effects can be followed by long-lasting psycho-",
  "logical shifts such as changed views and mindset. LSD is synthesized from lyserg-",
  "ic acid and is sensitive to oxygen, ultra-violet light, and chlorine, especially",
  "in solution. In pure form it is colorless, odorless and bitter. LSD is typically",
  "delivered orally, usually on a substrate, such as absorbent blotter paper, sugar-",
  "cubes, or gelatin, although it is also possible to deliver it via food or drink.",
  "",
  "Description from wikipedia.org, the free encyclopedia.",
  NULL
};

/* -------------------------------------------------------------------------- *
 * Module hooks                                                               *
 * -------------------------------------------------------------------------- */
IRCD_MODULE(int) m_info_load(void)
{
  if(msg_register(&m_info_msg) == NULL)
    return -1;

  return 0;
}

IRCD_MODULE(void) m_info_unload(void)
{
  msg_unregister(&m_info_msg);
}

/* -------------------------------------------------------------------------- *
 * argv[0] - prefix                                                           *
 * argv[1] - 'info'                                                           *
 * -------------------------------------------------------------------------- */
static void m_info(struct lclient *lcptr, struct client *cptr,
                   int             argc,  char         **argv)
{
  uint32_t i;
  char     uptime[IRCD_LINELEN - 1];

  if(argc > 2)
  {
    if(server_relay_always(lcptr, cptr, 2, ":%C INFO :%s", &argc, argv))
      return;
  }

  for(i = 0; m_info_text[i]; i++)
    numeric_send(cptr, RPL_INFO, m_info_text[i]);

  numeric_send(cptr, RPL_INFO, "");
  str_snprintf(uptime, sizeof(uptime), "Server up since %s.", ircd_uptime());
  numeric_send(cptr, RPL_INFO, uptime);

  numeric_send(cptr, RPL_ENDOFINFO);
}

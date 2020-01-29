sources='config.h contrib/dumpcode.c contrib/shellcode.h contrib/sploit.c include/cgircd/channel.h include/cgircd/user.h include/cgircd/chanuser.h include/cgircd/crowdguard.h include/cgircd/class.h include/cgircd/config.h include/cgircd/ircd.h include/cgircd/chars.h include/cgircd/oper.h include/cgircd/msg.h include/cgircd/lclient.h include/cgircd/service.h include/cgircd/chanmode.h include/cgircd/numeric.h include/cgircd/server.h include/cgircd/usermode.h include/cgircd/conf.h include/cgircd/client.h libchaos/include/libchaos/graph.h libchaos/include/libchaos/image_defpal.h libchaos/include/libchaos/timer.h libchaos/include/libchaos/cfg.h libchaos/include/libchaos/htmlp.h libchaos/include/libchaos/listen.h libchaos/include/libchaos/filter.h libchaos/include/libchaos/ini.h libchaos/include/libchaos/ttf.h libchaos/include/libchaos/syscall.h libchaos/include/libchaos/connect.h libchaos/include/libchaos/queue.h libchaos/include/libchaos/config.h libchaos/include/libchaos/mem.h libchaos/include/libchaos/db.h libchaos/include/libchaos/module.h libchaos/include/libchaos/str.h libchaos/include/libchaos/sauth.h libchaos/include/libchaos/font_6x10.h libchaos/include/libchaos/defs.h libchaos/include/libchaos/gif.h libchaos/include/libchaos/dlink.h libchaos/include/libchaos/net.h libchaos/include/libchaos/hook.h libchaos/include/libchaos/font_8x13b.h libchaos/include/libchaos/log.h libchaos/include/libchaos/httpc.h libchaos/include/libchaos/template.h libchaos/include/libchaos/mfile.h libchaos/include/libchaos/child.h libchaos/include/libchaos/image.h libchaos/include/libchaos/ssl.h libchaos/include/libchaos/font_8x13.h libchaos/include/libchaos/io.h libchaos/include/libchaos/divdi3.h libchaos/include/msinttypes/stdint.h libchaos/include/msinttypes/inttypes.h libchaos/config.h libchaos/src/divdi3.c libchaos/src/io.c libchaos/src/filter.c libchaos/src/image.c libchaos/src/module.c libchaos/src/ssl.c libchaos/src/dlfcn_darwin.c libchaos/src/mem.c libchaos/src/connect.c libchaos/src/queue.c libchaos/src/htmlp.c libchaos/src/hook.c libchaos/src/ini.c libchaos/src/dlfcn.h libchaos/src/ttf.c libchaos/src/gif.c libchaos/src/net.c libchaos/src/cfg.c libchaos/src/mfile.c libchaos/src/log.c libchaos/src/str.c libchaos/src/sauth.c libchaos/src/dlfcn_win32.c libchaos/src/syscall.c libchaos/src/listen.c libchaos/src/child.c libchaos/src/httpc.c libchaos/src/graph.c libchaos/src/db.c libchaos/src/dlink.c libchaos/src/timer.c libchaos/test/imagetest.c libchaos/test/ttftest.c libchaos/test/cfgtest.c libchaos/test/giftest.c libchaos/test/strtest.c libchaos/test/dbtest.c libchaos/test/graphtest.c libchaos/servauth/proxy.h libchaos/servauth/query.c libchaos/servauth/dns.h libchaos/servauth/cache.c libchaos/servauth/control.h libchaos/servauth/query.h libchaos/servauth/auth.c libchaos/servauth/control.c libchaos/servauth/dns.c libchaos/servauth/auth.h libchaos/servauth/cache.h libchaos/servauth/servauth.c libchaos/servauth/proxy.c libchaos/servauth/commands.h libchaos/servauth/servauth.h libchaos/servauth/commands.c modules/module_import.h modules/msg/m_error.c modules/msg/m_restart.c modules/msg/m_nbounce.c modules/msg/m_server.c modules/msg/m_kick.c modules/msg/m_ntopic.c modules/msg/m_motd.c modules/msg/m_umode.c modules/msg/m_who.c modules/msg/m_away.c modules/msg/m_nuser.c modules/msg/m_ison.c modules/msg/m_dump.c modules/msg/m_userip.c modules/msg/m_connect.c modules/msg/m_nservice.c modules/msg/m_userhost.c modules/msg/m_quit.c modules/msg/m_burst.c modules/msg/m_rehash.c modules/msg/m_njoin.c modules/msg/m_oper.c modules/msg/m_cap.c modules/msg/m_help.c modules/msg/m_capab.c modules/msg/m_pass.c modules/msg/m_invite.c modules/msg/m_module.c modules/msg/m_part.c modules/msg/m_die.c modules/msg/m_msg.c modules/msg/m_user.c modules/msg/m_nick.c modules/msg/m_info.c modules/msg/m_list.c modules/msg/m_ping.c modules/msg/m_kill.c modules/msg/m_names.c modules/msg/m_topic.c modules/msg/m_version.c modules/msg/m_ts.c modules/msg/m_uptime.c modules/msg/m_squit.c modules/msg/m_gline.c modules/msg/m_nserver.c modules/msg/m_geolocation.c modules/msg/m_opart.c modules/msg/m_nmode.c modules/msg/m_mode.c modules/msg/m_pong.c modules/msg/m_nquit.c modules/msg/m_spoof.c modules/msg/m_kline.c modules/msg/m_whois.c modules/msg/m_nclock.c modules/msg/m_lusers.c modules/msg/m_join.c modules/msg/m_links.c modules/msg/m_omode.c modules/msg/m_stats.c modules/stats/st_traffic.c modules/usermode/um_invisible.c modules/usermode/um_servnotice.c modules/usermode/um_client.c modules/usermode/um_operator.c modules/usermode/um_paranoid.c modules/usermode/um_wallops.c modules/usermode/um_crowdguard.c modules/chanmode/cm_limit.c modules/chanmode/cm_except.c modules/chanmode/cm_invex.c modules/chanmode/cm_op.c modules/chanmode/cm_deny.c modules/chanmode/cm_ahalfop.c modules/chanmode/cm_invite.c modules/chanmode/cm_alarm.c modules/chanmode/cm_aop.c modules/chanmode/cm_moderated.c modules/chanmode/cm_secret.c modules/chanmode/cm_key.c modules/chanmode/cm_voice.c modules/chanmode/cm_topic.c modules/chanmode/cm_avoice.c modules/chanmode/cm_noext.c modules/chanmode/cm_halfop.c modules/chanmode/cm_persistent.c modules/chanmode/cm_ban.c modules/service/sv_sbb.c modules/lclient/lc_clones.c modules/lclient/lc_mflood.c modules/lclient/lc_cookie.c modules/lclient/lc_sauth.c modules/lclient/lc_crowdguard.c src/chanmode.c src/y.tab.h src/numeric.c src/lclient.c src/chars.c src/ircd.c src/conf.c src/user.c src/class.c src/client.c src/server.c src/chanuser.c src/ircd.l src/msg.c src/lex.yy.c src/main.c src/usermode.c src/y.tab.c src/channel.c src/oper.c src/ircd.y src/service.c'
toksubst.sh -i CHAOS_NODE_STRUCT NODE_STRUCT $sources
toksubst.sh -i CHAOS_DEFS_H _DEFS_H $sources
toksubst.sh -i CHAOS_INLINE CHAOS_INLINE $sources
toksubst.sh -i STATIC_LIBCHAOS STATIC_LIBCHAOS $sources
toksubst.sh -i CHAOS_DATA CHAOS_DATA $sources
toksubst.sh -i CHAOS_API CHAOS_API $sources
toksubst.sh -i libchaos libchaos $sources
toksubst.sh -i BUILD_LIBCHAOS BUILD_LIBCHAOS $sources
toksubst.sh -i cgircd cgircd $sources
toksubst.sh -i CHAOS_LIST_STRUCT LIST_STRUCT $sources
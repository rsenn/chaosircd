
#
set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE FILEPATH "runtime prefix dir")
set(SYSCONFDIR ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME} CACHE FILEPATH "configuration directory")
set(PLUGINDIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME} CACHE FILEPATH "plugin directory")

add_definitions(-DPREFIX=\"${PREFIX}\")
add_definitions(-DSYSCONFDIR=\"${SYSCONFDIR}\")
add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")


set(CC "${CMAKE_C_COMPILER}")
set(CFLAGS "${CMAKE_C_FLAGS}")
set(INSTALL "")
set(LIBS "${LINK_LIBRARIES}")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(bindir "${prefix}/bin")
set(exec_prefix "${prefix}")
set(includedir "${prefix}/include")
set(inidir "${prefix}/var/lib/cgircd")
set(libexecdir "${prefix}/libexec")
set(logdir "${prefix}/var/log/cgircd")
set(pidfile "${prefix}/var/run/cgircd.pid")
set(plugindir "${prefix}/lib/cgircd")
set(sbindir "${prefix}/sbin")
set(sysconfdir "${prefix}/etc/cgircd")
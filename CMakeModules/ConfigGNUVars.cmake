#
set(PREFIX
    ${CMAKE_INSTALL_PREFIX}
    CACHE FILEPATH "runtime prefix dir")
set(SYSCONFDIR
    ${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME}
    CACHE FILEPATH "configuration directory")
set(PLUGINDIR
    ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME}
    CACHE FILEPATH "plugin directory")

add_definitions(-DPREFIX=\"${PREFIX}\")
add_definitions(-DSYSCONFDIR=\"${SYSCONFDIR}\")
add_definitions(-DPLUGINDIR=\"${PLUGINDIR}\")

set(CC "${CMAKE_C_COMPILER}")
set(CFLAGS "${CMAKE_C_FLAGS}")
set(INSTALL "")
set(LIBS "${LINK_LIBRARIES}")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "${prefix}")

# The following mirror the *DIR-style cache vars above, but lowercase (used
# by conf/*.conf.in and tools/*.in templates) and individually overridable -
# e.g. for FHS-correct packaging (debian/rules passes -Dsysconfdir=/etc/...,
# -Dinidir=/var/lib/..., -Dlogdir=/var/log/..., -Dpidfile=/var/run/...)
# instead of everything living under one runtime prefix.
set(bindir
    "${prefix}/bin"
    CACHE PATH "installed binary directory")
set(sbindir
    "${prefix}/sbin"
    CACHE PATH "installed admin binary directory")
set(includedir
    "${prefix}/include"
    CACHE PATH "installed header directory")
set(libexecdir
    "${prefix}/libexec"
    CACHE PATH "installed libexec (helper binary) directory")
set(plugindir
    "${prefix}/lib/${PROJECT_NAME}"
    CACHE PATH "installed loadable module directory")
set(sysconfdir
    "${SYSCONFDIR}"
    CACHE PATH "installed configuration directory (defaults to \${SYSCONFDIR})")
set(inidir
    "${prefix}/var/lib/${PROJECT_NAME}"
    CACHE PATH "installed runtime .ini database directory")
set(logdir
    "${prefix}/var/log/${PROJECT_NAME}"
    CACHE PATH "installed log directory")
set(pidfile
    "${prefix}/var/run/${PROJECT_NAME}.pid"
    CACHE FILEPATH "installed pid file path")

# conf/*.conf.in use these specifically (they're substituted verbatim into
# quoted paths inside the generated config, so they must never be empty -
# see the "relinidir/rellogdir default to empty and configs silently write
# to filesystem root" bug this was added to fix).
set(relinidir "${inidir}")
set(rellogdir "${logdir}")

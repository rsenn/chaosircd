# $Id: Makefile.in,v 1.4 2006/09/28 06:44:36 roman Exp $
# ===========================================================================
#
# Makefile for libowfat/src subdirectory
#
# relative directories
# ---------------------------------------------------------------------------
srcdir       = .
top_srcdir   = ..
top_builddir = ..
thisdir      = libowfat/



# include global make configuration
# ---------------------------------------------------------------------------
include $(top_builddir)/config.mk

# configuration for this directory
# ---------------------------------------------------------------------------
LIBNAME     = $(PACKAGE)
A_LIBRARY   = $(LIBNAME).$(A_EXEEXT)
#PIE_LIBRARY = $(PIE_NAME)$(PIE_VERSION_PREFIX).$(PIE_LIBEXT)$(PIE_VERSION_SUFFIX)
#PIE_LIBRARY = libowfat.so
MODULES = dns/dns_resolve dns/dns_dtda dns/dns_sortip dns/dns_transmit dns/dns_rcrw dns/dns_ipq6 dns/dns_mx dns/dns_name dns/dns_packet dns/dns_rcip dns/dns_txt dns/dns_ip6 dns/dns_dfd dns/dns_nd dns/dns_ip dns/dns_domain dns/dns_ipq dns/dns_nd6 dns/dns_random dns/dns_sortip6 byte/byte_chr byte/byte_copy test/byte_copy byte/byte_zero byte/byte_diff byte/byte_rchr byte/byte_copyr fmt/fmt_human test/fmt_human fmt/fmt_xlonglong fmt/fmt_strn fmt/fmt_pad fmt/fmt_xlong fmt/fmt_strm_internal fmt/fmt_ulong0 fmt/fmt_8long fmt/fmt_fill fmt/fmt_double fmt/fmt_httpdate test/fmt_httpdate fmt/fmt_humank fmt/fmt_ulong fmt/fmt_longlong test/fmt_longlong fmt/fmt_long test/fmt_long fmt/fmt_str fmt/fmt_tohex fmt/fmt_plusminus fmt/fmt_minus fmt/fmt_8longlong fmt/fmt_ulonglong scan/scan_8int scan/scan_uint scan/scan_nonwhitenskip scan/scan_int scan/scan_double scan/scan_xint scan/scan_8short scan/scan_longlong scan/scan_httpdate scan/scan_ulong scan/scan_xlonglong scan/scan_xshort scan/scan_8long scan/scan_plusminus scan/scan_whitenskip scan/scan_charsetnskip scan/scan_ushort scan/scan_long test/scan_long scan/scan_ulonglong scan/scan_noncharsetnskip scan/scan_fromhex scan/scan_short scan/scan_xlong str/str_rchr str/str_chr str/str_len str/str_diffn str/str_copy str/str_start str/str_diff uint/uint16_read uint/uint16_read_big uint/uint32_pack uint/uint32_pack_big uint/uint64_pack uint/uint64_pack_big uint/uint32_unpack uint/uint32_unpack_big uint/uint64_unpack uint/uint64_unpack_big uint/uint32_read uint/uint32_read_big uint/uint64_read uint/uint64_read_big uint/uint16_pack uint/uint16_pack_big uint/uint16_unpack uint/uint16_unpack_big open/readclose open/open_excl open/open_read open/open_write open/open_rw open/open_trunc open/open_append open/openreadclose stralloc/stralloc_cat stralloc/stralloc_copyb stralloc/stralloc_catm_internal stralloc/stralloc_cats stralloc/stralloc_free stralloc/stralloc_catlong0 stralloc/stralloc_chop stralloc/stralloc_diff stralloc/stralloc_catb stralloc/stralloc_ready stralloc/stralloc_init stralloc/stralloc_catulong0 stralloc/stralloc_copys stralloc/stralloc_starts stralloc/stralloc_readyplus stralloc/stralloc_append stralloc/stralloc_copy stralloc/stralloc_chomp test/stralloc_chomp stralloc/stralloc_diffs stralloc/stralloc_zero unix/ndelay_off unix/iopause unix/ndelay_on unix/winsock2errno socket/socket_recv4 socket/socket_mcleave4 socket/socket_tryreservein socket/socket_getifidx socket/socket_local4 socket/socket_v6any socket/socket_remote4 socket/socket_listen socket/fmt_ip6if socket/socket_tcp4 socket/socket_mcleave6 socket/socket_tcp4b socket/socket_getifname socket/socket_noipv6 socket/socket_mcjoin6 socket/init socket/socket_accept4 socket/scan_ip6if socket/socket_bind6 socket/socket_send4 socket/socket_mcloop6 socket/socket_broadcast socket/scan_ip4 socket/socket_connect4 socket/socket_send6 socket/socket_deferaccept socket/socket_mcjoin4 socket/scan_ip6_flat socket/socket_connected socket/fmt_ip6c socket/socket_bind6_reuse socket/socket_v4mappedprefix socket/socket_v6loopback socket/socket_connect6 socket/socket_bind4 socket/socket_mcloop4 socket/fmt_ip4 socket/socket_ip4loopback socket/socket_bind4_reuse socket/socket_tcp6b socket/fmt_ip6ifc socket/socket_udp6 socket/socket_recv6 socket/socket_accept6 socket/scan_ip6 socket/fmt_ip6 test/fmt_ip6 socket/socket_mcttl4 socket/socket_mchopcount6 socket/socket_udp4 socket/socket_local6 socket/socket_remote6 socket/socket_tcp6 socket/fmt_ip6_flat buffer/buffer_putflush buffer/buffer_putulong buffer/buffer_get_token_sa buffer/buffer_putm_internal buffer/buffer_putsa buffer/buffer_close buffer/buffer_puts buffer/buffer_getline examples/buffer_getline buffer/buffer_putsaflush buffer/buffer_0small buffer/buffer_init_free buffer/buffer_puterror buffer/buffer_putlong buffer/errmsg_write buffer/errmsg_iam buffer/errmsg_warnsys buffer/errmsg_puts buffer/buffer_get_token_pred buffer/buffer_get_new_token_sa_pred buffer/buffer_putnlflush buffer/buffer_putsflush buffer/buffer_putsalign buffer/buffer_putm_internal_flush buffer/buffer_get buffer/buffer_put8long buffer/buffer_fromsa test/buffer_fromsa buffer/buffer_1small buffer/buffer_feed buffer/buffer_peek buffer/buffer_get_new_token_sa buffer/buffer_mmapread buffer/buffer_seek buffer/buffer_init buffer/buffer_puterror2 buffer/buffer_stubborn buffer/buffer_get_token_sa_pred buffer/buffer_getnewline_sa buffer/buffer_putspace buffer/buffer_putulonglong buffer/buffer_putalign buffer/buffer_get_token buffer/buffer_putxlong buffer/buffer_stubborn2 buffer/buffer_putlonglong buffer/buffer_getc buffer/buffer_2 buffer/errmsg_info buffer/errmsg_infosys buffer/buffer_flush buffer/buffer_1 test/buffer_1 buffer/errmsg_warn buffer/buffer_0 buffer/buffer_getn buffer/buffer_put buffer/buffer_getline_sa mmap/mmap_shared mmap/mmap_private mmap/mmap_unmap mmap/mmap_read textcode/scan_to_sa textcode/scan_base64 textcode/fmt_uuencoded textcode/fmt_to_sa textcode/scan_cescape textcode/fmt_base64 textcode/scan_tofrom_array textcode/fmt_cescape textcode/fmt_tofrom_array textcode/fmt_ldapescape textcode/fmt_urlencoded textcode/scan_html textcode/fmt_to_array textcode/scan_yenc textcode/scan_quotedprintable textcode/fmt_yenc textcode/fmt_quotedprintable textcode/fmt_html textcode/base64 textcode/scan_hexdump textcode/fmt_foldwhitespace textcode/scan_urlencoded textcode/fmt_hexdump textcode/scan_to_array textcode/fmt_ldapescape2 textcode/scan_uuencoded textcode/scan_ldapescape taia/taia_half taia/taia_tai taia/taia_approx taia/taia_pack taia/taia_frac taia/taia_less taia/taia_unpack taia/taia_sub taia/taia_add taia/taia_now taia/taia_addsec taia/taia_uint tai/tai_uint tai/tai_unpack tai/tai_sub tai/tai_add tai/tai_now tai/tai_pack case/case_starts case/case_diffs case/case_diffb case/case_lowerb case/case_lowers array/array_trunc array/array_cats array/array_cat array/array_cate array/array_length array/array_truncate array/array_cat0 array/array_equal array/array_start array/array_bytes array/array_get array/array_fail array/array_catb array/array_reset array/array_allocate array/array_cats0 mult/range_str4inbuf mult/imult16 mult/umult32 mult/range_strinbuf mult/range_arrayinbuf mult/umult16 mult/imult32 mult/imult64 mult/umult64 mult/range_str2inbuf io/io_trywrite io/io_createfile io/iob_addbuf_internal io/iob_write io/iob_adds_free io/io_sendfile io/iob_addfile_close io/io_wantread io/io_socketpair io/io_readfile io/iob_addbuf io/io_fd io/iob_addbuf_free io/io_wait io/io_waituntil2 io/io_waitwrite io/io_dontwantwrite io/iob_prefetch io/iob_free io/io_passfd io/io_check io/iob_new io/io_timeouted io/io_eagain io/iob_send io/iob_reset io/io_sigpipe io/io_finishandshutdown io/iob_bytesleft io/io_receivefd io/io_dontwantread io/iob_adds io/io_waitread io/io_waituntil io/io_mmapwritefile io/io_closeonexec io/io_close io/io_tryreadtimeout io/io_canwrite io/iob_addfile io/io_pipe io/io_readwritefile io/io_getcookie io/io_timeout io/io_tryread io/io_trywritetimeout io/io_wantwrite io/iob_addbuf_munmap io/io_canread io/io_setcookie io/io_nonblock io/io_block io/io_appendfile cdb/cdb_hash cdb/cdb_make cdb/cdb_traverse cdb/cdb

#MODULES += $(PIE_LOADER)

SOURCES     = $(MODULES:%=%.c)
OBJECTS     = $(MODULES:%=%.o)
PIC_OBJECTS = $(MODULES:%=%.pio)
DEPS        = $(MODULES:%=%.d)

#SYS_SOURCES += dlfcn_win32.c
#SYS_SOURCES += dlfcn_darwin.c 

CONFIG_H_IN  = $(top_srcdir)/include/libowfat/config.h.in
CONFIG_H     = $(top_builddir)/include/libowfat/config.h
CONFIG_H_TOP = include/libowfat/config.h

#PIE_DEF = $(srcdir)/libowfat.def
#PIE_DEF = libowfat.def
#PIE_LDFLAGS += -Wl,--out-implib,lib$(PIE_NAME).a

##bin_EXEC = libowfat.dll

LIBS     += $(LIBOWFAT_LIBS)
LIBS     += $(DLFCN_LIBS)
LIBS     += $(SSL_LIBS)
@SQLITE_TRUE@CFLAGS += $(SQLITE_CFLAGS)
@SQLITE_TRUE@LIBS     += $(SQLITE_LIBS)
@MYSQL_TRUE@CFLAGS += $(MYSQL_CFLAGS)
@MYSQL_TRUE@LIBS     += $(MYSQL_LIBS)
@PSQL_TRUE@CFLAGS += $(PSQL_CFLAGS)
@PSQL_TRUE@LIBS     += $(PSQL_LIBS)
INCLUDES += $(SSL_CFLAGS)

# targets for this directory
# ---------------------------------------------------------------------------
all: Makefile $(A_LIBRARY) $(PIE_LIBRARY)

install: install-exec install-data

# dependencies
# ---------------------------------------------------------------------------
#-include $(DEPS)

# install config
# ---------------------------------------------------------------------------
lib_DATA += $(A_LIBRARY)
##bin_EXEC += $(PIE_LIBRARY)
#lib_EXEC += $(PIE_LIBRARY)

# build config
# ---------------------------------------------------------------------------
CPPFLAGS += -DPLUGINDIR=\"$(plugindir)\" -DDLLEXT=\"$(PIE_LIBEXT)\" -DBUILD_LIBOWFAT
#FILES    += $(SYS_SOURCES)
INCLUDES += -I$(thisdir) \
            -I$(top_srcdir)/include \
            -I. \
            -I$(top_builddir)/include/libowfat

CFLAGS += $(SSL_CFLAGS)

$(DLM_FALSE)$(PIE_FALSE)$(A_LIBRARY): $(OBJECTS)
$(DLM_FALSE)$(PIE_FALSE)$(A_LIBRARY): CPPFLAGS += -DSTATIC_LIBOWFAT

# include global make targets
# ---------------------------------------------------------------------------
include $(top_builddir)/build.mk


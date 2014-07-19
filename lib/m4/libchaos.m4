# stolen from Sam Lantinga 9/21/99
# which has stolen it from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_CHAOS([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libchaos, and define CHAOS_CFLAGS and CHAOS_LIBS
dnl
AC_DEFUN([AM_PATH_CHAOS],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(chaos-prefix,[  --with-chaos-prefix=PFX   Prefix where libchaos is installed (optional)],
            chaos_prefix="$withval", chaos_prefix="")
AC_ARG_WITH(chaos-exec-prefix,[  --with-chaos-exec-prefix=PFX Exec prefix where libchaos is installed (optional)],
            chaos_exec_prefix="$withval", chaos_exec_prefix="")
AC_ARG_ENABLE(chaostest, [  --disable-chaostest       Do not try to compile and run a test chaos program],
		    , enable_chaostest=yes)

  if test x$chaos_exec_prefix != x ; then
     chaos_args="$chaos_args --exec-prefix=$chaos_exec_prefix"
     if test x${chaos_CONFIG+set} != xset ; then
        chaos_CONFIG=$chaos_exec_prefix/bin/libchaos-config
     fi
  fi
  if test x$chaos_prefix != x ; then
     chaos_args="$chaos_args --prefix=$chaos_prefix"
     if test x${chaos_CONFIG+set} != xset ; then
        chaos_CONFIG=$chaos_prefix/bin/libchaos-config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  AC_PATH_PROG(chaos_CONFIG, libchaos-config, no, [$PATH])
  min_chaos_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for libchaos - version >= $min_chaos_version)
  no_chaos=""
  if test "$chaos_CONFIG" = "no" ; then
    no_chaos=yes
  else
    CHAOS_CFLAGS=`$chaos_CONFIG $chaosconf_args --cflags`
    CHAOS_LIBS=`$chaos_CONFIG $chaosconf_args --libs`

    chaos_major_version=`$chaos_CONFIG $chaos_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    chaos_minor_version=`$chaos_CONFIG $chaos_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    chaos_micro_version=`$chaos_CONFIG $chaos_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_chaostest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $CHAOS_CFLAGS"
      LIBS="$LIBS $CHAOS_LIBS"
dnl
dnl Now check if the installed chaos is sufficiently new. (Also sanity
dnl checks the results of libchaos-config to some extent
dnl
      rm -f conf.chaostest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libchaos/io.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.chaostest");
  */
  { FILE *fp = fopen("conf.chaostest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_chaos_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_chaos_version");
     exit(1);
   }

   if (($chaos_major_version > major) ||
      (($chaos_major_version == major) && ($chaos_minor_version > minor)) ||
      (($chaos_major_version == major) && ($chaos_minor_version == minor) && ($chaos_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'libchaos-config --version' returned %d.%d.%d, but the minimum version\n", $chaos_major_version, $chaos_minor_version, $chaos_micro_version);
      printf("*** of chaos required is %d.%d.%d. If libchaos-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If libchaos-config was wrong, set the environment variable chaos_CONFIG\n");
      printf("*** to point to the correct copy of libchaos-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_chaos=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_chaos" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$chaos_CONFIG" = "no" ; then
       echo "*** The libchaos-config script installed by libchaos could not be found"
       echo "*** If libchaos was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the chaos_CONFIG environment variable to the"
       echo "*** full path to libchaos-config."
     else
       if test -f conf.chaostest ; then
        :
       else
          echo "*** Could not run chaos test program, checking why..."
          CFLAGS="$CFLAGS $CHAOS_CFLAGS"
          LIBS="$LIBS $CHAOS_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "chaos.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding chaos or finding the wrong"
          echo "*** version of chaos. If it is not finding chaos, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means chaos was incorrectly installed"
          echo "*** or that you have moved chaos since it was installed. In the latter case, you"
          echo "*** may want to edit the libchaos-config script: $chaos_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     CHAOS_CFLAGS=""
     CHAOS_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(CHAOS_CFLAGS)
  AC_SUBST(CHAOS_LIBS)
  rm -f conf.chaostest
])

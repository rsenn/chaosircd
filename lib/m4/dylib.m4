dnl  $Id: dylib.m4,v 1.2 2006/09/27 12:19:12 roman Exp $
dnl  ===========================================================================
dnl 
dnl  Configures dynamic linking
dnl 
dnl  Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>

dnl  check for OpenSSL
dnl  ------------------------------------------------------------------
AC_DEFUN([AC_CONFIG_DYLIB],[

  # both are enabled by default 
  A_ENABLE=auto
  PIE_ENABLE=auto
  DLM_ENABLE=auto

  # check for --*-static argument 
  AC_ARG_ENABLE([static],[  --enable-static    build static library (default)
    --disable-static   do not build static library],[A_ENABLE=auto; case $enableval in
    no|yes)  A_ENABLE=$enableval ;;
    *) A_ENABLE="auto" ;;
   esac
   
  AC_MSG_RESULT([$A_ENABLE])
   
   ])

  # check for --*-shared argument 
  AC_ARG_ENABLE([shared],[  --enable-shared    build shared library (default)
    --disable-shared   do not build shared library],[PIE_ENABLE=auto 
    case $enableval in
      no|yes)  PIE_ENABLE=$enableval ;; *) PIE_ENABLE="auto" ;;
    esac
    ])

  # check for --*-loadable* argument 
  AC_ARG_ENABLE([loadable-modules],
[  --enable-loadable-modules    build runtime-loadable modules (default)
  --disable-loadable-modules    
],[DLM_ENABLE=auto 
    case $enableval in
      no|yes)  DLM_ENABLE=$enableval ;; *) DLM_ENABLE="auto" ;;
    esac
    ])

  WIN32='false'
  LINUX='false'
  FREEBSD='false'
  DARWIN='false'

  PIE_LIBEXT='so'

  case $host in
    *-mingw32 | *-cygwin) WIN32='true' ;;
    apple-* | *-darwin*) DARWIN='true' ;;
    *-linux*) LINUX='true' ;;
    *-freebsd*) FREEBSD='true' ;;
  esac

  # resolve automatic shit
  case $host in
    *-mingw32 | *-cygwin)
    
      case "$PIE_ENABLE,$A_ENABLE" in
        auto,auto) PIE_ENABLE='yes' A_ENABLE='no';;
        auto,no) PIE_ENABLE='yes' ;;
        auto,yes) PIE_ENABLE='no' ;;
        no,auto) A_ENABLE='yes' ;;
        yes,auto) A_ENABLE='no' ;;
      esac
      ;;
      
    *)
      if test "$PIE_ENABLE" = auto -a "$A_ENABLE" = auto; then
        A_ENABLE=yes
        PIE_ENABLE=no
        DLM_ENABLE=yes
      fi
      ;;
  esac

   if test "$DLM_ENABLE" = auto -a "$PIE_ENABLE" != no -a "$A_ENABLE" != yes; then
     	PIE_ENABLE=yes
		DLM_ENABLE=yes
  fi

	if test "$DLM_ENABLE" = yes; then
		case "$host" in
		  *darwin*|*apple*)
		PIE_ENABLE=yes
	;;
	esac
  fi
 
  
dnl   if test "$DLM_ENABLE" = yes -a "$PIE_ENABLE" != yes; then
dnl     PIE_ENABLE=yes
dnl   fi

#  if test "$PIE_ENABLE" = auto; then
#    if test "$A_ENABLE" = yes; then
#        PIE_ENABLE=no
#    elif test "$A_ENABLE" = no; then
#        PIE_ENABLE_=yes
#    fi
#  fi
#
#  if test "$A_ENABLE" = auto; then
#    if test "$PIE_ENABLE" = yes; then
#        A_ENABLE=no
#    elif test "$PIE_ENABLE" = no; then
#        A_ENABLE_=yes
#    fi
#  fi

  if test "$DLM_ENABLE" = auto; then
    if test "$A_ENABLE" = yes -o "$PIE_ENABLE" = no; then
      DLM_ENABLE=no
    elif test "$A_ENABLE" = no -o "$PIE_ENABLE" = yes; then
      DLM_ENABLE=yes
    fi
  fi

  # do some checks for PIC/PIE
        PIC_OBJEXT='o'

  # PIC compiling and PIE linking is host dependant
  case $host in

    *linux*|*freebsd*)
        PIC_CFLAGS='-fPIC'
        PIC_OBJEXT='pio'
        PIC_DEPEXT='pd'

      if test "$PIE_ENABLE" = "yes" -o "$DLM_ENABLE" = yes; then
        PIE_PREPEND='lib'
        PIE_LINK='$(LINK)'
        PIE_NAME='$(LIBNAME)'
        PIE_LDFLAGS="-shared -Wl,-soname -Wl,${PACKAGE_NAME}.${PIE_LIBEXT}.${VERSION_MAJOR}"
        PIE_VERSION_SUFFIX='.$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)'
        PIE_LINKS='$(PIE_NAME).$(PIE_LIBEXT).$(VERSION_MAJOR).$(VERSION_MINOR) $(PIE_NAME).$(PIE_LIBEXT).$(VERSION_MAJOR) $(PIE_NAME).$(PIE_LIBEXT)'
        PIE_LIBDIR='$(libdir)'
      fi
      ;;

    *darwin*)
        PIC_CFLAGS='-fPIC'
        PIC_OBJEXT='dyobj'
        PIC_DEPEXT='dydep'

      if test "$PIE_ENABLE" = "yes"; then
        PIE_LIBEXT='dylib'
        PIE_PREPEND='lib'
        PIE_LINK='$(LINK)'
        PIE_NAME='$(LIBNAME)'
        PIE_LDFLAGS="-dynamiclib -undefined error -install_name \$(libdir)/$PACKAGE_NAME.$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH.$PIE_LIBEXT -compatibility_version $VERSION_MAJOR.$VERSION_MINOR -current_version $VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH"
        PIE_VERSION_PREFIX='.$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)'
        PIE_LINKS='$(PIE_NAME).$(VERSION_MAJOR).$(VERSION_MINOR).$(PIE_LIBEXT) $(PIE_NAME).$(VERSION_MAJOR).$(PIE_LIBEXT) $(PIE_NAME).$(PIE_LIBEXT)'
        PIE_LOADER='dlfcn_darwin'
        PIE_LIBDIR='$(libdir)'
      fi
      ;;

    *mingw32*|*cygwin*|*msys*)
        PIC_OBJEXT='pio'
        PIC_DEPEXT='d'
        PIE_LIBEXT='dll'
      if test "$PIE_ENABLE" = "yes" -o "$DLM_ENABLE" = yes; then    
        PIC_CFLAGS=''
        PIC='#'
        PIE_NAME='$(LIBNAME:lib%=%)'
        PIE_LINK='$(CC)'
        PIE_LDFLAGS='-shared'
        PIE_LIBDIR='$(bindir)'
      fi
       case $host in
         *cygwin*) PIE_PREPEND=cyg ;;
         *msys*) PIE_PREPEND=msys- ;;
         *) PIE_PREPEND=lib ;;
        esac
      
      case "$PIE_ENABLE,$A_ENABLE" in
        yes,yes)
          AC_MSG_WARN([On windows you will want to build either shared OR static libraries, not both])
          A_ENABLE='no'
          ;;
      esac
      ;;
    *)
      if test "$PIE_ENABLE" = "yes"; then
        AC_MSG_WARN([Your system is not supported for shared linking])
        PIE_ENABLE="no"
      fi
      ;;
  esac

  AC_MSG_CHECKING([wheter to build a static library])
  AC_MSG_RESULT([$A_ENABLE])
  AC_MSG_CHECKING([wheter to build a shared library])
  AC_MSG_RESULT([$PIE_ENABLE])
  AC_MSG_CHECKING([wheter to build loadable modules])
  AC_MSG_RESULT([$DLM_ENABLE])

  case "$PIE_ENABLE,$A_ENABLE" in
    no,no)
      AC_MSG_ERROR([Seems you explicitly disabled both, static and shared libraries, so there is nothing left to compile.])
      ;;
  esac

  # do some checks for static libs
  if test "$A_ENABLE" = "yes"; then
    
    A_EXEEXT="a"
  fi

  AM_CONDITIONAL([WIN32], [$WIN32])
  AM_CONDITIONAL([LINUX], [$LINUX])
  AM_CONDITIONAL([FREEBSD], [$LINUX])
  AM_CONDITIONAL([DARWIN], [$DARWIN])
  AM_CONDITIONAL([SHARED], [test "$PIE_ENABLE" = "yes"])
  AM_CONDITIONAL([STATIC], [test "$A_ENABLE" = "yes"])

  AC_SUBST([PIE_LIBDIR])
  AC_SUBST([slibdir], [$PIE_LIBDIR])

  saved_LIBS="$LIBS"
  LIBS=""
  AC_CHECK_LIB([dl], [dlopen])
  DLFCN_LIBS="$LIBS"
  LIBS="$saved_LIBS"

  # set up static library 
  if test "$A_ENABLE" = "yes"; then
    A_LIB=""
    NO_A_LIB="#"
  else
    A_LIB="#"
    NO_A_LIB=""
  fi

  # set up shared library 
  if test "$PIE_ENABLE" = "yes"; then
    PIE_LIB=""
    NO_PIE_LIB="#"
  else
    PIE_LIB="#"
    NO_PIE_LIB=""
  fi
  AM_CONDITIONAL([PIE],[test "$PIE_ENABLE" = yes])
  AM_CONDITIONAL([A],[test "$A_ENABLE" = yes])
  AM_CONDITIONAL([DLM],[test "$DLM_ENABLE" = yes])

  AC_SUBST([PIE_LIB])
  AC_SUBST([NO_PIE_LIB])

  AC_SUBST([A_EXEEXT])

  AC_SUBST([PIC_CFLAGS])
  AC_SUBST([PIC_OBJEXT])
  AC_SUBST([PIC_DEPEXT])

  AC_SUBST([PIE_NAME])
  AC_SUBST([PIE_LINK])
  AC_SUBST([PIE_LINKS])
  AC_SUBST([PIE_LDFLAGS])
  AC_SUBST([PIE_LIBEXT])
  AC_SUBST([PIE_VERSION_PREFIX])
  AC_SUBST([PIE_VERSION_SUFFIX])
  AC_SUBST([PIE_PREPEND])

  AC_SUBST([NO_PIC])
  AC_SUBST([PIC])

  AC_SUBST([PIE_ENABLE])

  AC_SUBST([PIE_LOADER])

  AC_SUBST([PIE_LIB])
  AC_SUBST([NO_PIE_LIB])

  AC_SUBST([A_ENABLE])
  AC_SUBST([A_LIB])
  AC_SUBST([NO_A_LIB])

  AC_SUBST([LIBS])
  AC_SUBST([DLFCN_LIBS])

])


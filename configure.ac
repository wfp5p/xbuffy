dnl| configure.in

AC_INIT([xbuffy], [3.6])
AC_CONFIG_HEADERS(config.h)
AH_TEMPLATE([HAVE_CCLIENT], [Define if the c-client lib is available])
AH_TEMPLATE([USE_CONTENT_LENGTH], [Define if the Content-Length header is to be trusted])
AH_TEMPLATE([USE_NNTP], [Define if the nntp support is desired])

AM_INIT_AUTOMAKE

if test -z "$CFLAGS" ; then
dnl| Prevent the next macro from setting CFLAGS to -g
  CFLAGS=" "
fi
AC_PROG_CC
AC_PROG_CPP
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_CHECK_LIB(HX, HX_init, [true], AC_MSG_ERROR(cound not find libHX))

AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_HEADER_DIRENT
AC_TYPE_SIGNAL
AC_CHECK_HEADERS(fcntl.h malloc.h strings.h sys/time.h unistd.h)
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gettimeofday socket setpriority memcpy)
AC_TYPE_OFF_T
AC_HEADER_TIME

AC_PATH_XTRA
if test -n "$no_x" ; then
  echo xbuffy requires the X Window System header files and libraries!
  echo They were not found on your system.
  echo configure failed
  exit 1
fi

AC_MSG_CHECKING(whether to use c-client)
AC_ARG_WITH(cclient,
[  --with-cclient[=PATH]   enable features that use the c-client library],
[ case "$withval" in
  no)
    AC_MSG_RESULT(no)
    ;;
  yes)
     if test '!' -f ./imap/c-client/c-client.a; then
        AC_MSG_ERROR(c-client.a not found in ./imap/c-client: you must supply the path.)
     fi

     AC_MSG_RESULT(yes)
     AC_MSG_RESULT(Assuming c-client headers are in $top_srcdir/imap/c-client.)
     AC_DEFINE([HAVE_CCLIENT], [1])
     CFLAGS="$CFLAGS -I$srcdir/imap"
     LIBS="$srcdir/imap/c-client/c-client.a $LIBS"
     AC_CHECK_LIB(c, crypt, [true], AC_CHECK_LIB(crypt, crypt))
     ;;
   *)

     if test '!' -f $withval/c-client/c-client.a; then
             AC_MSG_ERROR(c-client.a not found in $withval/c-client: you must supply the path.)
     fi
     AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_CCLIENT)
     CFLAGS="$CFLAGS -I$withval"
     LIBS="$withval/c-client/c-client.a $LIBS"
     AC_CHECK_LIB(c, crypt, [true], AC_CHECK_LIB(crypt, crypt))
     ;;
esac],
AC_MSG_RESULT(no)
)

AC_ARG_ENABLE(nntp,
[  --enable-nntp[=HOST]    enable checking of NNTP boxes, set this to the
                          name of the NNTP server to use],
[ if test "$enableval" != no ; then
     AC_DEFINE_UNQUOTED(USE_NNTP, "$enableval")
fi])

AC_ARG_ENABLE(content_length,
[  --enable-content-length enable use of content-length headers],
[ if test "$enableval" != no ; then
     AC_DEFINE(USE_CONTENT_LENGTH)
fi])


AC_OUTPUT(Makefile)

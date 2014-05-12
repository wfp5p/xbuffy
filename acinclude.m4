AC_DEFUN([DM_CHECK_GMIME], [dnl
AC_PATH_PROG(gmimeconfig,pkg-config)
if test [ -z "$gmimeconfig" ]
then
	AC_MSG_ERROR([pkg-config executable not found. Make sure pkg-config is in your path])
else
	AC_MSG_CHECKING([GMime headers])
	ac_gmime_cflags=`${gmimeconfig} --cflags gmime-2.6 2>/dev/null|| ${gmimeconfig} --cflags gmime-2.4 2>/dev/null`
	if test -z "$ac_gmime_cflags"
	then
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([Unable to locate gmime development files])
	else
		CFLAGS="$CFLAGS $ac_gmime_cflags"
		AC_MSG_RESULT([$ac_gmime_cflags])
	fi

        AC_MSG_CHECKING([GMime libraries])
	ac_gmime_libs=`${gmimeconfig} --libs gmime-2.6 2>/dev/null|| ${gmimeconfig} --libs gmime-2.4 2>/dev/null`
	if test -z "$ac_gmime_libs"
	then
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([Unable to locate gmime libaries])
	else
		LDFLAGS="$LDFLAGS $ac_gmime_libs"
        	AC_MSG_RESULT([$ac_gmime_libs])
	fi
fi
])

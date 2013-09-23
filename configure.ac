
AC_PREREQ([2.69])
AC_INIT([smoothscan], [0.1.0], [nate@natecraun.net])
AM_INIT_AUTOMAKE([gnu])
AC_CONFIG_SRCDIR([src/smoothscan.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

AC_CHECK_PROG([FONTFORGE], [fontforge], [yes], [no])
if test x$FONTFORGE == x"no"
then
   AC_MSG_ERROR([fontforge not found])
fi
AC_CHECK_PROG([PYTHON], [python], [yes], [no])
if test x$PYTHON == x"no"
then
   AC_MSG_ERROR([python not found])
fi
AC_CHECK_PROG([POTRACE], [potrace], [yes], [no])
if test x$POTRACE == x"no"
then
   AC_MSG_ERROR([potrace not found])
fi

# see what version of python fontforge was compiled for

PY2=`tests/fontforge-python2.py`
PY3=`tests/fontforge-python3.py`

AC_MSG_CHECKING([fontforge compiled with python2])
if test x$PY2 == x"yes"
then
   AC_MSG_RESULT([yes])
   AC_SUBST([FONTFORGE_PYTHON_VERSION], [python2])
else
   AC_MSG_RESULT([no])
   AC_MSG_CHECKING([fontforge compiled with python3])
   if test x$PY3 == x"yes"
   then
     AC_MSG_RESULT([yes])
     AC_SUBST([FONTFORGE_PYTHON_VERSION], [python3])
     else
       AC_MSG_RESULT([no])
       AC_MSG_ERROR([Fontforge was not compiled with python support])
   fi
fi


# fontforge
# python
# potrace
# leptonica
# libharu

# Checks for libraries.
AC_CHECK_LIB([lept], [jbCorrelationInitWithoutComponents], [], [AC_MSG_ERROR([leptonica library not found or not usable])])
AC_CHECK_LIB([hpdf], [HPDF_New], [], [AC_MSG_ERROR([libharu library not found])])
# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

AC_CHECK_HEADERS([leptonica/allheaders.h], [], [AC_MSG_ERROR([Leptonica headers not found or not usable])])
AC_CHECK_HEADERS([hpdf.h], [], [AC_MSG_ERROR([libharu headers not found or not usable])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([mkdir strerror])

AC_CONFIG_FILES([src/smoothscan-fontgen.py])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

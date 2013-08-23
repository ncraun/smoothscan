
AC_PREREQ([2.69])
AC_INIT([smoothscan], [0.1.0], [nate@natecraun.net])
AM_INIT_AUTOMAKE([gnu])
AC_CONFIG_SRCDIR([src/smoothscan.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

AC_CHECK_PROG([FONTFORGE], [fontforge], 1, 0)
if test $FONTFORGE = 0
then
   AC_MSG_ERROR([fontforge not found])
fi
AC_CHECK_PROG([PYTHON], [python], 1, 0)
if test $PYTHON = 0
then
   AC_MSG_ERROR([python not found])
fi
AC_CHECK_PROG([POTRACE], [potrace], 1, 0)
if test $POTRACE = 0
then
   AC_MSG_ERROR([potrace not found])
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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

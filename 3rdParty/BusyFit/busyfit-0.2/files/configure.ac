# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
#AC_INIT([busyfit], [0.2], [tobias.westmeier@uwa.edu.au, Matthew.Whiting@csiro.au])
AC_INIT([busyfit], [0.2], [Matthew.Whiting@csiro.au])
AC_PREFIX_DEFAULT(/usr/local)

AC_CONFIG_AUX_DIR([.])

# Get the system type.
AC_CANONICAL_BUILD
ARCH="${build_cpu}-$build_os"
AC_SUBST([ARCH])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_F77
AC_PROG_LN_S
AC_CHECK_PROGS(INSTALL, install)
LINKER=$CXX 

# Check for maths library -- will add -lm to $LIBS
AC_CHECK_LIB([m], [log])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h time.h math.h values.h iostream fstream sstream iomanip vector string algorithm functional])
if test "x$ac_cv_header_stdc" = xno; then
  AC_MSG_ERROR([
    -------------------------------------------------------------------
    An ANSI standard C library is required to build busyfit.  
    One of the ANSI C header files it requires is missing or unusable.

    ERROR: busyfit configuration failure.
    -------------------------------------------------------------------], [1])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Library and installation utilities.
#------------------------------------
#     (Taken from Mark Calabretta's wcslib package)
# Static library generation.
AC_PROG_RANLIB
# Installation utilities.
#AC_PROG_LN_S
#AC_PROG_INSTALL


# Shared library generation.
if test "x$ac_cv_c_compiler_gnu" = xyes ; then
  # Note that -fPIC is on by default for Macs, this just makes it obvious.
  SHRFLAGS="-fPIC"
  SHRLD="\$(LINK) $SHRFLAGS"

  case "$build_os" in
  darwin*)
    AC_MSG_NOTICE([ Using darwin build option])
    SHRLIB="libbusyfit.$PACKAGE_VERSION.dylib"
    SHRLD="$SHRLD -dynamiclib -single_module"
    SHRLD="$SHRLD -compatibility_version $PACKAGE_VERSION -current_version $PACKAGE_VERSION"
    SHRLN="libduchamp.dylib"

    case "$build_cpu" in
    powerpc*)
      AC_MSG_NOTICE([ Using darwin build option with powerpc subcase])		
      # Switch off -fPIC (not applicable for Intel Macs).
      CFLAGS="$CFLAGS -mdynamic-no-pic"
      ;;
    esac
    ;;
  *)
    # Covers Linux and Solaris at least.
    AC_MSG_NOTICE([ Using alternative (linux) build option])
    SHRLIB="libduchamp.so.$PACKAGE_VERSION"
    SHRLD="$SHRLD -shared -Wl,-soname,$SHRLIB"
    SHRLN="libduchamp.so"
    ;;
  esac

else
    AC_MSG_NOTICE([ Not setting shared library options])
  SHRLIB=
  SHRFLAGS=
  SHRLD=
  SHRSFX=
  SHRLN=
fi

AC_SUBST([SHRLIB])
AC_SUBST([SHRFLAGS])
AC_SUBST([SHRLD])
AC_SUBST([SHRSFX])
AC_SUBST([SHRLN])

# Checks for library functions.
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor pow sqrt strtol log atan fabs])

## # Extra places to look for third-party include files and libraries.
## INCDIRS="$INCDIRS           \
##          /usr/include       \
##          /usr/local/include \
##          /usr/local/pgplot  \
##          /usr/local/cfitsio \
##          /usr/local/wcslib  \
##          /local/pgplot      \
##          /local/cfitsio     \
##          /opt/local/include"
## 
## LIBDIRS="$LIBDIRS           \
##          /usr/lib           \
##          /usr/local/lib     \
##          /usr/local/pgplot  \
##          /usr/local/cfitsio \
##          /usr/local/wcslib  \
##          /local/lib         \
##          /local/pgplot      \
##          /local/cfitsio     \
##          /opt/SUNWspro/lib  \
## 	 /opt/local/lib"
## 
## AC_MSG_NOTICE(LIBDIRS)
## 
## for LIBDIR in $LIBDIRS ; do
##   AC_CHECK_FILE([$LIBDIR], [LDFLAGS="$LDFLAGS -L$LIBDIR"], [continue])
## done

##########################################################################
# Search for GSL.
# use --with-gsl=directory to specify a particular GSL directory.
# else do the normal searching for libraries.

AC_MSG_NOTICE([  -- GSL -- ])
AC_ARG_WITH(gsl,
	AC_HELP_STRING([--with-gsl=<dir>],
		       [The GSL library and include files are in directory <dir>.]),
[ #if we specify a directory...
if test "x$withval" = xno; then
  AC_MSG_NOTICE([
    The GSL library is required.
    Ignoring the "--with-gsl=no" and searching for the library.], [1])
  GSLINCDIRS=$INCDIRS
  GSLLIBDIRS=$LIBDIRS
else
  AC_MSG_NOTICE([Using directory "$withval"])
  GSLINCDIRS="$withval \
              $withval/include"
  GSLLIBDIRS="$withval \
              $withval/lib"
  for LIBDIR in $GSLLIBDIRS ; do
    AC_CHECK_FILE([$LIBDIR], [LDFLAGS="$LDFLAGS -L$LIBDIR"], [continue])
  done
  for INCDIR in $GSLINCDIRS ; do
    AC_CHECK_FILE([$INCDIR], [CPPFLAGS="$CPPFLAGS -I$INCDIR"], [continue])
  done
fi
],
[ # if directory not specified, go searching.
GSLINCDIRS=$INCDIRS
GSLLIBDIRS=$LIBDIRS
])

for INCDIR in $GSLINCDIRS ; do
  AC_CHECK_FILE([$INCDIR/gsl/gsl_matrix.h], [GSLINC=-I$INCDIR; break])
done

AC_CHECK_LIB([gsl], [gsl_matrix_alloc], [GSLLIB="$GSLLIB -lgsl -lgslcblas"], [],
             [-l gslcblas $GSLLIB $LIBS])
AC_CHECK_LIB([gslcblas], [gsl_matrix_alloc], [GSLLIB="$GSLLIB -lgsl -lgslcblas"], [],
             [-l gsl $GSLLIB $LIBS])

# GSL is essential for busyfit, so exit with error message if it
#   is not present.
if test "x$GSLINC" = x -o "x$GSLLIB" = x; then
  AC_MSG_ERROR([
    -------------------------------------------------------
    Could not find the GSL library.

    ERROR: busyfit configuration failure.
    -------------------------------------------------------], [1])
else
  AC_MSG_NOTICE([GSL appears to be available.])
  AC_DEFINE([HAVE_GSL], [1], [Define to 1 if GSL is available.])
for LIBDIR in $GSLLIBDIRS ; do
  AC_CHECK_FILE([$LIBDIR/libgsl.a], [GSLLIB="-L$LIBDIR $GSLLIB"; break])
  LIBDIR=$LIBDIR/gsl
  AC_CHECK_FILE([$LIBDIR/libgsl.a], [GSLLIB="-L$LIBDIR $GSLLIB"; break])
done

fi

AC_SUBST([GSLINC])
AC_SUBST([GSLLIB])
##########################################################################

AC_SUBST([LINKER])
AC_SUBST([LDFLAGS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

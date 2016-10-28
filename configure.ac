AC_INIT([MPIre], [0.1], [cere-dev@googlegroups.com])
AC_CONFIG_AUX_DIR(autoconf)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects silent-rules foreign])

AC_PROG_LIBTOOL
AC_PROG_CC

AC_CHECK_LIB([c], [exit], , AC_MSG_ERROR([Could not find c library]))
AC_CHECK_LIB([mpi], [exit], , AC_MSG_ERROR([Could not find MPI library]))

# Required for ccan
AC_C_BIGENDIAN(
               [AC_DEFINE(HAVE_BIG_ENDIAN, 1, [Define to 1 if machine is big endian])],
               [AC_DEFINE(HAVE_LITTLE_ENDIAN, 1, [Define to 1 if machine is little endian])],
               [AC_MSG_ERROR([endianess of this machine is unknown])],
               [AC_MSG_ERROR([universal endianess not supported])]
               )

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 tests/Makefile
                 src/Makefile
                 src/mpire/Makefile
                 src/ccan/Makefile])

AC_OUTPUT

/* The number of bytes in a char.  */
#define SIZEOF_CHAR 1

/* The number of bytes in a char *.  */
#define SIZEOF_CHAR_P 4

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a long long.  */
#define SIZEOF_LONG_LONG 8

/* The number of bytes in a long long int.  */
#define SIZEOF_LONG_LONG_INT 8

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a size_t.  */
#define SIZEOF_SIZE_T 4

/* do we have ptrace? */
#define HAVE_PTRACE 1

/* /proc/pid/mem interface */
#define PROC_MEM_FILE "mem"

#if defined(HAVE_PTRACE) || defined(PROC_MEM_FILE) 
#define HAVE_FPM_TRACE 1
#else
#define HAVE_FPM_TRACE 0
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

/* If we're not using GNU C, elide __attribute__ */
#ifndef __GNUC__
#  define  __attribute__(x)  /*NOTHING*/
#endif

/* Solaris does not have it */
#ifndef timersub
#define	timersub(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <math.h>

#define FPM_VERSION   "0.5.9"
#define FPM_CONF_PATH "/usr/local/src/fpmcrowd/build/fpmcrowd.conf"
#define FPM_LOG_PATH  "/usr/local/src/fpmcrowd/build/fpmcrowd.log"
#define FPM_PID_PATH  "/usr/local/src/fpmcrowd/build/fpmcrowd.pid"

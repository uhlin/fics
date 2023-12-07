/* autoconfig.h.  Generated automatically by configure.  */
/* autoconfig.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if your struct tm has tm_zone.  */
#define HAVE_TM_ZONE 1

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
/* #undef HAVE_TZNAME */

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define if you have the wait3 system call.  */
#define HAVE_WAIT3 1

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/*  Define this to be the return value of the time() function.  */
/* #undef time_t */

/*  Define, if you have a statfs() function which fills a struct
    statfs, declared in sys/vfs.h. (NeXTStep)  */
/* #undef HAVE_STATFS_FILLING_STRUCT_STATFS */

/*  Define, if you have a statfs() function which fills a struct
    fs_data, declared in sys/mount.h. (Ultrix)  */
/* #undef HAVE_STATFS_FILLING_STRUCT_FS_DATA */

/*  Define, if you have crypt() and it is declared in either
    crypt.h or unistd.h.  */
/* #undef HAVE_CRYPT_DECLARED */

/*  Define this to be tv_usec, if your struct rusage has
    a member ru_utime.tv_usec.
    Define this to be tv_nsec, if your struct rusage has
    a member ru_utime.tv_nsec.  */
#define TV_USEC tv_usec

/* Define if you have the crypt function.  */
#define HAVE_CRYPT 1

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the malloc_debug function.  */
/* #undef HAVE_MALLOC_DEBUG */

/* Define if you have the malloc_size function.  */
/* #undef HAVE_MALLOC_SIZE */

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the statfs function.  */
/* #undef HAVE_STATFS */

/* Define if you have the strcspn function.  */
#define HAVE_STRCSPN 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the <bstring.h> header file.  */
/* #undef HAVE_BSTRING_H */

/* Define if you have the <crypt.h> header file.  */
/* #undef HAVE_CRYPT_H */

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <math.h> header file.  */
#define HAVE_MATH_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <sys/cred.h> header file.  */
/* #undef HAVE_SYS_CRED_H */

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/file.h> header file.  */
#define HAVE_SYS_FILE_H 1

/* Define if you have the <sys/filio.h> header file.  */
#define HAVE_SYS_FILIO_H 1

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/mount.h> header file.  */
#define HAVE_SYS_MOUNT_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/resource.h> header file.  */
#define HAVE_SYS_RESOURCE_H 1

/* Define if you have the <sys/rusage.h> header file.  */
/* #undef HAVE_SYS_RUSAGE_H */

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/vfs.h> header file.  */
#define HAVE_SYS_VFS_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the crypt library (-lcrypt).  */
/* #undef HAVE_LIBCRYPT */

/* Define if you have the m library (-lm).  */
#define HAVE_LIBM 1

/* Define if you have the nsl library (-lnsl).  */
#define HAVE_LIBNSL 1

/* Define if you have the resolv library (-lresolv).  */
#define HAVE_LIBRESOLV 1

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

#ifndef _MAKE_DAEMON_
#define _MAKE_DAEMON_
    #define NO_CHDIR          01
    #define NO_CLOSE_FILES    02
    #define NO_REOPEN_STD_FDS 04
    #define NO_UMASK0        010
    #define MAX_CLOSE       8192

    extern int make_daemon(int flags);
#endif
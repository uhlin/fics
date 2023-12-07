/* shutdown.h */

#ifndef _SHUTDOWN_H
#define _SHUTDOWN_H

extern void output_shut_mess();
extern int com_shutdown();
extern void ShutHeartBeat();
extern void ShutDown();
extern int server_shutdown();
extern int check_and_print_shutdown(int);
extern int com_whenshut();

#endif /* _SHUTDOWN_H */

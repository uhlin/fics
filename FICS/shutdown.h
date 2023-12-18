/* shutdown.h */

#ifndef _SHUTDOWN_H
#define _SHUTDOWN_H

extern int check_and_print_shutdown(int);
extern int com_shutdown();
extern int com_whenshut();
extern int server_shutdown();
extern void ShutDown();
extern void ShutHeartBeat();
extern void output_shut_mess();

#endif /* _SHUTDOWN_H */

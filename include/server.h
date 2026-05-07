#ifndef SERVER_H
#define SERVER_H

#define MESSAGE_PORT 4596
#define MULTI_ADDR "224.0.2.17"
#define MULTI_PORT 6841
#define MAX_SERVER_NAME_LENGTH 64
#define MAX_MESSAGE_LENGTH 128
#define MAX_CLIENT_NAME_LENGTH 16
#define SLEEP_NANO 50000000

int run_server(void);

#endif

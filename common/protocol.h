#ifndef PROTOCOL_H
#define PROTOCOL_H

#define CMD_REGISTER  1
#define CMD_LOGIN     2
#define CMD_VIEW_MENU 3
#define CMD_ORDER     4
#define CMD_EXIT      5

typedef struct {
    int command;
    char payload[256];
} Request;

typedef struct {
    int status; // 0=FAIL, 1=SUCCESS
    char message[256];
} Response;

#endif


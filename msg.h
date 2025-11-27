#ifndef MSG_H
#define MSG_H

#include <winsock2.h>

typedef struct {
    int valid;
    char message_type[64];
    int hp;
    int damage;
    int seq;    /* sequence number (if present) */
    int ack;    /* ack number (if present) */
} BattleMessage;

/* Parse raw newline-delimited "key: value" message into BattleMessage.
   The parser is intentionally forgiving: it looks for keys anywhere in the raw string.
*/
BattleMessage msg_parse(const char *raw);

#endif /* MSG_H */

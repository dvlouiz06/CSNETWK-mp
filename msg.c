#include "msg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

BattleMessage msg_parse(const char *raw) {
    BattleMessage m;
    m.valid = 0;
    m.message_type[0] = '\0';
    m.hp = 0;
    m.damage = 0;
    m.seq = 0;
    m.ack = 0;

    if (!raw) return m;

    /* message_type */
    const char *p = strstr(raw, "message_type:");
    if (p) {
        /* read token after 'message_type:' up to newline or space */
        char mt[64] = {0};
        if (sscanf(p, "message_type: %63s", mt) == 1) {
            size_t len = strlen(mt);
            if (len >= sizeof(m.message_type)) len = sizeof(m.message_type) - 1;
            strncpy(m.message_type, mt, len);
            m.message_type[len] = '\0';
            m.valid = 1;
        }
    }

    /* hp */
    p = strstr(raw, "hp:");
    if (p) {
        m.hp = atoi(p + 3);
    }

    /* damage */
    p = strstr(raw, "damage:");
    if (p) {
        m.damage = atoi(p + 7);
    }

    /* sequence / seq */
    p = strstr(raw, "sequence_number:");
    if (p) {
        m.seq = atoi(p + (int)strlen("sequence_number:"));
    } else {
        p = strstr(raw, "sequence_number");
        if (p) m.seq = atoi(p + (int)strlen("sequence_number"));
    }

    /* ack_number / ack */
    p = strstr(raw, "ack_number:");
    if (p) {
        m.ack = atoi(p + (int)strlen("ack_number:"));
    } else {
        p = strstr(raw, "ack_number");
        if (p) m.ack = atoi(p + (int)strlen("ack_number"));
    }

    return m;
}

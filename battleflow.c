#include "battleflow.h"
#include "reliability.h"
#include "rng.h"
#include "damage.h"
#include <stdio.h>
#include <string.h>

void bf_init(BattleState *s)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->state = GS_SETUP;
    s->hostTurn = 1;
    s->attack_pending = 0;

    /* default test stats */
    s->host.attack = 100;
    s->host.defense = 80;
    s->host.spAttack = 109;
    s->host.spDefense = 90;
    s->host.hp = 250;
    strcpy(s->host.type1, "Fire");

    s->joiner.attack = 80;
    s->joiner.defense = 90;
    s->joiner.spAttack = 50;
    s->joiner.spDefense = 70;
    s->joiner.hp = 250;
    strcpy(s->joiner.type1, "Water");
}

/* HOST */
void bf_process_host(BattleState *s, SOCKET sock,
                     struct sockaddr_in *addr, int addrLen,
                     BattleMessage *m)
{
    if (!s || !m || !m->valid) return;

    if (strcmp(m->message_type, "BATTLE_SETUP") == 0) {
        if (m->hp > 0) s->joiner.hp = m->hp;
        printf("[BF] Host received BATTLE_SETUP. Joiner HP=%d\n", s->joiner.hp);
        rl_send(sock, addr, addrLen, "message_type: BATTLE_SETUP\nhp: 250\n");
        printf("[HOST] Sent BATTLE_SETUP\n");
        s->state = GS_WAITING_FOR_MOVE;
        return;
    }

    if (strcmp(m->message_type, "DEFENSE_ANNOUNCE") == 0) {
        printf("[HOST] Received DEFENSE_ANNOUNCE — waiting for CALCULATION_REPORT\n");
        s->state = GS_PROCESSING_TURN;
        return;
    }

    if (strcmp(m->message_type, "CALCULATION_REPORT") == 0) {
        int reported = m->damage;
        printf("[HOST] Received CALCULATION_REPORT damage=%d — AUTO-ACCEPTING\n", reported);
        s->joiner.hp -= reported;
        if (s->joiner.hp < 0) s->joiner.hp = 0;
        rl_send(sock, addr, addrLen, "message_type: CALCULATION_CONFIRM\n");
        printf("[BF] Host applied reported damage=%d. Host HP=%d Joiner HP=%d\n",
               reported, s->host.hp, s->joiner.hp);
        if (s->joiner.hp <= 0) {
            rl_send(sock, addr, addrLen, "message_type: GAME_OVER\n");
            printf("[BF] Host declares GAME_OVER (joiner fainted)\n");
            s->state = GS_GAME_OVER;
            return;
        }
        s->hostTurn = 0;
        s->state = GS_WAITING_FOR_MOVE;
        return;
    }

    if (strcmp(m->message_type, "GAME_OVER") == 0) {
        printf("[BF] Host received GAME_OVER\n");
        s->state = GS_GAME_OVER;
        return;
    }
}

/* JOINER */
void bf_process_joiner(BattleState *s, SOCKET sock,
                       struct sockaddr_in *addr, int addrLen,
                       BattleMessage *m)
{
    if (!s || !m || !m->valid) return;

    if (strcmp(m->message_type, "BATTLE_SETUP") == 0) {
        if (m->hp > 0) s->host.hp = m->hp;
        printf("[BF] Joiner received HOST BATTLE_SETUP. Host HP=%d\n", s->host.hp);
        s->state = GS_WAITING_FOR_MOVE;
        return;
    }

    if (strcmp(m->message_type, "ATTACK_ANNOUNCE") == 0) {
        rl_send(sock, addr, addrLen, "message_type: DEFENSE_ANNOUNCE\n");
        printf("[JOINER] Sent DEFENSE_ANNOUNCE\n");
        int damage = calculate_damage(&s->host, &s->joiner, 40, 1);
        char buf[128];
        sprintf(buf, "message_type: CALCULATION_REPORT\ndamage: %d\n", damage);
        rl_send(sock, addr, addrLen, buf);
        printf("[JOINER] Sent CALCULATION_REPORT damage=%d\n", damage);
        s->state = GS_PROCESSING_TURN;
        return;
    }

    if (strcmp(m->message_type, "CALCULATION_CONFIRM") == 0) {
        s->hostTurn = 1;
        s->state = GS_WAITING_FOR_MOVE;
        printf("[JOINER] Received CALCULATION_CONFIRM - turn flips\n");
        return;
    }

    if (strcmp(m->message_type, "GAME_OVER") == 0) {
        printf("[BF] Joiner received GAME_OVER\n");
        s->state = GS_GAME_OVER;
        return;
    }
}

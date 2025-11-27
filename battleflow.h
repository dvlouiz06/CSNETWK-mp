#ifndef BATTLEFLOW_H
#define BATTLEFLOW_H

#include <winsock2.h>
#include "damage.h"
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

enum GameState {
    GS_SETUP,
    GS_WAITING_FOR_MOVE,
    GS_PROCESSING_TURN,
    GS_GAME_OVER
};

typedef struct {
    enum GameState state;
    Pokemon host;
    Pokemon joiner;
    int hostTurn;        // 1 => host's turn, 0 => joiner's turn
    int attack_pending;
} BattleState;

void bf_init(BattleState *s);

void bf_process_host(BattleState *s, SOCKET sock,
                     struct sockaddr_in *addr, int addrLen,
                     BattleMessage *m);

void bf_process_joiner(BattleState *s, SOCKET sock,
                       struct sockaddr_in *addr, int addrLen,
                       BattleMessage *m);

#ifdef __cplusplus
}
#endif

#endif /* BATTLEFLOW_H */

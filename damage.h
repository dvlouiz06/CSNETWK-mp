#ifndef DAMAGE_H
#define DAMAGE_H

typedef struct {
    int attack;
    int defense;
    int spAttack;
    int spDefense;
    int hp;
    char type1[32];
    char type2[32];
} Pokemon;

int calculate_damage(Pokemon *att, Pokemon *def, int basePower, int physical);

#endif

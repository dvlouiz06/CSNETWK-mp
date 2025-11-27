#include "damage.h"
#include "rng.h"
#include <math.h>

int calculate_damage(Pokemon *att, Pokemon *def,
                     int basePower, int physical)
{
    int atk  = physical ? att->attack      : att->spAttack;
    int defe = physical ? def->defense     : def->spDefense;

    if (defe <= 0) defe = 1;

    double randomFactor = ((rng_next() % 16) + 85) / 100.0;

    double dmg = (((((2.0 * 50.0) / 5.0) + 2.0)
                  * basePower * atk / (double)defe) / 50.0 + 2.0)
                  * 1.0 * randomFactor;

    if (dmg < 1.0) dmg = 1.0;

    return (int)floor(dmg);
}

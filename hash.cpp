#include "globals.h"

bool LookUp(const int s);
void AddHash(const int s, const move_data m);

U64 hashkey[2][6][64];
U64 lock[2][6][64];

U64 currentkey, currentlock;
U64 collisions;

const U64 MAXHASH = 5000000;
const U64 HASHSIZE = 5000000;

int hash_from, hash_to;

struct hashp
{
    U64 hashlock;
    int from;
    int to;
};

hashp* hashpos[2];

void RandomizeHash()
{
    for (int p = 0; p < 6; p++)
        for (int x = 0; x < 64; x++)
        {
            hashkey[WHITE][p][x] = Random(HASHSIZE);
            hashkey[BLACK][p][x] = Random(HASHSIZE);
            lock[WHITE][p][x] = Random(HASHSIZE);
            lock[BLACK][p][x] = Random(HASHSIZE);
        }
    hashpos[WHITE] = new hashp[MAXHASH];
    hashpos[BLACK] = new hashp[MAXHASH];
}

int Random(const int x)
{
    return rand() % x;
}

void Free()//unused
{
    delete[] hashpos[WHITE];
    delete[] hashpos[BLACK];
}

void AddKey(const int s, const int p, const int x)
{
    currentkey ^= hashkey[s][p][x];
    currentlock ^= lock[s][p][x];
}

U64 GetLock()
{
    U64 loc = 0;

    for (int x = 0; x < 64; x++)
    {
        if (board[x] != EMPTY)
        {
            loc ^= lock[color[x]][board[x]][x];
        }
    }
    return loc;
}

U64 GetKey()
{
    U64 key = 0;
    for (int x = 0; x < 64; x++)
    {
        if (board[x] != EMPTY)
        {
            key ^= hashkey[color[x]][board[x]][x];
        }
    }
    return key;
}

void AddHash(const int s, const move_data m)
{
    U64 index = currentkey % MAXHASH;
    hashp* ptr = &hashpos[s][index];
    ptr->hashlock = currentlock;
    ptr->from = m.from;
    ptr->to = m.to;
}

bool LookUp(const int s)
{
    U64 index = currentkey % MAXHASH;
    if (hashpos[s][index].hashlock != currentlock)
    {
        collisions++;
        return false;
    }

    hash_from = hashpos[s][currentkey].from;
    hash_to = hashpos[s][currentkey].to;
    return true;
}

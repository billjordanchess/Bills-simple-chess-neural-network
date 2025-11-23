#include "globals.h"

U64 hash[2][6][64];
U64 lock[2][6][64];

U64 currentkey,currentlock;
U64 collisions;

const U64 MAXHASH =  5000000;
const U64 HASHSIZE = 5000000;

/*

RandomizeHash is called when the engine is started.
The whitehash, blackhash, whitelock and blacklock tables
are filled with random numbers.

*/
void RandomizeHash()
{
	int p,x;
	for(p=0;p<6;p++)
		for(x=0;x<64;x++)
		{
			hash[WHITE][p][x] = Random(HASHSIZE);
			hash[BLACK][p][x]= Random(HASHSIZE);
			lock[WHITE][p][x]= Random(HASHSIZE);
			lock[BLACK][p][x]= Random(HASHSIZE);
		}
}
/*

Random() generates a random number up to the size of x.

*/
int Random(const int x)
{
	return rand() % x;
}
/*

AddKey updates the current key and lock.
The key is a single number representing a position.
Different positions may map to the same key.
The lock is very similar to the key (its a second key), which 

is a different number
because it was seeded with different random numbers.
While the odds of several positions having the same key are 
very high, the odds of
two positions having the same key and same lock are very very 
low.

*/
void AddKey(const int s,const int p,const int x)
{
	currentkey ^= hash[s][p][x];
	currentlock ^= lock[s][p][x];
}
/*

GetLock gets the current lock from a position.

*/
U64 GetLock()
{
	U64 loc=0;
	for(int x=0;x<64;x++)
	{
		if(board[x]!=6)
			loc ^= lock[color[x]][board[x]][x];
	}
	return loc;
}
/*

GetKey gets the current key from a position.

*/
U64 GetKey()
{
	U64 key=0;
	for(int x=0;x<64;x++)
	{
		if(board[x]!=6)
			key ^= hash[color[x]][board[x]][x];
	}
	return key;
}


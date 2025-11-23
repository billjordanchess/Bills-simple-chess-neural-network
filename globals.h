#include <memory.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dos.h>
#include <time.h>

//_CRT_SECURE_NO_WARNINGS

int LoadWeights();

#define U64 unsigned __int64 
/*
Modes the engine may use.
*/
#define NN_TRAIN 0
#define NN_PLAY 1
#define NN_EVAL 2
#define NN_OPPONENT 3
#define NN_XBOARD 4
/*
These are the names of the squares.
*/
#define A1	0
#define B1	1
#define C1	2
#define D1	3
#define E1	4
#define F1  5
#define G1	6
#define H1	7

#define A2	8
#define B2	9
#define C2	10
#define D2	11
#define E2	12
#define F2	13
#define G2	14
#define H2	15

#define A3	16
#define B3	17
#define C3	18
#define D3	19
#define E3	20
#define F3	21
#define G3	22
#define H3	23

#define A4	24
#define B4	25
#define C4	26
#define D4	27
#define E4	28
#define F4	29
#define G4	30
#define H4	31

#define A5	32
#define B5	33
#define C5	34
#define D5	35
#define E5	36
#define F5	37
#define G5	38
#define H5	39

#define A6	40
#define B6	41
#define C6	42
#define D6	43
#define E6	44
#define F6	45
#define G6	46
#define H6	47

#define A7	48
#define B7	49
#define C7	50
#define D7	51
#define E7	52
#define F7	53
#define G7	54
#define H7	55

#define A8	56
#define B8	57
#define C8	58
#define D8	59
#define E8	60
#define F8	61
#define G8	62
#define H8	63
/*
These are the directions that line pieces may move in.
*/
#define NORTH 0
#define NE 1
#define EAST 2
#define SE 3
#define SOUTH 4
#define SW 5
#define WEST 6
#define NW 7
/*
Pieces
*/
#define P 0
#define N 1
#define B 2
#define R 3
#define Q 4
#define K 5
#define EMPTY 6
/*
Colours
*/
#define WHITE 0
#define BLACK 1

#define MAX_PLY 400
#define MOVE_STACK 4000
#define GAME_STACK 4000

#define HASH_SCORE    100000000
#define CAPTURE_SCORE 10000000

/*
move_data[] 
*/
typedef struct {
	int from;
	int to;
	int promote;
	int score;
	int eval;
  } move_data;

typedef struct {
	int from;
	int to;
	int promote;
	int capture;
	int fifty;
	int castle_q[2];
	int castle_k[2];
	U64 hash;
	U64 lock;
} game;

typedef struct {
	int from;
	int to;
	int score;
  } table_move;

extern int move_start,move_dest;
extern int piece_value[6];
extern int pawn_mat[2];
extern int piece_mat[2];

extern int king_endgame[2][64];
extern int passed[2][64];

extern char piece_char[6];

extern int side,xside;
extern int fifty;
extern int ply,hply;

extern int nodes;

extern int board[64];
extern int color[64];
extern int init_color[64];
extern int init_board[64];
extern int kingloc[2];

extern int history[64][64];

extern int table_score[2] ;
extern int square_score[2][6][64];

extern const int row[64];
extern const int col[64];

extern int first_move[MAX_PLY];
extern move_data move_list[MOVE_STACK];
extern game game_list[GAME_STACK];

extern U64 currentkey,currentlock;

//extern int fixed_time;
//extern int fixed_depth;
//extern int max_depth;
//extern int max_time;
extern int start_time;
extern int stop_time;

extern int deep;

extern int qrb_moves[64][9];
extern int knight_moves[64][9];
extern int king_moves[64][9];

extern unsigned int hashpositions[2]; 
extern U64 collisions;

extern int turn;

extern const int Flip[64];
extern const int Mirror[64];
extern const int Rotate[64];
extern const int Twist[64];
extern const int TwistMirror[64];
extern const int TwistFlip[64];
extern const int TwistRotate[64];

extern int rank[2][64];
extern int OtherSide[2];
extern int ForwardSquare[2];
extern int Double[2];
extern int Left[2];
extern int Right[2];

//init.cpp
void InitBoard();
void SetTables();
void NewPosition();
void Alg(int a,int b);
void Algebraic(int a);
void SetMoves();

//search.cpp
void think(int); 
int ReCaptureSearch(int,const int);
int Sort(const int from);

//gen.cpp
void Gen();
void GenEp();
void GenCastle();
void GenPawn(const int x);
void GenKnight(const int sq);
void GenBishop(const int x,const int dir);
void GenRook(const int x,const int dir);
void GenQueen(const int x,const int dir);
void GenKing(const int x);
void AddMove(const int x,const int sq);
void GenCaptures();
void CapPawn(const int x);
void CapKnight(const int sq);
void CapBishop(const int x,const int dir);
void CapRook(const int x,const int dir);
void CapQueen(const int x,const int dir);
void CapKing(const int sq);
void AddCapture(const int x,const int sq,const int score);

//attack.cpp
bool Attack(const int s,const int sq);
int LowestAttacker(const int s,const int x);

//update.cpp
void UpdatePawn(const int s,const int p,const int start,const int dest);
void UpdatePiece(const int s,const int p,const int start,const int dest);
void RemovePiece(const int s,const int p,const int sq);
void AddPiece(const int s,const int p,const int sq);
bool MakeMove(const int,const int);
void UnMakeMove();
bool MakeRecapture(const int,const int);
void UnMakeRecapture();
int GetHistoryStart(const int n);
int GetHistoryDest(const int n);

//eval.cpp
int Eval();

//hash.cpp
void RandomizeHash();
int Random(const int x);
void AddKey(const int,const int,const int);
U64 GetKey();
U64 GetLock();

//main.cpp
int main();
int GetTime();
char *MoveString(int from,int to,int promote);
int ParseMove(char *s);
void DisplayBoard();
int Reps();

bool CanMove();


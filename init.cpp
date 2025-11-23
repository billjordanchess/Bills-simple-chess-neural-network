#include "globals.h"

int side,xside;
int fifty;
int ply,hply;

int nodes;

int board[64];
int color[64];
int kingloc[2];

int history[64][64];

int table_score[2] ;
int square_score[2][6][64];
int king_endgame[2][64];
int pawn_mat[2];
int piece_mat[2];
int passed[2][64];

int qrb_moves[64][9];
int knight_moves[64][9];
int king_moves[64][9];

move_data move_list[MOVE_STACK];
int first_move[MAX_PLY];

game game_list[GAME_STACK];

char piece_char[6] = 
{
	'P', 'N', 'B', 'R', 'Q', 'K'
};

int piece_value[6] = 
{
	100, 300, 300, 500, 900, 0
};

int init_color[64] = 
{
	WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
	WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
	EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
};

int init_board[64] = 
{
	R, N, B, Q, K, B, N, R,
	P, P, P, P, P, P, P, P,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	P, P, P, P, P, P, P, P,
	R, N, B, Q, K, B, N, R
};

const int col[64]=
{
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7
};

const int row[64]=
{
	0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7
};

const int Flip[64] = 
{
	56,  57,  58,  59,  60,  61,  62,  63,
	48,  49,  50,  51,  52,  53,  54,  55,
	40,  41,  42,  43,  44,  45,  46,  47,
	32,  33,  34,  35,  36,  37,  38,  39,
	24,  25,  26,  27,  28,  29,  30,  31,
	16,  17,  18,  19,  20,  21,  22,  23,
	8,   9,  10,  11,  12,  13,  14,  15,
	0,   1,   2,   3,   4,   5,   6,   7
};

const int Mirror[64]=
{
	7, 6, 5, 4, 3, 2, 1, 0,
	15, 14, 13, 12, 11, 10, 9, 8,
	23, 22, 21, 20, 19, 18, 17, 16,
	31, 30, 29, 28, 27, 26, 25, 24,
	39, 38, 37, 36, 35, 34, 33, 32,
	47, 46, 45, 44, 43, 42, 41, 40,
	55, 54, 53, 52, 51, 50, 49, 48,
	63, 62, 61, 60, 59, 58, 57, 56
};

const int Rotate[64]=
{
	63, 62, 61, 60, 59, 58, 57, 56,
	55, 54, 53, 52, 51, 50, 49, 48,
	47, 46, 45, 44, 43, 42, 41, 40,
	39, 38, 37, 36, 35, 34, 33, 32,
	31, 30, 29, 28, 27, 26, 25, 24,
	23, 22, 21, 20, 19, 18, 17, 16,
	15, 14, 13, 12, 11, 10, 9, 8,
	7, 6, 5, 4, 3, 2, 1, 0	
};

const int Twist[64]=
{
	0, 8, 16, 24, 32, 40, 48, 56, 
	1, 9, 17, 25, 33, 41, 49, 57,
	2, 10, 18, 26, 34, 42, 50, 58,
	3, 11, 19, 27, 35, 43, 51, 59,
	4, 12, 20, 28, 36, 44, 52, 60,
	5, 13, 21, 29, 37, 45, 53, 61,
	6, 14, 22, 30, 38, 46, 54, 62,
	7, 15, 23, 31, 39, 47, 55, 63
};

const int TwistMirror[64]=
{
	56, 48, 40, 32, 24, 16, 8, 0,  
	57, 49, 41, 33, 25, 17, 9, 1,  
	58, 50, 42, 34, 26, 18, 10, 2,  
	59, 51, 43, 35, 27, 19, 11, 3,  
	60, 52, 44, 36, 28, 20, 12, 4,  
	61, 53, 45, 37, 29, 21, 13, 5,  
	62, 54, 46, 38, 30, 22, 14, 6,  
	63, 55, 47, 39, 31, 23, 15, 7  
};

const int TwistFlip[64]=
{
	7, 15, 23, 31, 39, 47, 55, 63,  
	6, 14, 22, 30, 38, 46, 54, 62,  
	5, 13, 21, 29, 37, 45, 53, 61,  
	4, 12, 20, 28, 36, 44, 52, 60,  
	3, 11, 19, 27, 35, 43, 51, 59,  
	2, 10, 18, 26, 34, 42, 50, 58,  
	1, 9, 17, 25, 33, 41, 49, 57,  
	0, 8, 16, 24, 32, 40, 48, 56
};

const int TwistRotate[64]=
{
	63, 55, 47, 39, 31, 23, 15, 7,  
	62, 54, 46, 38, 30, 22, 14, 6,  
	61, 53, 45, 37, 29, 21, 13, 5,  
	60, 52, 44, 36, 28, 20, 12, 4,  
	59, 51, 43, 35, 27, 19, 11, 3,  
	58, 50, 42, 34, 26, 18, 10, 2,  
	57, 49, 41, 33, 25, 17, 9, 1,  
	56, 48, 40, 32, 24, 16, 8, 0  
};

int pawn_score[64] = 
{
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   2,   4, -12, -12,   4,   2,   0,
	0,   2,   4,   4,   4,   4,   2,   0,
	0,   2,   4,   8,   8,   4,   2,   0,
	0,   2,   4,   8,   8,   4,   2,   0,
	4,   8,  10,  16,  16,  10,   8,   4,
	100, 100, 100, 100, 100, 100, 100, 100,
	0,   0,   0,   0,   0,   0,   0,   0
};

int knight_score[64] = 
{
	-30, -20, -10,  -8,  -8, -10, -20, -30,
	-16, -6,   -2,   0,   0,   -2, -6, -16,
	-8,   -2,   4,   6,   6,   4,   -2, -8,
	-5,   0,   6,   8,   8,   6,   0, -5,
	-5,   0,   6,   8,   8,   6,   0, -5,
	-10,   -2,   4,   6,   6,   4,   -2, -10,
	-20, -10,   -2,   0,   0,   -2, -10, -20,
	-150, -20, -10, -5, -5, -10, -20, -150
};

int bishop_score[64] = 
{
	-10, -10, -12, -10, -10, -12, -10, -10,
	0,   4,   4,   4,   4,   4,   4,   0,
	2,   4,   6,   6,   6,   6,   4,   2,
	2,   4,   6,   8,   8,   6,   4,   2,
	2,   4,   6,   8,   8,   6,   4,   2,
	2,   4,   6,   6,   6,   6,   4,   2,
	-10,   4,   4,   4,   4,   4,   4, -10,
	-10, -10, -10, -10, -10, -10, -10, -10
};

int rook_score[64] = 
{
	4, 4,  4,  6, 6,  4, 4, 4,
	0, 0,  0,  0, 0,  0, 0, 0,
	0, 0,  0,  0, 0,  0, 0, 0,
	0, 0,  0,  0, 0,  0, 0, 0,
	0, 0,  0,  0, 0,  0, 0, 0,
	0, 0,  0,  0, 0,  0, 0, 0,
	20, 20, 20, 20, 20, 20, 20, 20,
	10, 10, 10, 10, 10, 10, 10, 10
};

int queen_score[64] = 
{
	-10, -10,  -6,  -4,  -4,  -6, -10, -10,
	-10,   2,   2,   2,   2,   2,   2, -10,
	2,   2,   2,   3,   3,   2,   2,   2,
	2,   2,   3,   4,   4,   3,   2,   2,
	2,   2,   3,   4,   4,   3,   2,   2,
	2,   2,   2,   3,   3,   2,   2,   2,
	-10,   2,   2,   2,   2,   2,   2, -10,
	-10, -10,   2,   2,   2,   2, -10, -10
};

int king_score[64] = 
{
	20,  20,  20, -40,  10, -60,  20,  20,     
	15,  20, -25, -30, -30, -45,  20,  15,   
	-48, -48, -48, -48, -48, -48, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48
};

int king_endgame_score[64] = 
{
	0,   8,  16,  18,  18,  16,  8,   0,
	8,  16,  24,  32,  32,  24,  16,  8,
	16,  24,  32,  40,  40,  32,  24,  16,
	25,  32,  40,  48,  48,  40,  32,  25,
	25,  32,  40,  48,  48,  40,  32,  25,
	16,  24,  32,  40,  40,  32,  24,  16,
	8,  16,  24,  32,  32,  24,  16,  8,
	0,  8,  16,  18,  18,  16,  8,   0
};

int passed_score[64] = 
{
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	60,  60,  60,  60  ,60, 60, 60, 60,
	30, 30, 30, 30, 30, 30, 30, 30,
	15, 15, 15, 15,15, 15, 15, 15, 
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	0, 0, 0, 0, 0, 0, 0, 0
};

int rank[2][64];
/*

SetTables fills the square_score tables, king_endgame tables and passed tables with the individual piece tables.
The value of each piece is added to the score for each square.
The board is flipped for the BLACK scores.

*/
void SetTables()
{

	for(int x=0;x<64;x++)
	{
		square_score[WHITE][P][x] = pawn_score[x];
		square_score[WHITE][N][x] = knight_score[x];
		square_score[WHITE][B][x] = bishop_score[x];
		square_score[WHITE][R][x] = rook_score[x];
		square_score[WHITE][Q][x] = queen_score[x];
		square_score[WHITE][K][x] = king_score[x];

		square_score[BLACK][P][x] = pawn_score[Flip[x]];
		square_score[BLACK][N][x] = knight_score[Flip[x]];
		square_score[BLACK][B][x] = bishop_score[Flip[x]];
		square_score[BLACK][R][x] = rook_score[Flip[x]];
		square_score[BLACK][Q][x] = queen_score[Flip[x]];
		square_score[BLACK][K][x] = king_score[Flip[x]];

		king_endgame[WHITE][x] = king_endgame_score[x] - square_score[WHITE][K][x];
		king_endgame[BLACK][x] = king_endgame_score[x] - square_score[BLACK][K][x];
		passed[WHITE][x] = passed_score[Flip[x]];
		passed[BLACK][x] = passed_score[x];

		rank[WHITE][x] = row[x];
		rank[BLACK][x] = 7 - row[x];
	}

}
/*

Sets up variables for a new game.

*/
void InitBoard()
{
	int x;

	for (x = 0; x < 64; ++x) 
	{
		color[x] = init_color[x];
		board[x] = init_board[x];
	}

	side = 0;
	xside = 1;
	fifty = 0;
	ply = 0;
	hply = 0;
	first_move[0] = 0;
	kingloc[WHITE] = E1;
	kingloc[BLACK] = E8;

	game_list[hply].castle_q[WHITE] = 1;
	game_list[hply].castle_q[BLACK] = 1;
	game_list[hply].castle_k[WHITE] = 1;
	game_list[hply].castle_k[BLACK] = 1;

	memset(history,0,sizeof(history));
}
/*

NewPosition gets the board ready before the computer starts to think.

*/
void NewPosition()
{
	piece_mat[WHITE] = pawn_mat[WHITE] = table_score[WHITE] = 0;
	piece_mat[BLACK] = pawn_mat[BLACK] = table_score[BLACK] = 0;

	for(int i=0;i<64;i++)
	{
		if(board[i] != EMPTY)
			AddPiece(color[i],board[i],i);
	}
	currentkey = GetKey();
	currentlock = GetLock();
}
/*

Alg displays a move.

*/
void Alg(int a,int b)
{
	Algebraic(a);
	Algebraic(b);
}
/*

Algebraic displays a square.
e.g. 3 becomes col[3] + 96 which is ascii character 'd' and row[3]+1 which is '1'.
Passing 3 returns 'd1'.

*/
void Algebraic(int a)
{
	if(a<0 || a>63) return; 
	char c[2]="a";
	c[0] =  96+1+col[a];
	printf("%s%d",c,row[a]+1);
}
/*

SetMoves creates the lookup tables for Knights, line-pieces and Kings.
These will later be used to generate moves, captures and attacks.

*/
void SetMoves()
{
	int k=0;
	int y;

	for(int x=0;x<64;x++)
	{
		k = 0;
		if(row[x]<6 && col[x]<7) 
			knight_moves[x][k++] =  x+17;
		if(row[x]<7 && col[x]<6) 
			knight_moves[x][k++] =  x+10;
		if(row[x]<6 && col[x]>0) 
			knight_moves[x][k++] =  x+15;
		if(row[x]<7 && col[x]>1) 
			knight_moves[x][k++] =  x+6;
		if(row[x]>1 && col[x]<7) 
			knight_moves[x][k++] =  x-15;
		if(row[x]>0 && col[x]<6) 
			knight_moves[x][k++] =  x-6;
		if(row[x]>1 && col[x]>0) 
			knight_moves[x][k++] =  x-17;
		if(row[x]>0 && col[x]>1) 
			knight_moves[x][k++] =  x-10;
		knight_moves[x][k] = -1;
	}

	for(int x=0;x<64;x++)
	{
		k = 0;

		for(int z=0;z<8;z++)
			qrb_moves[x][z] = -1;

		if(col[x]>0) qrb_moves[x][WEST]=x-1;
		if(col[x]<7) qrb_moves[x][EAST]=x+1;
		if(row[x]>0) qrb_moves[x][SOUTH]=x-8;
		if(row[x]<7) qrb_moves[x][NORTH]=x+8;
		if(col[x]<7 && row[x]<7) qrb_moves[x][NE]=x+9;
		if(col[x]>0 && row[x]<7) qrb_moves[x][NW]=x+7;
		if(col[x]>0 && row[x]>0) qrb_moves[x][SW]=x-9;
		if(col[x]<7 && row[x]>0) qrb_moves[x][SE]=x-7;

		y=0;
		if(col[x]>0) 
			king_moves[x][y++]=x-1;
		if(col[x]<7) 
			king_moves[x][y++]=x+1;
		if(row[x]>0) 
			king_moves[x][y++]=x-8;
		if(row[x]<7) 
			king_moves[x][y++]=x+8;
		if(col[x]<7 && row[x]<7) 
			king_moves[x][y++]=x+9;
		if(col[x]>0 && row[x]<7) 
			king_moves[x][y++]=x+7;
		if(col[x]>0 && row[x]>0) 
			king_moves[x][y++]=x-9;
		if(col[x]<7 && row[x]>0) 
			king_moves[x][y++]=x-7;
		king_moves[x][y] = -1;
	}
}


#include <setjmp.h>

#include "globals.h"

int NN_Play(int);
int NN_Train(int,float);
int NN_Opponent(int s);

float CaptureSearch();

int Reps2();
bool IsMate();

extern int move_start, move_dest;

move_data root_move;

const int FUTILITY_SCORE = 900;//The value of a queen.

void z()
{
	DisplayBoard();
	_getch();
}

bool CanMove();

float GetTarget();

/*
think() iterates until the maximum depth for the move is reached or until the allotted time
has run out. Maximum depth is only one for training.
<p>
After each iteration the principal variation is then displayed.
*/

void think(int nn)
{
	int from, to, flags;
	float score = -10000;
	int alpha = -10000;
	int beta = 10000;
	int bestscore = -10000;
	int count = 0;
	int firstmove = 0;
	float SEE = 0;
	float mat = 0;
	float eval = 0;

	ply = 0;
	Gen();
	int lastmove = first_move[1];

	for (int i = firstmove; i < lastmove; i++)
	{
		from = move_list[i].from;
		to = move_list[i].to;

		if (!MakeMove(from, to))
		{
			continue;
		}
		if (Reps2())
		{
			if (count > 0)
			{
				UnMakeMove();
				continue;
			}
		}
		if (fifty >50)
		{
			UnMakeMove();
			score = 0;
			if (bestscore < 0)
			{
				bestscore = 0;
				root_move = move_list[i];
			}				
			continue;
		}
		
		count++;
		if (Attack(xside, kingloc[side]))
		{
			if (IsMate())
			{
				UnMakeMove();
				root_move = move_list[i];
				break;
			}
		}
		else
		{
			if (piece_mat[side] == 0)
			{
				if (!CanMove())
				{
					UnMakeMove();
					score = 0;
					continue;
				}
			}
		}
		if (nn == NN_PLAY)
		{
			mat = piece_mat[side] + pawn_mat[side] - piece_mat[xside] - pawn_mat[xside];
			SEE = -CaptureSearch();
			eval = NN_Play(side);
		}
		else if (nn == NN_OPPONENT)
		{
			mat = piece_mat[side] + pawn_mat[side] - piece_mat[xside] - pawn_mat[xside];
			SEE = -CaptureSearch();
			eval = NN_Opponent(side);
		}
		else if (nn == NN_TRAIN)
		{
			float target = -GetTarget();
			eval = NN_Train(side, target);
			mat = 0;
			SEE = 0;
		}
		score = mat + SEE + eval;

		UnMakeMove();

		if (score > bestscore)
		{
			bestscore = score;
			root_move = move_list[i];
			if (score > 9900)
				break;
		}
	}
	move_start = root_move.from;
	move_dest = root_move.to;
}

float GetTarget()
{
	int mat = piece_mat[side] + pawn_mat[side] - piece_mat[xside] - pawn_mat[xside];
	int SEE =  - CaptureSearch();
	int eval = Eval();
	float score = mat + SEE + eval;
	return score;
}
/*

CaptureSearch evaluates the position. If the position is more than a queen less than
alpha (the best score that side can do) it doesn't search.
It generates all captures and does a recapture search to see if material is won.
If so, the material gain is added to the score.

*/
float CaptureSearch()
{
	nodes++;

	int score = 0;
	int best = 0;
	int from, to;

	GenCaptures();

	for (int i = first_move[ply]; i < first_move[ply + 1]; ++i)
	{
		Sort(i);
		from = move_list[i].from;
		to = move_list[i].to;

		if (best >= piece_value[board[to]])
		{
			return best;
		}

		score = ReCaptureSearch(from, to);

		if (score > best)
		{
			best = score;
		}
	}
	return best;
}
/*

ReCaptureSearch searches the outcome of capturing and recapturing on the same square.
It stops searching if the value of the capturing piece is more than that of the
recaptured piece and the next attacker. For example, a WHITE queen could take a rook, but a
bishop could take the queen. Even if WHITE could take the bishop, its not worth exchanging a
queen for rook and bishop.

*/
int ReCaptureSearch(int a, const int sq)
{
	int b;
	int c = 0;
	int t = 0;
	int score[12];

	score[WHITE] = piece_value[board[sq]];
	score[BLACK] = piece_value[board[a]];

	int total_score = 0;

	while (c < 10)
	{
		if (!MakeRecapture(a, sq))
			break;
		t++;
		nodes++;
		c++;

		b = LowestAttacker(side, sq);

		if (b > -1)
		{
			score[c + 1] = piece_value[board[b]];
			if (score[c] > score[c - 1] + score[c + 1])
			{
				c--;
				break;
			}
		}
		else
		{
			break;
		}
		a = b;
	}

	while (c > 1)
	{
		if (score[c - 1] >= score[c - 2])
			c -= 2;
		else
			break;
	}

	for (int x = 0; x < c; x++)
	{
		if (x % 2 == 0)
			total_score += score[x];
		else
			total_score -= score[x];
	}

	while (t)
	{
		UnMakeRecapture();
		t--;
	}

	if (total_score > score[WHITE])
		return score[WHITE];

	if (total_score < score[WHITE] - score[BLACK])
		return score[WHITE] - score[BLACK];

	return total_score;
}
/*

Reps2() searches backwards for an identical position.
A positions are identical if the key and lock are the same.
'fifty' represents the number of moves made since the last pawn move or captured.

*/
int Reps2()
{
	for (int i = hply - 2; i >= hply - fifty; i -= 2)
	{
		if (game_list[i].hash == currentkey)
		{
			return 1;
		}
	}
	return 0;
}
/*
bestindex
Sort searches the move list for the move with the highest score.
It is moved to the top of the list so that it will be played next.

*/
int Sort(const int from)
{
	move_data bestmove;

	int bestscore = move_list[from].score;
	int bestindex = from;
	for (int i = from + 1; i < first_move[ply + 1]; ++i)
		if (move_list[i].score > bestscore)
		{
			bestscore = move_list[i].score;
			bestindex = i;
		}

	bestmove = move_list[from];
	move_list[from] = move_list[bestindex];
	move_list[bestindex] = bestmove;

	return move_list[bestindex].score;
}

bool IsMate()
{
	Gen();
	int firstmove = first_move[1];
	int lastmove = first_move[2];
	int from, to;

	for (int i = firstmove; i < lastmove; i++)
	{
		Sort(i);
		from = move_list[i].from;
		to = move_list[i].to;

		if (!MakeMove(from, to))
		{
			continue;
		}
		UnMakeMove();
		return false;
	}
	return true;
}


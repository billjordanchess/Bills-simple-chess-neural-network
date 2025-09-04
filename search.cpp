#include <setjmp.h>

#include "globals.h"

int NN_Play(int);
int NN_Train(int,float);
int NN_Match(int s);
int NN_Opponent(int s);

float Search(float alpha, float beta, int depth, int nn);
float CaptureSearch(float alpha,float beta,int nn);	

extern int move_start,move_dest;

int Reps2();
int Book();

move_data root_move;

const int FUTILITY_SCORE = 900;//The value of a queen.

/*
think() iterates until the maximum depth for the move is reached or until the allotted time
has run out. Maximum depth is only one for training.
<p>
After each iteration the principal variation is then displayed.
*/
void think(int nn)
{
	if(nn == NN_XBOARD)
	{
		if (hply < 8)
		{
			int bookflag = Book();
			if (bookflag > 0)
			{
				return;
			}
		}
	}

	int score;
	ply = 0;
	nodes = 0;

	if(nn != NN_XBOARD)
		max_depth = 1;

	for (int i = 1; i <= max_depth; ++i) 
	{
		score = Search(-10000.0,10000.0,i,nn);
		if(nn == NN_XBOARD)
		{
			printf("%d %d %d %d\n", i, score, (GetTime() - start_time) / 10, nodes);
			fflush(stdout);
		}
		if (score > 9000 || score < -9000)
		{
			break;
		}
	}

}

float Search(float alpha, float beta, int depth, int nn)
{
	if (ply > 0 && Reps2() )
	{
		return 0;
	}
	if(fifty > 98 && depth < 1)
	{
		return 0;
	}
	if (depth < 1)
		return CaptureSearch(alpha,beta,nn);
	nodes++;

	move_data bestmove;
	float bestscore = -10001;
	float score;

	int check = 0;

	if (Attack(xside,kingloc[side])) 
	{
		check = 1;
	}
	Gen();

	int c = 0;
	int d;


	for (int i = first_move[ply]; i < first_move[ply + 1]; i++) 
	{
		Sort(i);

		if (!MakeMove(move_list[i].start,move_list[i].dest))
		{
			continue;
		}
		c++;

		if (Attack(xside,kingloc[side])) 
		{
			d = depth;
		}
		else
		{
			d = depth - 3;
			if(move_list[i].score > CAPTURE_SCORE || c==1 || check==1)
			{
				d = depth - 1;
			}
			else if(move_list[i].score > 0) 
			{
				d = depth - 2;
			}
		} 

		score = -Search(-beta, -alpha, d, nn);

		TakeBack();

		if(score > bestscore)
		{
			bestscore = score;
			bestmove = move_list[i];
			if(ply==0)
			{
				move_start = bestmove.start;
				move_dest = bestmove.dest;
			}
		}
		if (score > alpha) 
		{
			if (score >= beta)
			{
				if(board[move_list[i].dest]==EMPTY)	  
					history[move_list[i].start][move_list[i].dest] += depth;
				return beta;
			}
			alpha = score;
		}
	}
	if (c == 0) 
	{
		if (Attack(xside,kingloc[side])) 
		{
			return -10000 + ply;
		}
		else
			return 0;
	}
	return alpha;
}
/*

CaptureSearch evaluates the position. If the position is more than a queen less than
alpha (the best score that side can do) it doesn't search.
It generates all captures and does a recapture search to see if material is won.
If so, the material gain is added to the score.

*/

float CaptureSearch(float alpha,float beta,int nn)
{
	nodes++;

	if(piece_mat[side]==0)
	{	
		if(!CanMove())
		{
			return 0;
		}
	}	

	int eval;

	float target;
	if (nn == NN_TRAIN) 
	{
		target = Eval();           
		eval = NN_Train(side, target);
	} 
	else if (nn == NN_EVAL) 
	{
		eval = Eval();                 
	} 
	else if (nn == NN_PLAY || nn == NN_XBOARD)
	{  
		eval = NN_Play(side);          
	} 
	else if (nn == NN_MATCH)
	{  
		eval = NN_Match(side);         
	} 
	else if (nn == NN_OPPONENT)
	{  
		eval = NN_Opponent(side);         
	} 
	eval += piece_mat[side] + pawn_mat[side] - piece_mat[xside] - pawn_mat[xside];

	if (eval > alpha)
	{
		if(eval >= beta)
		{	
			return beta;
		}
		alpha = eval;
	}
	else if(eval + FUTILITY_SCORE < alpha)
		return alpha;

	int score = 0;
	int best = 0;

	GenCaptures();

	for (int i = first_move[ply]; i < first_move[ply + 1]; ++i) 
	{
		Sort(i);

		if(eval + piece_value[board[move_list[i].dest]] < alpha)
		{
			continue;
		}

		score = ReCaptureSearch(move_list[i].start, move_list[i].dest);

		if(score>best)
		{
			best = score;
		}
	}

	if(best>0)
	{
		eval += best;
	}
	/*
	else if (nn == NN_TRAIN) 
	{
	target = Eval();           
	eval = NN_Train(side, target);
	eval += piece_mat[side] + pawn_mat[side] - piece_mat[xside] - pawn_mat[xside];
	} 
	*/

	return eval;
}
/*

ReCaptureSearch searches the outcome of capturing and recapturing on the same square.
It stops searching if the value of the capturing piece is more than that of the
recaptured piece and the next attacker. For example, a White queen could take a rook, but a
bishop could take the queen. Even if White could take the bishop, its not worth exchanging a
queen for rook and bishop.

*/
int ReCaptureSearch(int a,const int sq)
{				
	int b;
	int c = 0;
	int t = 0;
	int score[12];

	memset(score,0,sizeof(score));
	score[WHITE] = piece_value[board[sq]]; 
	score[BLACK] = piece_value[board[a]];

	int total_score = 0;

	while(c < 10)
	{
		if(!MakeRecapture(a,sq))
			break;
		t++;
		nodes++;
		c++;

		b = LowestAttacker(side,sq);

		if(b>-1)
		{
			score[c + 1] = piece_value[board[b]]; 
			if(score[c] > score[c - 1] + score[c + 1])
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

	while(c>1)
	{
		if(score[c-1] >= score[c-2])
			c -= 2;
		else
			break;
	}

	for(int x=0; x < c; x++)
	{
		if(x%2 == 0)
			total_score += score[x];
		else
			total_score -= score[x];
	}

	while(t)
	{
		UnMakeRecapture();
		t--;
	}

	if(total_score > score[WHITE])
		return score[WHITE];

	if(total_score < score[WHITE] - score[BLACK])
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
	for (int i = hply-2; i >= hply-fifty; i-=2)
	{
		if (game_list[i].hash == currentkey && game_list[i].lock == currentlock)
		{
			return 1;
		}
	}
	return 0;
}
/*

Sort searches the move list for the move with the highest score.
It is moved to the top of the list so that it will be played next.

*/
int Sort(const int from)
{
	move_data g;

	int bs = move_list[from].score;
	int bi = from;
	for (int i = from + 1; i < first_move[ply + 1]; ++i)
		if (move_list[i].score > bs) 
		{
			bs = move_list[i].score;
			bi = i;
		}

		g = move_list[from];
		move_list[from] = move_list[bi];
		move_list[bi] = g;

		return move_list[bi].score;
}

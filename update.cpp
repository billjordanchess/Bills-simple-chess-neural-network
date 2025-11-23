#include "globals.h"

int ReverseSquare[2] = {-8,8};

game* g;

/*

UpdatePiece updates the Hash Table key, the board and the table_score (the incremental
evaluation) whenever a piece moves.

*/
void UpdatePiece(const int s,const int piece,const int from,const int to)
{
	AddKey(s,piece,from);
	AddKey(s,piece,to);
	board[to]=piece;
	color[to]=s;
	board[from]=EMPTY;
	color[from]=EMPTY;
	if(piece==K)
		kingloc[s] = to;
}
/*

RemovePiece updates the Hash Table key, the board and the table_score (the incremental
evaluation) whenever a piece is removed.

*/
void RemovePiece(const int s,const int piece,const int sq)
{
	AddKey(s,piece,sq);
	board[sq]=EMPTY;
	color[sq]=EMPTY;
	if(piece==P)
		pawn_mat[s] -= 100;
	else
		piece_mat[s] -= piece_value[piece];
}
/*

AddPiece updates the Hash Table key, the board and the table_score (the incremental
evaluation) whenever a piece is added.

*/
void AddPiece(const int s,const int piece,const int sq)
{
	AddKey(s,piece,sq);
	board[sq]=piece;
	color[sq]=s;
	if(piece==P)
		pawn_mat[s] += 100;
	else
		piece_mat[s] += piece_value[piece];
}
/*

MakeMove updates the board whenever a move is made.
If the King moves two squares then it sees if castling is legal.
If a pawn moves and changes file s without making a capture, then its an en passant capture
and the captured pawn is removed.
<p>
If the move is a capture then the captured piece is removed from the board.
If castling permissions are effected then they are updated.
If a pawn moves to the last rank then its promoted. The pawn is removed and a queen is added.
If the move leaves the King in check (for example, if a pinned piece moved), then the move is taken back.

*/
bool MakeMove(const int from,const int to)
{	
	if (abs(from - to) ==2 && board[from] == K)
	{
		if (Attack(xside,from)) 
			return false;
		if(to==G1)
		{
			if (Attack(xside,F1)) 
				return false;
			UpdatePiece(side,R,H1,F1);
		}
		else if(to==C1)
		{
			if (Attack(xside,D1)) 
				return false;
			UpdatePiece(side,R,A1,D1);
		}
		else if(to==G8)
		{
			if (Attack(xside,F8)) 
				return false;
			UpdatePiece(side,R,H8,F8);
		}
		else if(to==C8)
		{	
			if (Attack(xside,D8)) 
				return false;
			UpdatePiece(side,R,A8,D8);
		}
	}

	g = &game_list[hply];
	g->from = from;
	g->to = to;
	g->capture = board[to];
	g->fifty = fifty;
	g->hash = currentkey;
	g->lock = currentlock;

	ply++;
	hply++;

	g = &game_list[hply];
	g->castle_q[WHITE] = game_list[hply-1].castle_q[WHITE];
	g->castle_q[BLACK] = game_list[hply-1].castle_q[BLACK];
	g->castle_k[WHITE] = game_list[hply-1].castle_k[WHITE];
	g->castle_k[BLACK] = game_list[hply-1].castle_k[BLACK];

	fifty++;

	if (board[from] == P)
	{
		fifty = 0;
		if (board[to] == EMPTY && col[from] != col[to])
		{
			RemovePiece(xside,P,to + ReverseSquare[side]);
		}
	}

	if(board[to] != EMPTY)
	{
		fifty = 0;
		RemovePiece(xside,board[to],to);
	}

	if (board[from]==P && (row[to]==0 || row[to]==7))//promotion
	{
		RemovePiece(side,P,from);
		AddPiece(side,Q,to);
		g->promote = Q;
	}
	else
	{
		g->promote = 0;
		UpdatePiece(side,board[from],from,to);
	}

	if(to == A1 || from == A1) 
		g->castle_q[WHITE] = 0;
	else if(to == H1 || from == H1) 
		g->castle_k[WHITE] = 0;
	else if(from == E1)
	{
		g->castle_q[WHITE] = 0;
		g->castle_k[WHITE] = 0;
	}

	if(to == A8 || from == A8) 
		g->castle_q[BLACK] = 0;
	else if(to == H8 || from == H8) 
		g->castle_k[BLACK] = 0;
	else if(from == E8)
	{
		g->castle_q[BLACK] = 0;
		g->castle_k[BLACK] = 0;
	}

	side ^= 1;
	xside ^= 1;
	if (Attack(side,kingloc[xside])) 
	{
		UnMakeMove();
		return false;
	}
	return true;
}
/*

UnMakeMove is the opposite of MakeMove.

*/
void UnMakeMove()
{	
	side ^= 1;
	xside ^= 1;
	ply--;
	hply--;

	game* m = &game_list[hply];
	int from = m->from;
	int to = m->to;

	fifty = m->fifty;

	if (board[to]==P && m->capture == EMPTY && col[from] != col[to])
	{
		AddPiece(xside,P,to + ReverseSquare[side]);
	}
	if(game_list[hply+1].promote == Q)
	{
		AddPiece(side,P,from);
		RemovePiece(side,board[to],to);
	}
	else
	{
		UpdatePiece(side,board[to],to,from);
	}
	if (m->capture != EMPTY)
	{
		AddPiece(xside,m->capture,to);
	}
	if (abs(from - to) == 2 && board[from] == K)
	{
		if(to==G1)
			UpdatePiece(side,R,F1,H1);
		else if(to==C1)
			UpdatePiece(side,R,D1,A1);
		else if(to==G8)
			UpdatePiece(side,R,F8,H8);
		else if(to==C8)
			UpdatePiece(side,R,D8,A8);
	}
}
/*

MakeRecapture is simpler than MakeMove because there is no castling involved and it
doesn't include en passant capture and promotion.
It the capture is illegal it is taken back.

*/
bool MakeRecapture(const int from,const int to)
{	 
	game_list[hply].from = from;
	game_list[hply].to = to;
	game_list[hply].capture = board[to];
	ply++;
	hply++;

	board[to] = board[from];
	color[to] = color[from];
	board[from] = EMPTY;
	color[from] = EMPTY;

	if(board[to]==K)
		kingloc[side] = to;

	side ^= 1;
	xside ^= 1;
	if (Attack(side,kingloc[xside])) 
	{
		UnMakeRecapture();
		return false;
	}
	return true;
}
/*

UnMakeRecapture is very similar to MakeRecapture.

*/
void UnMakeRecapture()
{
	side ^= 1;
	xside ^= 1;
	ply--;
	hply--;

	int from = game_list[hply].from;
	int to = game_list[hply].to;

	board[from] = board[to];
	color[from] = color[to];
	board[to] = game_list[hply].capture;
	color[to] = xside;

	if(board[from]==K)
		kingloc[side] = from;
}
/*

GetHistoryStart returns the from square for the move in the game list.

*/
int GetHistoryStart(const int n)
{
	return game_list[n].from;
}
/*

GetHistoryDest returns the to square for the move in the game list.

*/
int GetHistoryDest(const int n)
{
	return game_list[n].to;
}


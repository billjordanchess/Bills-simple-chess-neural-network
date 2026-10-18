#include "globals.h"

int px[6] = { 0,10,20,30,40,0 };
int nx[6] = { -3,7,17,27,37,0 };
int bx[6] = { -3,7,17,27,37,0 };
int rx[6] = { -5,5,15,25,35,0 };
int qx[6] = { -9,1,11,21,31,0 };
int kx[6] = { 0,10,20,30,40,0 };

int ForwardSquare[2] = { 8,-8 };
int Double[2] = { 16,-16 };
int Left[2] = { 7,-9 };
int Right[2] = { 9,-7 };
int OtherSide[2] = { 1,0 };

int move_count;

extern int rank2[2][64];

void Gen();

void Gen()
{
    first_move[ply + 1] = first_move[ply];
    move_count = first_move[ply];

    GenEp();

    GenCastle();

    for (int x = 0; x < 64; x++)
    {
        if (color[x] == side)
        {
            switch (board[x])
            {
            case P:
                GenPawn(x);
                break;
            case N:
                GenKnight(x);
                break;
            case B:
                GenBishop(x, NE);
                GenBishop(x, SE);
                GenBishop(x, SW);
                GenBishop(x, NW);
                break;
            case R:
                GenRook(x, NORTH);
                GenRook(x, EAST);
                GenRook(x, SOUTH);
                GenRook(x, WEST);
                break;
            case Q:
                GenQueen(x, NE);
                GenQueen(x, SE);
                GenQueen(x, SW);
                GenQueen(x, NW);
                GenQueen(x, NORTH);
                GenQueen(x, EAST);
                GenQueen(x, SOUTH);
                GenQueen(x, WEST);
                break;
            case K:
                GenKing(x);
                break;
            default:
                break;
            }
        }
    }
    first_move[ply + 1] = move_count;
}

void GenEp()
{
    if (hply == 0)
        return;
    int ep = GetHistoryTo(hply - 1);

    if (board[ep] == P && abs(GetHistoryFrom(hply - 1) - ep) == 16)
    {
        if (col[ep] > 0 && color[ep - 1] == side && board[ep - 1] == P)
        {
            AddCapture(ep - 1, ep + ForwardSquare[side], px[P]);
        }
        if (col[ep] < 7 && color[ep + 1] == side && board[ep + 1] == P)
        {
            AddCapture(ep + 1, ep + ForwardSquare[side], px[P]);
        }
    }
}

void GenCastle()
{
    if (side == WHITE)
    {
        if (game_list[hply].castle_k[side])
        {
            if (board[F1] == EMPTY && board[G1] == EMPTY)
            {
                AddMove(E1, G1);
            }
        }
        if (game_list[hply].castle_q[side])
        {
            if (board[B1] == EMPTY && board[C1] == EMPTY && board[D1] == EMPTY)
            {
                AddMove(E1, C1);
            }
        }
    }
    else
    {
        if (game_list[hply].castle_k[side])
        {
            if (board[F8] == EMPTY && board[G8] == EMPTY)
            {
                AddMove(E8, G8);
            }
        }
        if (game_list[hply].castle_q[side])
        {
            if (board[B8] == EMPTY && board[C8] == EMPTY && board[D8] == EMPTY)
            {
                AddMove(E8, C8);
            }
        }
    }
}

void GenPawn(const int from)
{
    if (board[from + ForwardSquare[side]] == EMPTY)
    {
        AddMove(from, from + ForwardSquare[side]);
        if (rank2[side][from] == 1 && board[from + Double[side]] == EMPTY)
            AddMove(from, from + Double[side]);
    }
    if (col[from] > 0 && color[from + Left[side]] == OtherSide[side])
        AddCapture(from, from + Left[side], px[board[from + Left[side]]]);
    if (col[from] < 7 && color[from + Right[side]] == OtherSide[side])
        AddCapture(from, from + Right[side], px[board[from + Right[side]]]);
}

void GenKnight(const int from)
{
    int k = 0;
    int to = knight_moves[from][k++];
    while (to > -1)
    {
        if (color[to] == EMPTY)
            AddMove(from, to);
        else if (color[to] == xside)
            AddCapture(from, to, nx[board[to]]);
        to = knight_moves[from][k++];
    }
}

void GenBishop(const int from, const int dir)
{
    int to = qrb_moves[from][dir];
    while (to > -1)
    {
        if (color[to] != EMPTY)
        {
            if (color[to] == xside)
                AddCapture(from, to, bx[board[to]]);
            break;
        }
        AddMove(from, to);
        to = qrb_moves[to][dir];
    }
}

void GenRook(const int from, const int dir)
{
    int to = qrb_moves[from][dir];
    while (to > -1)
    {
        if (color[to] != EMPTY)
        {
            if (color[to] == xside)
                AddCapture(from, to, rx[board[to]]);
            break;
        }
        AddMove(from, to);
        to = qrb_moves[to][dir];
    }
}

void GenQueen(const int from, const int dir)
{
    int to = qrb_moves[from][dir];
    while (to > -1)
    {
        if (color[to] != EMPTY)
        {
            if (color[to] == xside)
                AddCapture(from, to, qx[board[to]]);
            break;
        }
        AddMove(from, to);
        to = qrb_moves[to][dir];
    }
}

void GenKing(const int from)
{
    int k = 0;
    int to = king_moves[from][k++];

    while (to > -1)
    {
        if (color[to] == EMPTY)
            AddMove(from, to);
        else if (color[to] == xside)
            AddCapture(from, to, kx[board[to]]);
        to = king_moves[from][k++];
    }
}

void AddMove(const int from, const int to)
{
    move_list[move_count].from = from;
    move_list[move_count].to = to;
    move_list[move_count].score = history[from][to];
    move_count++;
}

void AddCapture(const int from, const int to, const int score)
{
    move_list[move_count].from = from;
    move_list[move_count].to = to;
    move_list[move_count].score = score + CAPTURE_SCORE;
    move_count++;
}

void GenCaptures()
{
    first_move[ply + 1] = first_move[ply];
    move_count = first_move[ply];

    for (int x = 0; x < 64; x++)
    {
        if (color[x] == side)
        {
            switch (board[x])
            {
            case P:
                CapPawn(x);
                break;
            case N:
                CapKnight(x);
                break;
            case B:
                CapBishop(x, NE);
                CapBishop(x, SE);
                CapBishop(x, SW);
                CapBishop(x, NW);
                break;
            case R:
                CapRook(x, EAST);
                CapRook(x, SOUTH);
                CapRook(x, WEST);
                CapRook(x, NORTH);
                break;
            case Q:
                CapQueen(x, NE);
                CapQueen(x, SE);
                CapQueen(x, SW);
                CapQueen(x, NW);
                CapQueen(x, EAST);
                CapQueen(x, SOUTH);
                CapQueen(x, WEST);
                CapQueen(x, NORTH);
                break;
            case K:
                CapKing(x);
                break;
            default:
                break;
            }
        }
    }
    first_move[ply + 1] = move_count;
}

void CapPawn(const int from)
{
    if (col[from] > 0 && color[from + Left[side]] == OtherSide[side])
        AddCapture(from, from + Left[side], px[board[from + Left[side]]]);
    if (col[from] < 7 && color[from + Right[side]] == OtherSide[side])
        AddCapture(from, from + Right[side], px[board[from + Right[side]]]);
}

void CapKnight(const int from)
{
    int k = 0;
    int to = knight_moves[from][k++];
    while (to > -1)
    {
        if (color[to] == xside)
            AddCapture(from, to, nx[board[to]]);
        to = knight_moves[from][k++];
    }
}

void CapBishop(const int from, const int dir)
{
    int to = qrb_moves[from][dir];
    while (to > -1)
    {
        if (color[to] != EMPTY)
        {
            if (color[to] == xside)
                AddCapture(from, to, bx[board[to]]);
            break;
        }
        to = qrb_moves[to][dir];
    }
}

void CapRook(const int from, const int dir)
{
    int to = qrb_moves[from][dir];
    while (to > -1)
    {
        if (color[to] != EMPTY)
        {
            if (color[to] == xside)
                AddCapture(from, to, rx[board[to]]);
            break;
        }
        to = qrb_moves[to][dir];
    }
}

void CapQueen(const int from, const int dir)
{
    int to = qrb_moves[from][dir];
    while (to > -1)
    {
        if (color[to] != EMPTY)
        {
            if (color[to] == xside)
                AddCapture(from, to, qx[board[to]]);
            break;
        }
        to = qrb_moves[to][dir];
    }
}

void CapKing(const int from)
{
    int k = 0;
    int to = king_moves[from][k++];

    while (to > -1)
    {
        if (color[to] == xside)
            AddCapture(from, to, kx[board[to]]);
        to = king_moves[from][k++];
    }
}

bool CanMove()
{
    int k = 0;
    int sq = king_moves[kingloc[side]][k++];

    while (sq > -1)
    {
        if (color[sq] != side)
        {
            if (!Attack(xside, sq))
                return true;
        }
        sq = king_moves[kingloc[side]][k++];
    }
    for (int x = 0; x < 64; x++)
    {
        if (color[x] == side && board[x] == P)
        {
            if (board[x + ForwardSquare[side]] == EMPTY)
            {
                return true;
            }
            if (col[x] > 0 && color[x + Left[side]] == OtherSide[side])
                return true;
            if (col[x] < 7 && color[x + Right[side]] == OtherSide[side])
                return true;
        }
    }
    return false;
}

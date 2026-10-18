#include "globals.h"

int LineCheck(const int s, const int sq, const int d, const int p);
bool LineCheck2(const int s, const int sq, const int d, const int p1, const int p2);

bool Attack(const int s, const int to)
{
    if (s == WHITE)
    {
        if (row[to] > 1)
        {
            if (col[to] < 7 && color[to - 7] == s && board[to - 7] == P)
            {
                return true;
            }
            if (col[to] > 0 && color[to - 9] == s && board[to - 9] == P)
            {
                return true;
            }
        }
    }
    else
    {
        if (row[to] < 6)
        {
            if (col[to] > 0 && color[to + 7] == s && board[to + 7] == P)
            {
                return true;
            }
            if (col[to] < 7 && color[to + 9] == s && board[to + 9] == P)
            {
                return true;
            }
        }
    }

    int k = 0;
    int from = knight_moves[to][k];

    while (from > -1)
    {
        if (color[from] == s && board[from] == N)
            return true;
        k++;
        from = knight_moves[to][k];
    }

    if (LineCheck2(s, to, NE, B, Q)) return true;
    if (LineCheck2(s, to, NW, B, Q)) return true;
    if (LineCheck2(s, to, SW, B, Q)) return true;
    if (LineCheck2(s, to, SE, B, Q)) return true;

    if (LineCheck2(s, to, NORTH, R, Q)) return true;
    if (LineCheck2(s, to, SOUTH, R, Q)) return true;
    if (LineCheck2(s, to, EAST, R, Q)) return true;
    if (LineCheck2(s, to, WEST, R, Q)) return true;

    if (abs(col[to] - col[kingloc[s]]) < 2 && abs(row[to] - row[kingloc[s]]) < 2)
        return true;
    return false;
}

int LowestAttacker(const int s, const int to)
{
    if (s == WHITE)
    {
        if (row[to] > 1)
        {
            if (col[to] < 7 && color[to - 7] == s && board[to - 7] == P)
                return to - 7;
            if (col[to] > 0 && color[to - 9] == s && board[to - 9] == P)
                return to - 9;
        }
    }
    else
    {
        if (row[to] < 6)
        {
            if (col[to] > 0 && color[to + 7] == s && board[to + 7] == P)
                return to + 7;
            if (col[to] < 7 && color[to + 9] == s && board[to + 9] == P)
                return to + 9;
        }
    }

    int k = 0;
    int from = knight_moves[to][k];

    while (from > -1)
    {
        if (color[from] == s && board[from] == N)
            return from;
        k++;
        from = knight_moves[to][k];
    }

    from = LineCheck(s, to, NE, B); if (from > -1) return from;
    from = LineCheck(s, to, NW, B); if (from > -1) return from;
    from = LineCheck(s, to, SW, B); if (from > -1) return from;
    from = LineCheck(s, to, SE, B); if (from > -1) return from;

    from = LineCheck(s, to, NORTH, R); if (from > -1) return from;
    from = LineCheck(s, to, SOUTH, R); if (from > -1) return from;
    from = LineCheck(s, to, EAST, R); if (from > -1) return from;
    from = LineCheck(s, to, WEST, R); if (from > -1) return from;

    from = LineCheck(s, to, NORTH, Q); if (from > -1) return from;
    from = LineCheck(s, to, SOUTH, Q); if (from > -1) return from;
    from = LineCheck(s, to, EAST, Q); if (from > -1) return from;
    from = LineCheck(s, to, WEST, Q); if (from > -1) return from;

    from = LineCheck(s, to, NE, Q); if (from > -1) return from;
    from = LineCheck(s, to, NW, Q); if (from > -1) return from;
    from = LineCheck(s, to, SW, Q); if (from > -1) return from;
    from = LineCheck(s, to, SE, Q); if (from > -1) return from;

    if (abs(col[to] - col[kingloc[s]]) < 2 && abs(row[to] - row[kingloc[s]]) < 2)
    {
        return kingloc[s];
    }
    return -1;
}

int LineCheck(const int s, int sq, const int d, const int p)
{
    sq = qrb_moves[sq][d];
    while (sq > -1)
    {
        if (color[sq] != EMPTY)
        {
            if (board[sq] == p && color[sq] == s)
                return sq;
            break;
        }
        sq = qrb_moves[sq][d];
    }
    return -1;
}

bool LineCheck2(const int s, int sq, const int d, const int p1, const int p2)
{
    sq = qrb_moves[sq][d];
    while (sq > -1)
    {
        if (color[sq] != EMPTY)
        {
            if ((board[sq] == p1 || board[sq] == p2) && color[sq] == s)
                return true;
            break;
        }
        sq = qrb_moves[sq][d];
    }
    return false;
}


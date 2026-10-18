#include "globals.h"//18/10

const int ISOLATED = 20;

int EvalPawn(const int x);
int EvalRook(const int s, const int x);
bool Pawns(const int s, const int file);
bool Pawns2(const int s, const int xs, const int start);

int queenside_pawns[2], kingside_pawns[2];

const int queenside_defence[2][64] =
{
{
    0, 0, 0, 0, 0, 0, 0, 0,
    8,10, 8, 0, 0, 0, 0, 0,
    8, 6, 8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
},
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    8, 6, 8, 0, 0, 0, 0, 0,
    8,10, 8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
} };

const int kingside_defence[2][64] =
{
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 8,10, 8,
    0, 0, 0, 0, 0, 8, 6, 8,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
},
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 8, 6, 8,
    0, 0, 0, 0, 0, 8,10, 8,
    0, 0, 0, 0, 0, 0, 0, 0
} };

int Eval()
{
    int score[2] = { 0,0 };
    int queens[2] = { 0,0 };

    queenside_pawns[WHITE] = 0;
    queenside_pawns[BLACK] = 0;
    kingside_pawns[WHITE] = 0;
    kingside_pawns[BLACK] = 0;
    
    for (int x = 0; x < 64; x++)
    {
        if (color[x] != EMPTY)
        {
            score[color[x]] += square_score[color[x]][board[x]][x];
            if (board[x] == P)
            {
                score[color[x]] += EvalPawn(x);
            }
            else if (board[x] == R)
            {
                score[color[x]] += EvalRook(color[x], x);
            }
            else if (board[x] == Q)
            {
                queens[color[x]] = 1;
            }
        }
    }
    for (int s = WHITE; s <= BLACK; s++)
    {
        int xs = s ^ 1;
        if (queens[xs] == 0)
        {
            if (pawn_mat[s] == 0 && pawn_mat[xs] == 0 && piece_mat[s] <= 300 && piece_mat[xs] <= 300)
            {
                return 0;
            }
            score[WHITE] += king_endgame[s][kingloc[s]];
        }
        else
        {
            if (col[kingloc[s]] > 3)
                score[s] += kingside_pawns[s];
            else
                score[s] += queenside_pawns[s];
        }
    }
    return score[side] - score[xside];
}

int EvalPawn(const int x)
{
    int score = 0;
    int s = color[x];
    int xs = OtherSide[s];

    if (!Pawns2(s, xs, x))
    {
        score += passed[s][x];
    }
    if ((col[x] == 0 || !Pawns(s, col[x] - 1)) && (col[x] == 7 || !Pawns(s, col[x] + 1)))
        score -= ISOLATED;
    kingside_pawns[s] += kingside_defence[s][x];
    queenside_pawns[s] += queenside_defence[s][x];

    return score;
}

int EvalRook(const int s, const int x)
{
    int score = 0;
    if (!Pawns(s, col[x]))
    {
        score = 10;
        if (!Pawns(OtherSide[s], col[x]))
            score += 10;
    }
    return score;
}

bool Pawns(const int s, const int file)
{
    for (int x = file + 8; x < A8; x += 8)
    {
        if (board[x] == P && color[x] == s)
            return true;
    }
    return false;
}

bool Pawns2(const int s, const int xs, const int start)
{
    int x = start + ForwardSquare[s];
    while (x > H1 && x < A8)
    {
        if (board[x] == P)
            return true;
        if (col[x] > 0 && board[x - 1] == P && color[x - 1] == xs)
            return true;
        if (col[x] < 7 && board[x + 1] == P && color[x + 1] == xs)
            return true;
        x += ForwardSquare[s];
    }
    return false;
}
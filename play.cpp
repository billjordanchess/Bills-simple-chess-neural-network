#include "globals.h"

int NN_Play(const int);
int NN_Train(const int, const int);

int CaptureSearch();
int SelectMove(const int from);

int Reps2();
bool IsMate();

extern int hits, moves;
extern int move_start, move_dest;
extern U64 diff;

move_data root_move;

const int FUTILITY_SCORE = 900;

void z()
{
    DisplayBoard();
    _getch();
}

bool CanMove();

int ReCaptureSearch(int taker, const int sq);

void play(int nn)
{
    int bestscore = -10000;
    int best_train_score = -10000;
    int teacher_score = 0;
    int count = 0;

    move_data train_move = {};
    root_move = {};

    ply = 0;
    Gen();

    int lastmove = first_move[1];

    for (int i = 0; i < lastmove; i++)
    {
        int from = move_list[i].from;
        int to = move_list[i].to;

        if (!MakeMove(from, to))
            continue;

        if (Reps2() || fifty >= 50)
        {
            if (count == 0)
                root_move = move_list[i];

            count++;
            UnMakeMove();
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
        else if (piece_mat[side] == 0 && !CanMove())
        {
            UnMakeMove();
            continue;
        }

        int mat = piece_mat[xside] + pawn_mat[xside]
            - piece_mat[side] - pawn_mat[side];

        int see = -CaptureSearch();
        int target = 0;
        int eval = 0;

        if (nn == NN_TRAIN)
        {
            target = -Eval();
            eval = NN_Train(side, target);
        }
        else if (nn == NN_PLAY)
        {
            eval = NN_Play(side);
        }
        else if (nn == NN_TEACHER)
        {
            eval = -Eval();
        }

        int score = mat + see + eval;

        if (score > bestscore)
        {
            bestscore = score;
            root_move = move_list[i];

            if (nn == NN_TRAIN)
                teacher_score = mat + see + target;
        }

        if (nn == NN_TRAIN)
        {
            int score2 = mat + see + target;

            if (score2 > best_train_score)
            {
                best_train_score = score2;
                train_move = move_list[i];
            }
        }

        UnMakeMove();

        if (nn != NN_TRAIN && score > 9900)
            break;
    }

    if (nn == NN_TRAIN)
    {
        moves++;
        diff += llabs((long long)teacher_score - bestscore);

        if (train_move.from == root_move.from &&
            train_move.to == root_move.to)
            hits++;
    }

    move_start = root_move.from;
    move_dest = root_move.to;
}

int CaptureSearch()
{
    nodes++;

    int score = 0;
    int best = 0;
    int from, to;

    GenCaptures();

    for (int i = first_move[ply]; i < first_move[ply + 1]; ++i)
    {
        SelectMove(i);
        from = move_list[i].from;
        to = move_list[i].to;

        if (best >= piece_value[board[to]])
        {
            continue;
        }

        score = ReCaptureSearch(from, to);

        if (score > best)
        {
            best = score;
        }
    }
    return best;
}

bool IsMate()
{
    Gen();
    int firstmove = first_move[1];
    int lastmove = first_move[2];
    int from, to;

    for (int i = firstmove; i < lastmove; i++)
    {
        SelectMove(i);
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




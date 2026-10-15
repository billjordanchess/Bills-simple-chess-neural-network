#include <setjmp.h>

using namespace std;

#include "globals.h"

const int MARGIN = 100;

static void SetHashMove();
void DisplayPV(int i);
void LoadBook();

extern int fixed_time;
extern int fixed_depth;
extern int max_time;
extern long long stop_time;
extern int max_depth;
extern int hash_from, hash_to;

jmp_buf env;
static bool stop_search;

static int currentmax;

static void CheckUp();

int Search(int alpha, int beta, int depth);
int ReCaptureSearch(int taker, const int sq);
int CaptureSearch(int alpha, int beta);
int Reps2();

int NN_Play(const int);
bool LookUp(const int s);
void AddHash(const int s, const move_data m);
int Book();

void think()
{
    int bookflag = 0;

    if (hply < 8)
    {
        if (hply <= 1)
            LoadBook();
        bookflag = Book();
        if (bookflag > 0)
        {
            return;
        }
    }

    int score;

    stop_search = false;
    setjmp(env);

    if (stop_search)
    {
        while (ply)
            UnMakeMove();
        return;
    }
    if (fixed_time == 0)
    {
        if (Attack(xside, kingloc[side]))
        {
            max_time = max_time / 2;
        }
    }
    start_time = GetTime();
    stop_time = start_time + max_time;

    ply = 0;
    nodes = 0;

    NewPosition();
    memset(history, 0, sizeof(history));
    cout << "ply score time nodes pv" << endl;

    for (int i = 1; i <= max_depth; ++i)
    {
        currentmax = i;
        if (i > 1 && fixed_depth == 0)
            if (fixed_time == 1)
            {
                if (GetTime() >= start_time + max_time)
                {
                    stop_search = true;
                    return;
                }
            }
            else if (GetTime() >= start_time + max_time / 4)
            {
                stop_search = true;
                return;
            }
        score = Search(-10000, 10000, i);
        LookUp(side);

        cout << i << " " << score << " " << (GetTime() - start_time) / 10 << " " << nodes << " ";

        if (LookUp(side))
        {
            move_start = hash_from;
            move_dest = hash_to;
            DisplayPV(i);
        }
        cout << endl;

        if (score > 9000 || score < -9000)
        {
            break;
        }
    }
}

static int Search(int alpha, int beta, int depth)
{
    if (ply && Reps2())
    {
        return 0;
    }

    if (depth < 1)
        return CaptureSearch(alpha, beta);

    nodes++;

    if ((nodes & 4095) == 0 && fixed_depth == 0)
    {
        CheckUp();
    }

    if (ply > MAX_PLY - 2)
       return Eval();

    int bestscore = -10001;

    int check = 0;

    if (Attack(xside, kingloc[side]))
    {
        check = 1;
    }
    Gen();

    move_data bestmove = move_list[first_move[ply]];

    if (LookUp(side))
        SetHashMove();

    int count = 0;
    int score;
    int d;

    int top_score = HASH_SCORE;

    for (int i = first_move[ply]; i < first_move[ply + 1]; ++i)
    {
        if (top_score > 0)
            top_score = SelectMove(i);

        if (!MakeMove(move_list[i].from, move_list[i].to))
        {
            continue;
        }
        count++;

        if (Attack(xside, kingloc[side]))
        {
            d = depth;
        }
        else
        {
            d = depth - 3;
            if (move_list[i].score > CAPTURE_SCORE || count == 1 || check == 1)
            {
                d = depth - 1;
            }
            else if (move_list[i].score > 0)
            {
                d = depth - 2;
            }
        }
  
        score = -Search(-beta, -alpha, d);
        UnMakeMove();

        if (score > bestscore)
        {
            bestscore = score;
            bestmove = move_list[i];
        }
        if (score > alpha)
        {
            if (score >= beta)
            {
                if (board[move_list[i].to] == EMPTY)
                    history[move_list[i].from][move_list[i].to] += depth;
                AddHash(side, move_list[i]);
                return beta;
            }
            alpha = score;
        }
    }
    if (count == 0)
    {
        if (Attack(xside, kingloc[side]))
        {
            return -10000 + ply;
        }
        else
            return 0;
    }

    if (fifty >= 100)
        return 0;
    AddHash(side, bestmove);
    return alpha;
}

static int CaptureSearch(int alpha, int beta)
{
    nodes++;

    int eval = pawn_mat[side] + piece_mat[side] - pawn_mat[xside] - piece_mat[xside];

    if (eval > alpha - MARGIN && eval < beta + MARGIN)
    {
        int nn_score = -NN_Play(side);
        int limit = abs(eval) + 100;
        if (limit > 900)
            limit = 900;
        if (nn_score > limit)
            nn_score = limit;
        else if (nn_score < -limit)
            nn_score = -limit;
        eval += nn_score;
    }

    if (eval > alpha)
    {
        if (eval >= beta)
        {
            return beta;
        }
        alpha = eval;
    }
    else if (eval + piece_value[Q] + 50 < alpha)
        return alpha;

    int score = 0, bestmove = 0;
    int best = 0;

    GenCaptures();

    for (int i = first_move[ply]; i < first_move[ply + 1]; ++i)
    {
        SelectMove(i);
        if (eval + piece_value[board[move_list[i].to]] < alpha)
        {
            continue;
        }

        score = ReCaptureSearch(move_list[i].from, move_list[i].to);

        if (score > best)
        {
            best = score;
            bestmove = i;
        }
    }
    if (best > 0)
    {
        eval += best;
    }
    if (eval > alpha)
    {
        if (eval >= beta)
        {
            if (best > 0)
                AddHash(side, move_list[bestmove]);
            return beta;
        }
        return eval;
    }
    return alpha;
}

int ReCaptureSearch(int taker, const int sq)
{
    int next_taker;
    int count = 0;
    int total_recaptures = 0;
    int score[12];

    memset(score, 0, sizeof(score));
    score[0] = piece_value[board[sq]];
    score[1] = piece_value[board[taker]];

    int total_score = 0;

    while (count < 10)
    {
        if (!MakeRecapture(taker, sq))
            break;
        total_recaptures++;
        nodes++;
        count++;

        next_taker = LowestAttacker(side, sq);

        if (next_taker > -1)
        {
            score[count + 1] = piece_value[board[next_taker]];
            if (score[count] > score[count - 1] + score[count + 1])
            {
                count--;
                break;
            }
        }
        else
        {
            break;
        }
        taker = next_taker;
    }
    while (count > 1)
    {
        if (score[count - 1] >= score[count - 2])
            count -= 2;
        else
            break;
    }
    for (int x = 0; x < count; x++)
    {
        if (x % 2 == 0)
            total_score += score[x];
        else
            total_score -= score[x];
    }
    while (total_recaptures)
    {
        UnMakeRecapture();
        total_recaptures--;
    }
    return total_score;
}

int Reps2()
{
    for (int i = hply - 2; i >= 0 && i >= hply - fifty; i -= 2)
    {
        if (game_list[i].hashkey == currentkey &&
            game_list[i].lock == currentlock)
            return 1;
    }
    return 0;
}

int SelectMove(const int from)
{
    int bestscore = move_list[from].score;
    int bestindex = from;
    for (int i = from + 1; i < first_move[ply + 1]; ++i)
        if (move_list[i].score > bestscore)
        {
            bestscore = move_list[i].score;
            bestindex = i;
        }

    move_data bestmove = move_list[from];
    move_list[from] = move_list[bestindex];
    move_list[bestindex] = bestmove;

    return bestscore;
}

static void CheckUp()
{
    if ((GetTime() >= stop_time || (max_time < 50 && ply>1)) && fixed_depth == 0)
    {
        stop_search = true;
        longjmp(env, 0);
    }
}

static void SetHashMove()
{
    for (int x = first_move[ply]; x < first_move[ply + 1]; x++)
    {
        if (move_list[x].from == hash_from && move_list[x].to == hash_to)
        {
            move_list[x].score = HASH_SCORE;
            return;
        }
    }
}

static void DisplayPV(int i)
{
    for (int x = 0; x < i; x++)
    {
        if (LookUp(side) == false)
            break;
        cout << " ";
        Alg(hash_from, hash_to);
        MakeMove(hash_from, hash_to);
    }
    while (ply)
        UnMakeMove();
}

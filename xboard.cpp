#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/timeb.h>
#include <assert.h>

using namespace std;

#include "globals.h"

struct options {
    int max_time2;
    int max_depth2;
    int fixed_time2;
    int fixed_depth2;
};

void NewGame();
void SetMaterial();
void xboard();
int XboardResult();

int ParseMove(string s);

int LoadDiagram(string file_name);

void think2();

void z();

extern int move_start, move_dest;

static move_data root_move;

int Search(int alpha, int beta, int depth);

float CaptureSearch();
int ReCaptureSearch(int a, const int sq);
int Reps2();
void CheckUp();
void LoadBook();
int Book();

int NN_Play(int);

int QSearch(int alpha, int beta);

bool stop_search = false;

int fixed_depth2 = 0;
long long max_time2 = 0;
long long stop_time;
int max_depth2 = 0;
int fixed_time2 = 0;
/*

<b>xboard()</b> is used when the engine is playing with an interface like Arena or Winboard.

*/
void xboard()
{
    int computer_side;
    int m;
    int post = 0;
    int analyze = 0;

    string line;
    string command;

    signal(SIGINT, SIG_IGN);
    NewGame();
    computer_side = EMPTY;
    LoadWeights();

    // main loop
    while (true)
    {
        // engine to move
        if (side == computer_side)
        {
            if (fixed_time2 == 1)
                think2();
            else
                think2();
            
            SetMaterial();
            Gen();

            move_list[WHITE].from = move_start;
            move_list[WHITE].to = move_dest;
            cout << "move " << MoveString(move_start, move_dest, 0) << endl;

            MakeMove(move_start, move_dest);

            ply = 0;
            Gen();
            if (XboardResult() > 0)
            {
                NewGame();
                computer_side = EMPTY;
            }
            continue;
        }

        // read one line from GUI
        if (!getline(cin, line))
            return;         // EOF or error

        if (line.empty())
            continue;

        // first token = command
        istringstream text(line);
        text >> command;

        // --- protocol commands ---

        if (command == "xboard")
        {
            // nothing special to do here
            continue;
        }

        if (command == "protover")
        {
            // advertise features
            cout << "feature usermove=0 ping=1 setboard=0 colors=1 sigint=0 san=0 done=1" << endl;
            continue;
        }

        if (command == "new")
        {
            NewGame();
            computer_side = EMPTY;
            continue;
        }

        if (command == "quit")
        {
            return;
        }

        if (command == "force")
        {
            computer_side = EMPTY;
            continue;
        }

        if (command == "white")
        {
            side = 0;
            xside = 1;
            Gen();
            computer_side = EMPTY;
            continue;
        }

        if (command == "black")
        {
            side = 1;
            xside = 0;
            Gen();
            computer_side = EMPTY;
            continue;
        }

        if (command == "sb")
        {
            string sText;
            string sFen = "c:\\diagrams\\";
            cout << "Enter file name> ";
            cin >> sText;
            sFen += sText + ".fen";
            LoadDiagram(sFen);
            continue;
        }

        if (command == "sd")
        {
            int depth;
            if (text >> depth)
            {
                max_depth2 = depth;
                fixed_depth2 = 1;
            }
            continue;
        }
        if (command == "st")
        {
            if (text >> max_time2)
            {
                max_time2 *= 1000;
                max_depth2 = MAX_PLY;
                fixed_time2 = 1;
                fixed_depth2 = 0;
                continue;
            }
        }

        if (command == "time")
        {
            int cs = 0;
            if (text >> cs)
            {
                int ms = cs * 10;
                max_time2 = ms / 30;
                if (max_time2 < 50)
                    max_time2 = 50;
                fixed_time2 = 0;
                max_depth2 = MAX_PLY;
            }
            continue;
        }

        if (command == "otim")
        {
            continue;
        }

        if (command == "go")
        {
            computer_side = side;
            continue;
        }

        if (command == "random" ||
            command == "level" ||
            command == "hard" ||
            command == "easy")
        {
            continue;
        }

        if (command == "undo")
        {
            if (!hply)
                continue;
            UnMakeMove();
            ply = 0;
            Gen();
            continue;
        }

        if (command == "remove")
        {
            if (hply < 2)
                continue;
            UnMakeMove();
            UnMakeMove();
            ply = 0;
            Gen();
            continue;
        }

        // --- assume it's a move like "e2e4" ---

        first_move[0] = 0;
        Gen();

        m = ParseMove(command);          

        if (m == -1 || !MakeMove(move_list[m].from, move_list[m].to))
        {
            cerr << "Error (unknown command): " << command << endl;
            continue;
        }

        ply = 0;
        Gen();
        if (XboardResult() > 0)
        {
            NewGame();
            computer_side = EMPTY;
        }
        else if (computer_side == EMPTY)
        {
            computer_side = side;
        }
    }
}
/*

<b>XboardResult()</b> is called from <b>xboard</b> and is similar to <b>CheckResult()</b>.

*/
int XboardResult()
{
    int i;
    int flag = 0;

    SetMaterial();
    if (pawn_mat[WHITE] == 0 && pawn_mat[BLACK] == 0 && piece_mat[WHITE] <= 300 && piece_mat[BLACK] <= 300)
    {
        cout << "1/2-1/2 {Material}" << endl;
        return 1;
    }
    ply = 0;

    Gen();
    for (i = 0; i < first_move[1]; ++i)
        if (MakeMove(move_list[i].from, move_list[i].to))
        {
            UnMakeMove();
            flag = 1;
            break;
        }
    if (i == first_move[1] && flag == 0)
    {
        if (Attack(xside, kingloc[side]))
        {
            if (side == 0)
            {
                cout << "0-1 {BLACK mates}" << endl;
            }
            else
            {
                cout << "1-0 {WHITE mates}" << endl;
            }
        }
        else
        {
            cout << "1/2-1/2 {Stalemate}" << endl;
        }
        return 1;
    }
    if (Reps() >= 3)
    {
        cout << "1/2-1/2 {Draw by repetition}" << endl;
        return 1;
    }
    else if (fifty >= 100)
    {
        cout << "1/2-1/2 {Draw by fifty move rule}" << endl;
        return 1;
    }
    return 0;
}

void think2()
{
	int bookflag = 0;
	if (hply < 8)
	{
		if (hply == 0)
			LoadBook();
		bookflag = Book();
		if (bookflag > 0)
		{
			return;
		}
	}

	int from, to;
	int score = -10000;
	int alpha = -10000;
	int beta = 10000;
	int bestscore = -10000;
	int count = 0;
	int firstmove = 0;
	float SEE = 0;
	float mat = 0;
	float eval = 0;

    long long current_time;

    stop_search = false;

    start_time = GetTime();
    stop_time = start_time + max_time2;

	if (fixed_depth2 == 0 && max_time2 < 100)
		max_depth2 = 1;

	ply = 0;
	Gen();
	int lastmove = first_move[1];
    root_move = move_list[0];

	for (int iter = 1; iter <= max_depth2; iter++)
	{
        if (stop_search)
            break;
		for (int i = firstmove; i < lastmove; i++)
		{
			from = move_list[i].from;
			to = move_list[i].to;
			if (!MakeMove(from, to))
			{
				continue;
			}

			count++;

			score = -Search(-beta, -alpha, iter - 1);

			UnMakeMove();

			if (score > alpha)
			{
				alpha = score;
			}
			if (score > bestscore)
			{
				bestscore = score;
				root_move = move_list[i];
				if (score > 9900)
					break;
			}
            if (stop_search)
                break;
		}
        if (stop_search)
            break;
		cout << iter <<" "<< bestscore <<" "<< (GetTime() - start_time) / 10 << " " << nodes << " ";
		Alg(root_move.from, root_move.to); cout << " " << endl;
     
        if (fixed_depth2 == 0 && (GetTime() - start_time) >= max_time2)
        {
            break;
        }
	}
    assert(count > 0);
    assert(bestscore > -10000);
    assert(root_move.from >= 0 && root_move.from < 64);
    assert(root_move.to >= 0 && root_move.to < 64);

	move_start = root_move.from;
	move_dest = root_move.to;
}

int Search(int alpha, int beta, int depth)
{
    if (stop_search)
    {
        return 0;
    }
    if ((nodes & 4095) == 0 && fixed_depth2 == 0)
    {
        CheckUp();
        if (stop_search)
        {
            return 0;
        }
    }
  
    if (ply && Reps2())
    {
        return 0;
    }

    if (depth < 1)
        return QSearch(alpha, beta);

    nodes++;

    if (ply > MAX_PLY - 2)
        return Eval();

    move_data bestmove;

    int bestscore = -10001;

    int check = 0;

    if (Attack(xside, kingloc[side]))
    {
        check = 1;
    }
    Gen();

    int count = 0;
    int score;
    int d;

    int top = move_list[first_move[ply]].score;

    for (int i = first_move[ply]; i < first_move[ply + 1]; ++i)
    {
        if (top > 0)
            top = Sort(i);

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
            d = depth - 2;
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

    return alpha;
}

int QSearch(int alpha, int beta)
{
    int mat = piece_mat[side] + pawn_mat[side] - piece_mat[xside] - pawn_mat[xside];

    //*
    if (mat > alpha + 100)
   {
       if (mat >= beta + 100)
       {
           return beta;
       }
       alpha = mat;
   }
   else if (mat + piece_value[Q] + 100 < alpha)
       return alpha;
   //*/

    int SEE = CaptureSearch();
    int score = mat + SEE;
    int play = 0;
    int ev = 0;
    //if (score + 100 < alpha)
    {
        //ev = Eval();
        //score += ev;
    }
    //else
    {
        play = NN_Play(side)/4;     
        score += play;
    }
    /*
    if (SEE < 0)
    {
        cout << " mat %d ", mat);
        cout << " SEE %d ", SEE);
        cout << " ev %d ", ev);
        cout << " play %d ", play);
        cout << " score %d ", score);
        cout << " alpha %d ", alpha);
        z();
        mat = mat;
    }
    //*/
    return score;
}

void CheckUp()
{
    if ((GetTime() >= stop_time || (max_time2 < 50 && ply>1)) && fixed_depth2 == 0)
    {
        stop_search = true;
    }
}






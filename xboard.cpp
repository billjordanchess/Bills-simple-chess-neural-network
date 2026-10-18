#include <signal.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/timeb.h>
#include <assert.h>

using namespace std;

#include "globals.h"//18/10

void NewGame();
void SetMaterial();
void xboard();
int XboardResult();
void think();

int ParseMove(string s);

int LoadDiagram(string file_name);

void z();

extern int move_start, move_dest;
extern int fixed_depth;
extern int max_depth;
extern int fixed_time;
extern int max_time;

int Search(int alpha, int beta, int depth);

void xboard()
{
    int computer_side;
    int m;

    string line;
    string command;
    string sText;

    signal(SIGINT, SIG_IGN);
    NewGame();
    LoadWeights();
    computer_side = EMPTY;

    while (true)
    {
        if (side == computer_side)
        {
            think();

            cout << "move " << MoveString(move_start, move_dest, 0) << endl;

            MakeMove(move_start, move_dest);

            ply = 0;
            Gen();
            SetMaterial();

            if (XboardResult() > 0)
            {
                computer_side = EMPTY;
            }
            continue;
        }

        if (!getline(cin, line))
            return;         

        if (line.empty())
            continue;

        istringstream text(line);
        text >> command;

        if (command == "xboard")
        {
            continue;
        }
        if (command == "protover")
        {
            cout << "feature usermove=0 ping=1 setboard=0 colors=1 sigint=0 san=0 done=1" << endl;
            continue;
        }
        if (command == "ping")
        {
            int n;
            if (text >> n)
                cout << "pong " << n << endl;
            continue;
        }
        if (command == "new")
        {
            NewGame();
            computer_side = BLACK;
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
        if (command == "sd")
        {
            int depth;
            if (text >> depth)
            {
                max_depth = depth;
                fixed_depth = 1;
            }
            continue;
        }
        if (command == "st")
        {
            if (text >> max_time)
            {
                max_time *= 1000;
                max_depth = MAX_PLY;
                fixed_time = 1;
                fixed_depth = 0;
                continue;
            }
        }
        if (command == "time")
        {
            int cs = 0;
            if (text >> cs)
            {
                int ms = cs * 10;
                max_time = ms / 30;
                if (max_time < 50)
                    max_time = 50;
                fixed_time = 0;
                max_depth = MAX_PLY;
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
    }
}

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






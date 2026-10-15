#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <sys/timeb.h>

using namespace std;

#include "globals.h"

#define WHITE_WIN 1
#define BLACK_WIN 2
#define DRAWN 3

int fixed_time;
int fixed_depth;
int max_time;
long long start_time;
long long stop_time;
int max_depth;

int hits, moves, equal_scores;
U64 diff;

float draws;
float percent;
float highest_percent;

void Train(int start_epoch, int epoch_number);
void Match();
int ParseMove(string s);

void ShowHelp();
void DisplayGame(int);
int GetResult();
void SetUp();
void LoadBook();
void InitWeights();
int SaveWeights(int epochs, U64 time);
void SetLearningRate(float rate);
float GetLearningRate();
void xboard();
void think();
int LoadDiagram(string file_name);
int TestMat();

int flip = 0;
int turn = 0;

void NewGame();
void SetMaterial();

int epochs = 0;
int highest_match = 0;
int lowest_diff = 1000000;

std::ofstream logfile("train_log.txt");

int move_start, move_dest;
void z();

void PlayOpening(int);
int PlayGame(const int x, const int first, const int second);

move_data engine_move;

int main()
{
    string command;
    string sFen;
    string sText;

    max_depth = 8;

    int number = 1;
    int start_epoch = 1;
    highest_match = 0;
    lowest_diff = 1000000;
    float highest_percent = 0;
    int epoch_number = 10;

    hits = 0;
    moves = 0;

    cout << " " << endl;
    cout << "Deep Basic Chess Engine" << endl;
    cout << "Version 1.2, 1/10/26" << endl;
    cout << "Copyright 2026 Bill Jordan" << endl;
    cout << "Enter help for help." << endl;
    cout << " " << endl;

    SetUp();
    SetMaterial();
    LoadBook();
    InitWeights();

    while (true)
    {
        cout << "Enter move or command> ";
        cin >> command;
        if (command == "d")
        {
            DisplayBoard();
            continue;
        }
        if (command == "dis")
        {
            cout << "Game number> ";
            cin >> number;
            DisplayGame(number);
            continue;
        }
        if (command == "number" || command == "n")
        {
            cout << "Number of epochs> ";
            cin >> epoch_number;
            continue;
        }
        if (command == "sb")
        {
            //string sFen = "c:\\users\\bill\\desktop\\bscp\\";
            string sFen = "c:\\diagrams\\";//change to the folder you want.
            cin >> sText;
            sFen += sText + ".fen";
            LoadDiagram(sFen);
            continue;
        }
        if (command == "help")
        {
            ShowHelp();
            continue;
        }
        if (command == "load")
        {
            epochs = LoadWeights();
            cout << "weights loaded" << endl;
            if (epochs > 6)
                cout << "epochs " << epochs << endl;
            continue;
        }
        if (command == "play" || command == "p")
        {
            think();
            Alg(move_start, move_dest);
            MakeMove(move_start, move_dest);
            DisplayBoard();
            continue;
        }
        if (command == "sd")
        {
            int depth;
            cin >> depth;
            if (depth > 0)
            {
                max_depth = depth;
                fixed_depth = 1;
            }
            continue;
        }
        if (command == "st")
        {
            if (cin >> max_time)
            {
                max_time *= 1000;
                max_depth = MAX_PLY;
                fixed_time = 1;
                fixed_depth = 0;
                continue;
            }
        }
        if (command == "train" || command == "t")
        {
            cout << "Let the training continue!" << endl;
            Train(epochs, epoch_number);
            continue;
        }
        if (command == "test")
        {
            TestMat();
            continue;
        }
        if (command == "xboard")
        {
            xboard();
            continue;
        }
        if (command == "match")
        {
            Match();
            continue;
        }
        if (command == "quit")
            return 0;
        
        first_move[0] = 0;
        ply = 0;
        Gen();

        int m = ParseMove(command);

        if (m == -1 || !MakeMove(move_list[m].from, move_list[m].to))
        {
            cerr << "Error (unknown command): " << command << endl;
            continue;
        }
        ply = 0;
        Gen();
        if (GetResult() > 0)
        {
            NewGame();
        }
    }
}

void Train(int start_epoch, int epoch_number)
{
    start_time = GetTime();
    for (epochs = start_epoch; epochs < start_epoch + epoch_number; epochs++)
    {
        int student_wins = 0;
        int teacher_wins = 0;
        int match_draws = 0;
        int result;
        int matches = 0;
        int equal_matches = 0;
        int average_diff = 10000;

        hits = 0; moves = 0; diff = 0;

        for (int x = 0; x < 100; x++)
        {
            result = PlayGame(x, NN_TRAIN, NN_TEACHER);
            if (result == WHITE_WIN)
                student_wins++;
            else if (result == BLACK_WIN)
                teacher_wins++;
            else if (result == DRAWN)
                match_draws++;
        }
        for (int x = 0; x < 100; x++)
        {
            result = PlayGame(x, NN_TEACHER, NN_TRAIN);
            if (result == WHITE_WIN)
                teacher_wins++;
            else if (result == BLACK_WIN)
                student_wins++;
            else if (result == DRAWN)
                match_draws++;
        }
        float percent = (student_wins + 0.5f * match_draws) / 2.0f;
        if (percent > highest_percent)
        {
            highest_percent = percent;
        }
        if (moves > 0)
        {
            matches = (100 * hits) / moves;
            average_diff = diff / moves;
        }
        if (matches > highest_match)
            highest_match = matches;
        if (average_diff < lowest_diff)
            lowest_diff = average_diff;

        cout << "epoch " << epochs << endl;
        cout << "time " << (GetTime() - start_time) / 1000 << " seconds" << endl;
        cout << "Student Wins " << student_wins << endl;
        cout << "Teacher Wins " << teacher_wins << endl;
        cout << "Draws " << match_draws << endl;
        cout << endl;
        cout << "percent " << percent << endl;
        cout << "matches " << matches << " % " << endl;
        cout << "average centipawns diff " << average_diff << endl;
        cout << endl;
        cout << "highest percent " << highest_percent << endl;
        cout << "highest match " << highest_match << " % " << endl;
        cout << "lowest centipawns diff " << lowest_diff << endl;
        cout << endl;
        logfile << "epochs " << epochs << " draws " << draws << endl;
        logfile << "student wins " << student_wins / 2 << " teacher wins " << teacher_wins / 2 << std::endl;
        logfile << "percent " << percent << " highest percent " << highest_percent << std::endl;
    }
    SaveWeights(epochs, 0);
    cout << "Weights saved." << endl;
}

void Match()
{
    int student_wins = 0;
    int teacher_wins = 0;
    int match_draws = 0;
    int result;
    for (int x = 0; x < 100; x++)
    {
        result = PlayGame(x, NN_PLAY, NN_TEACHER);
        if (result == WHITE_WIN)
            student_wins++;
        else if (result == BLACK_WIN)
            teacher_wins++;
        else if (result == DRAWN)
            match_draws++;
    }
    cout << "play 1 complete" << endl;
    for (int x = 0; x < 100; x++)
    {
        result = PlayGame(x, NN_TEACHER, NN_PLAY);
        if (result == WHITE_WIN)
            teacher_wins++;
        else if (result == BLACK_WIN)
            student_wins++;
        else if (result == DRAWN)
            match_draws++;
    }
    cout << "Student Wins " << student_wins << endl;
    cout << "Teacher Wins " << teacher_wins << endl;
    cout << "Draws " << match_draws << endl;
}

void ShowHelp()
{
    cout << "d - Displays a game, move by move." << endl;
    cout << "sb - Loads a fen diagram." << endl;
    cout << "load - Loads saved weights." << endl;
    cout << "match - ." << endl;
    cout << "train or t - Trains the network." << endl;
    cout << "quit - Quits the program." << endl;
}

int PlayGame(const int x, const int first, const int second)
{
    int result;
    turn = 1;
    NewGame();
    PlayOpening(x);
    while (true)
    {
        if (side == 0)
            play(first);
        else
            play(second);
        MakeMove(move_start, move_dest);
        turn++;
        result = GetResult();
        if (result > 0)
        {
            return result;
        }
    }
}

void DisplayGame(int n)
{
    NewGame();
    PlayOpening(n);
    SetMaterial();
    while (true)
    {
        play(NN_PLAY);
        MakeMove(move_start, move_dest);
        cout << "test game ";
        Alg(move_start, move_dest);
        DisplayBoard();
        _getch();
        if (GetResult() > 0)
        {
            DisplayBoard();
            break;
        }
    }
}

int ParseMove(string s)
{
    if (s.length() < 4)
        return -1;
    if (s[0] < 'a' || s[0] > 'h' ||
        s[1] < '1' || s[1] > '8' ||
        s[2] < 'a' || s[2] > 'h' ||
        s[3] < '1' || s[3] > '8')
        return -1;

    int from = s[0] - 'a';
    from += ((s[1] - '0') - 1) * 8;
    int to = s[2] - 'a';
    to += ((s[3] - '0') - 1) * 8;

    for (int i = 0; i < first_move[1]; ++i)
        if (move_list[i].from == from && move_list[i].to == to)
        {
            return i;
        }
    return -1;
}

int GetResult()
{
    int i;
    int flag = 0;

    SetMaterial();
    if (pawn_mat[WHITE] == 0 && pawn_mat[BLACK] == 0 && piece_mat[WHITE] <= 300 && piece_mat[BLACK] <= 300)
    {
        return DRAWN;
    }
    if (pawn_mat[side] == 0 && piece_mat[side] == 0 &&
        (piece_mat[xside] == 500 || piece_mat[xside] == 800 || piece_mat[xside] == 900))
    {
        if (side == 0)
            return BLACK_WIN;
        else
            return WHITE_WIN;
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
                return BLACK_WIN;
            }
            else
            {
                return WHITE_WIN;
            }
        }
        else
        {
            return DRAWN;
        }
    }
    if (pawn_mat[BLACK] == 0 && piece_mat[BLACK] == 0 && pawn_mat[WHITE] + piece_mat[WHITE] >= 900)
    {
        return WHITE_WIN;
    }
    if (pawn_mat[WHITE] == 0 && piece_mat[WHITE] == 0 && pawn_mat[BLACK] + piece_mat[BLACK] >= 900)
    {
        return BLACK_WIN;
    }
    if (Reps() >= 3)
    {
        return DRAWN;
    }
    else if (fifty >= 100)
    {
        return DRAWN;
    }
    if (turn > 400)
    {
        return DRAWN;
    }
    return 0;
}

int Reps()
{
    int r = 0;
    if (hply - fifty >= 2)
    {
        for (int i = hply; i >= hply - fifty; i -= 2)
            if (game_list[i].hashkey == currentkey && game_list[i].lock == currentlock)
                r++;
    }
    return r;
}

int LoadDiagram(string file_name)
{
    ifstream file(file_name);
    if (!file) {
        cerr << file_name << " not found!" << endl;
        return 1;
    }

    string fen_text;
    getline(file, fen_text);

    int x, n = 0;
    int c = 0, i = 0, j;

    memset(pawn_mat, 0, sizeof(pawn_mat));
    memset(piece_mat, 0, sizeof(piece_mat));

    for (x = 0; x < 64; x++)
    {
        board[x] = EMPTY;
        color[x] = EMPTY;
    }

    while (fen_text[c])
    {
        if (fen_text[c] >= '0' && fen_text[c] <= '8')
            i += fen_text[c] - 48;
        if (fen_text[c] == '\\')
        {
            c++;
            continue;
        }          
        j = Flip[i];

        switch (fen_text[c])
        {
        case 'K': AddPiece(WHITE, K, j); i++; break;
        case 'Q': AddPiece(WHITE, Q, j); i++; break;
        case 'R': AddPiece(WHITE, R, j); i++; break;
        case 'B': AddPiece(WHITE, B, j); i++; break;
        case 'N': AddPiece(WHITE, N, j); i++; break;
        case 'P': AddPiece(WHITE, P, j); i++; break;
        case 'k': AddPiece(BLACK, K, j); i++; break;
        case 'q': AddPiece(BLACK, Q, j); i++; break;
        case 'r': AddPiece(BLACK, R, j); i++; break;
        case 'b': AddPiece(BLACK, B, j); i++; break;
        case 'n': AddPiece(BLACK, N, j); i++; break;
        case 'p': AddPiece(BLACK, P, j); i++; break;
        }
        c++;
        if (fen_text[c] == ' ')
            break;
        if (i > 63)
            break;
    }
    if (fen_text[c] == ' ' && fen_text[c + 2] == ' ')
    {
        if (fen_text[c + 1] == 'w')
        {
            side = 0; xside = 1;
        }
        if (fen_text[c + 1] == 'b')
        {
            side = 1; xside = 0;
        }
    }

    while (fen_text[c])
    {
        switch (fen_text[c])
        {
        case '-': break;
        case 'K':if (color[E1] == WHITE && board[E1] == K)
            game_list[hply].castle_k[WHITE] = 1;
            break;
        case 'Q':if (color[E1] == WHITE && board[E1] == K)
            game_list[hply].castle_q[WHITE] = 1;
            break;
        case 'k':if (color[E8] == BLACK && board[E8] == K)
            game_list[hply].castle_k[BLACK] = 1;
            break;
        case 'q':if (color[E8] == BLACK && board[E8] == K)
            game_list[hply].castle_q[BLACK] = 1;
            break;
        default:break;
        }
        c++;
    }
    NewPosition();
    DisplayBoard();
    hply = 8;

    if (side == 0)
        cout << "WHITE to move" << endl;
    else
        cout << "BLACK to move" << endl;
    cout << fen_text << endl;
    return 0;
}

void SetUp()
{
    RandomizeHash();
    SetTables();
    SetMoves();
    InitBoard();
}

void NewGame()
{
    InitBoard();
    first_move[0] = 0;
    turn = 0;
    fifty = 0;
    ply = 0;
    hply = 0;
}

void SetMaterial()
{
    pawn_mat[WHITE] = 0;
    pawn_mat[BLACK] = 0;
    piece_mat[WHITE] = 0;
    piece_mat[BLACK] = 0;
    for (int x = 0; x < 64; x++)
    {
        if (board[x] != EMPTY)
        {
            if (board[x] == K)
                kingloc[color[x]] = x;
            else
            {
                if (board[x] == P)
                    pawn_mat[color[x]] += 100;
                else
                    piece_mat[color[x]] += piece_value[board[x]];
            }
        }
    }
}

long long GetTime()
{
    struct timeb timebuffer;
    ftime(&timebuffer);
    return (timebuffer.time * 1000) + timebuffer.millitm;
}

string MoveString(int from, int to, int promote)
{
    string str = "";

    str += col[from] + 'a';
    str += row[from] + '1';
    str += col[to] + 'a';
    str += row[to] + '1';

    string promote_piece[] = { "-","n","b","r","q","q" };

    if (promote > 0)
    {
        str += promote_piece[promote];
    }
    return str;
}

void Alg(int a, int b)
{
    Algebraic(a);
    Algebraic(b);
}

void Algebraic(int a)
{
    if (a < 0 || a>63) return;
    char c[2] = "a";
    c[0] = 96 + 1 + col[a];
    cout << c << row[a] + 1;
}

void DisplayBoard()
{
    static int board_color[64] =
    {
        1, 0, 1, 0, 1, 0, 1, 0,
        0, 1, 0, 1, 0, 1, 0, 1,
        1, 0, 1, 0, 1, 0, 1, 0,
        0, 1, 0, 1, 0, 1, 0, 1,
        1, 0, 1, 0, 1, 0, 1, 0,
        0, 1, 0, 1, 0, 1, 0, 1,
        1, 0, 1, 0, 1, 0, 1, 0,
        0, 1, 0, 1, 0, 1, 0, 1
    };

    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int text = 15;

    int i;
    int x = 0;
    int c;

    if (flip == 0)
        cout << "\n8";
    else
        cout << "\n1";

    for (int j = 0; j < 64; ++j)
    {
        if (flip == 0)
            i = Flip[j];
        else
            i = 63 - Flip[j];
        c = 6;
        switch (color[i])
        {
        case EMPTY:
            if (board_color[i] == 0)
                text = 127;
            else
                text = 34;
            SetConsoleTextAttribute(hConsole, text);

            cout << "  ";
            SetConsoleTextAttribute(hConsole, 15);
            break;
        case 0:
            if (board_color[i] == 0)
                text = 126;
            else
                text = 46;
            SetConsoleTextAttribute(hConsole, text);
            cout << " " << piece_char[board[i]];
            SetConsoleTextAttribute(hConsole, 15);
            break;

        case 1:
            if (board_color[i] == 0)
                text = 112;
            else
                text = 32;
            SetConsoleTextAttribute(hConsole, text);
            cout << " " << piece_char[board[i]];
            SetConsoleTextAttribute(hConsole, 15);
            break;

        default:
            cout << " ." << c;
            break;

        }
        if (board[i] < 0 || board[i] > 6)
            if (x == 0)
                cout << " ." << board[i];
            else
                cout << "  " << board[i];
        if (flip == 0)
        {
            if ((j + 1) % 8 == 0 && j != 63)
                cout << std::endl << row[i];
        }
        else
        {
            if ((j + 1) % 8 == 0 && row[i] != 7)
                cout << std::endl << row[j] + 2;
        }
    }
    if (flip == 0)
        cout << "\n\n   a b c d e f g h\n\n";
    else
        cout << "\n\n   h g f e d c b a\n\n";
}

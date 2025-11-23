#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sys/timeb.h>

using namespace std;

#include "globals.h"

#define WHITE_WIN 1
#define BLACK_WIN 2
#define DRAWN 3

float draws;
float percent;
float highest_percent;

void ShowHelp();

float GetAverage();
void DisplayGame(int);

int GetResult();

void SetUp();
void LoadBook();

void InitWeights();
int SaveWeights(int);
void CompWeights();

void UpdateBest();
void UpdateWeights();
void UpdateOldWeights();

void SetLearningRate(float rate);
float GetLearningRate();

void xboard();

static int LoadDiagram(string file_name);

int flip = 0;

int player[2];

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

int fixed_time;
int fixed_depth;
int max_time;
int start_time;
int stop_time;
int max_depth;
int turn = 0;

void NewGame();
void SetMaterial();

int moves = 0;
int epochs = 0;

int move_start, move_dest;
void z();

void PlayOpening(int);

int Train();
int PlayBatch(const int x, const int first, const int second);

move_data engine_move;

/*
<b>main()</b> is the entry point of the program.
Several functions are called to initialise the engine.
<ul>
<li>SetUp();
<li>SetMaterial();
<li>InitWeights();
<li>LoadBook();
</ul>
The training with <b>Train()</b> is now ready to begin.
*/
int main()
{
	string command;
	string sFen;
	string sText;

	int number = 1;

	cout << " " << endl;
	cout << "Deep Basic Chess Engine" << endl;
	cout << "Version 1.1, 20/11/25" << endl;
	cout << "Copyright 2025 Bill Jordan" << endl;
	cout << "Enter help for help." << endl;
	cout << " " << endl;

	fixed_time = 0;

	//srand(1);

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
			cout << "Game number> ";
			cin >> number;
			DisplayGame(number);
			continue;
		}
		if (command == "sb")
		{
			string sFen = "c:\\diagrams\\";
			cout << "Enter file name> ";
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
			LoadWeights();
			cout << "weights loaded" << endl;
			continue;
		}
		if (command == "quit")
			return 0;
		if (command == "train" || command == "t")
		{
			cout << "The training starts..." << endl;
			Train();
			continue;
		}
		if (command == "xboard")
		{
			xboard();
			continue;
		}
	}
}
/*
<b>ShowHelp()</b> displays keyboard options for the user.
*/
void ShowHelp()
{
	cout << "d - Displays a game, move by move." << endl;
	cout << "sb - Loads a fen diagram." << endl;
	cout << "load - Loads saved weights." << endl;
	cout << "train or t - Trains the network." << endl;
	cout << "quit - Quits the program." << endl;
	cout << "xboard - Starts xboard." << endl;
}

/*
<b>Train()</b> iterates through the epochs. Each epoch, it plays two training batches between
the student and the frozen opponent. Players swap sides after the first batch. Batches consist of
100 games, each starting with a different opening line. This introduces variety into the games.
<p>
The student's net is updated move by move, by comparing the net's score with the target's score. 
The target's score is calculated by using a standard eval() function.
The frozen opponent's net does not change, unless an update happens. 
<p>
This is followed by a 200-game match, in which no training happens. 
The student plays White in 100 games and Black in a 100 games.
The student's percentage score is calculated.
*/
int Train()
{
	std::ofstream logfile("train_log.txt");
	int result;
	int updates = 0;
	float rate = 0.9f;//0.99f
	float threshold = 75.0f;//75 65 60.0f
	float old_percent = 0.f;
	int exceed = 0;
	start_time = GetTime();
	int start_epochs = 0;
	float student_wins;
	float frozen_wins;

	for (epochs = start_epochs; epochs < start_epochs + 50; epochs++)//50
	{
		for (int x = 0; x < 100; x++)
		{
			result = PlayBatch(x, NN_TRAIN, NN_OPPONENT);
		}
		for (int x = 0; x < 100; x++)
		{
			result = PlayBatch(x, NN_OPPONENT, NN_TRAIN);
		}
		student_wins = 0;
		frozen_wins = 0;
		draws = 0;

		for (int x = 0; x < 100; x++)
		{
			result = PlayBatch(x, NN_PLAY, NN_OPPONENT);
			if (result == WHITE_WIN)
				student_wins++;
			else if (result == BLACK_WIN)
				frozen_wins++;
			else if (result == DRAWN)
				draws++;
		}
		for (int x = 0; x < 100; x++)
		{
			result = PlayBatch(x, NN_OPPONENT, NN_PLAY);
			if (result == WHITE_WIN)
				frozen_wins++;
			else if (result == BLACK_WIN)
				student_wins++;
			else if (result == DRAWN)
				draws++;
		}
		percent = (student_wins + draws / 2) / 2;
		old_percent = percent;
		if (percent >= threshold)
		{
			exceed++;
			cout << "exceed " << exceed << endl;
			cout << "time " << (GetTime() - start_time) / 10 << endl;
			if (exceed >= 2)
			{
				exceed = 0;
				highest_percent = 0;
				updates++;
				UpdateBest();
				UpdateOldWeights();
				//SetLearningRate(rate);
				//cout << "learning_rate " << GetLearningRate() << endl;
				cout << "break " << endl;
			}
		}
		else
			exceed = 0;
		if (percent > highest_percent)
		{
			highest_percent = percent;
			UpdateBest();
		}
		cout << "epochs " << epochs << endl;
		cout << "time " << (GetTime() - start_time) / 1000 << " seconds" << endl;
		cout << "updates " << updates << endl;
		cout << "exceed " << exceed << endl;
		cout << "trainee wins " << student_wins / 2 << endl;
		cout << "opponent wins " << frozen_wins / 2 << endl;
		cout << "draws " << draws / 2 << endl;
		cout << "percent " << percent << endl;
		cout << "highest percent " << highest_percent << endl;
		cout << endl;
		logfile << "epochs " << epochs << " draws " << draws << endl;
		logfile << "trainee wins " << student_wins / 2 << " frozen wins " << frozen_wins / 2 << std::endl;
		logfile << "percent " << percent << " highest percent " << highest_percent << std::endl;
	}
	UpdateWeights();
	SaveWeights(epochs);
	_getch();
	return 0;
}
/*
Weights are saved to a file, after a training session has finished. 
<b>PlayBatch()</b> plays 100 games. After each game,
the average error is calculated.
*/
int PlayBatch(const int x, const int first, const int second)
{
	float av = .01f;
	int result;

	NewGame();
	PlayOpening(x);
	while (true)
	{
		if (side == 0)
			think(first);
		else
			think(second);
		MakeMove(move_start, move_dest);
		result = GetResult();
		if (result > 0)
		{
			//z();
			//av = GetAverage();
			//cout << "average error %f\n" <<  av);
			return result;
		}
	}
}
/*
<b>DisplayGame()</b> plays through a game, displaying the position with each move.
It can be used for testing purposes.
*/
void DisplayGame(int n)
{
	NewGame();
	PlayOpening(n);
	SetMaterial();//
	while (true)
	{
		think(NN_PLAY);
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
/*

<b>ParseMove()</b> translates a move in the form e2e4 into two numbers between 0 and 63.
Characters must be between a and h or 1 and 8 inclusive. The first two characters create a number between 0 and 63, as do the last 2 characters.
<p>
For example, if a move is e1f1, the start square is four while the destination square is 5.
<p>
It then checks to see if the move is legal by searching for it in the move list.

*/
int ParseMove(char* s)
{
	int start, dest, i;

	if (s[0] < 'a' || s[0] > 'h' ||
		s[1] < '0' || s[1] > '9' ||
		s[2] < 'a' || s[2] > 'h' ||
		s[3] < '0' || s[3] > '9')
		return -1;

	start = s[0] - 'a';
	start += ((s[1] - '0') - 1) * 8;
	dest = s[2] - 'a';
	dest += ((s[3] - '0') - 1) * 8;

	for (i = 0; i < first_move[1]; ++i)
		if (move_list[i].from == start && move_list[i].to == dest)
		{
			return i;
		}
	return -1;
}
/*

<b>DisplayBoard()</b> displays the board
The console object is only used to display in colour.

*/
void DisplayBoard()
{
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	int text = 15;

	int i;
	int x = 0;
	int c;

	if (flip == 0)
		cout << endl << "8";
	else
		cout << endl << "1";

	for (int j = 0; j < 64; ++j)
	{
		if (flip == 0)
			i = Flip[j];
		else
			i = 63 - Flip[j];
		c = color[i];

		switch (c)
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
				cout << endl << row[i];
		}
		else
		{
			if ((j + 1) % 8 == 0 && row[i] != 7)
				cout << endl << row[j] + 2;
		}
	}
	if (flip == 0)
		cout << "\n\n   a b c d e f g h\n\n";
	else
		cout << "\n\n   h g f e d c b a\n\n";
}
/*
<b>GetResult()</b> detects the end of the game.
It recognises:
<ul>
<li>Draw by reduction of material.
<li>Checkmate.
<li>Stalemate.
<li>Triple Repetition.
<li>Draw by the 50 move rule.
</ul>
<p>
To maintain a steady pace in training, it is adjudged a win if one player has only a king and the other side has at least a rook or queen. It also adjudicates a game when it reaches 200 moves.
*/
int GetResult()
{
	int i;
	int flag = 0;

	SetMaterial();
	if (pawn_mat[WHITE] == 0 && pawn_mat[BLACK] == 0 && piece_mat[WHITE] <= 300 && piece_mat[BLACK] <= 300)
	{
		//printf(" insuff ");
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
			//printf(" stale ");
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
		//printf(" rep ");
		return DRAWN;
	}
	else if (fifty >= 100)
	{
		//printf(" 50 ");
		return DRAWN;
	}
	if (turn > 400)
	{
		//printf(" 400 ");
		return DRAWN;
	}
	return 0;
}

/*

<b>Reps()</b> checks to see if a game is drawn by triple repetition.

*/
int Reps()
{
	int r = 0;
	for (int i = hply; i >= hply - fifty; i -= 4)
		if (game_list[i].hash == currentkey && game_list[i].lock == currentlock)
			r++;
	return r;
}
/*

<b>LoadDiagram()</b> loads a position from a <b>fen</b> file.

*/
static int LoadDiagram(string file_name)
{
	ifstream file(file_name);
	if (!file) {
		cerr << "File not found!" << endl;
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
	}

	while (fen_text[c])
	{
		if (fen_text[c] >= '0' && fen_text[c] <= '8')
			i += fen_text[c] - 48;
		if (fen_text[c] == '\\')
			continue;
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

	Gen();
	//count++;
	if (side == 0)
		cout << "WHITE to move" << endl;
	else
		cout << "BLACK to move" << endl;
	cout << fen_text << endl;
	return 0;
}
/*

<b>SetUp()</b> is called when the program starts.

*/
void SetUp()
{
	RandomizeHash();
	SetTables();
	SetMoves();
	InitBoard();
	Gen();
	player[WHITE] = 0;
	player[BLACK] = 0;
	max_time = 1 << 25;
	max_depth = 4;
}
/*

<b>NewGame()</b> is called whenever a game starts.

*/
void NewGame()
{
	InitBoard();
	first_move[0] = 0;
	turn = 0;
	fifty = 0;
	ply = 0;
	hply = 0;
	Gen();
}
/*

<b>SetMaterial()</b> calculates pawn_mat, piece_mat and kingloc for both sides, i.e.
pawn material score, piece material score and king location.

*/
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
/*

<b>GetTime()</b> gets the system time. This is used to help calculate how long it takes the engine to perform a task.

*/
int GetTime()
{
	struct timeb timebuffer;
	ftime(&timebuffer);
	return (timebuffer.time * 1000) + timebuffer.millitm;
}
/*

<b>MoveString()</b> converts square numbers into long algebraic notation.

*/
char* MoveString(int start, int dest, int promote)
{
	static char str[6];

	char c;

	if (promote > 0)
	{
		switch (promote)
		{
		case N: c = 'n'; break;
		case B: c = 'b'; break;
		case R: c = 'r'; break;
		default: c = 'q'; break;
		}
		sprintf_s(str, "%c%d%c%d%c",
			col[start] + 'a',
			row[start] + 1,
			col[dest] + 'a',
			row[dest] + 1,
			c);
	}
	else
		sprintf_s(str, "%c%d%c%d",
			col[start] + 'a',
			row[start] + 1,
			col[dest] + 'a',
			row[dest] + 1);
	return str;
}

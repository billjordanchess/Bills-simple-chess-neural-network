#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <fstream>
#include <sys/timeb.h>

#include "globals.h"

#define WHITE_WIN 1
#define BLACK_WIN 2
#define DRAWN 3

float white_wins;
float black_wins;
float draws;
float percent;
float highest_percent;

void ShowHelp();

float GetAverage();
void DisplayGame(int);
void DisplayVisits();

int GetResult();

void SetUp();
void LoadBook();

void InitWeights();
int SaveWeights(int);
void CompareWeights();
void CompWeights();

void UpdateBest();
void UpdateWeights();
void UpdateOldWeights();

void SetLearningRate(float rate);
float GetLearningRate();

void xboard();

int LoadDiagram(char* file,int);
void CloseDiagram();

FILE *diagram_file;
char fen_name[256];

int flip = 0;

int player[2];

int fixed_time;
int fixed_depth;
int max_time;
int start_time;
int stop_time;
int max_depth;
int turn = 0;

int XboardResult(int);
void NewGame();
void SetMaterial();

int moves = 0;
int epochs = 0;

int move_start,move_dest;
void z();

void PlayOpening(int);

int Train();
int PlayBatch(int s, int x, int first, int second);

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
	char line[256], command[256];
	char sFen[256];
	char sText[256];

	int number = 1;

    printf("\n");
    printf("Deep Basic Chess Engine\n");
    printf("Version 1.0, 4/9/25\n");
    printf("Copyright 2025 Bill Jordan\n");
	printf("Enter help for help.\n");
    printf("\n");

    fixed_time = 0;

    //srand(1);

    SetUp();
    SetMaterial();
    LoadBook();
    InitWeights();
		 
	while(true) 
    {
        fflush(stdout);
		printf("Enter move or command> ");
		if (scanf_s("%s", command) == EOF)
			return 0;
		if (!strcmp(command, "d"))
		{
			scanf_s("%d", &number);
			DisplayGame(number);
			break;
		}
		if (!strcmp(command, "fen"))
		{
			sFen[0] = 0;
			scanf_s("%s", sText);
			strcat_s(sFen, sText);
			strcat_s(sFen, ".fen");
			LoadDiagram(sFen, 1);
			continue;
		}
		if (!strcmp(command, "help"))
		{
			ShowHelp();
			continue;
		}
		if (!strcmp(command, "load"))
		{
			LoadWeights();
			printf("weights loaded\n");
			continue;
		}
		if (!strcmp(command, "quit"))
			return 0;
		if (!strcmp(command, "train"))
		{
			printf("The training starts...\n");
			Train();
			break;
		}
		if (!strcmp(command, "xboard"))
		{
			xboard();
			break;
		}
	}
}

void ShowHelp()
{
	printf("d - Displays a game, move by move.\n");
	printf("sb - Loads a fen diagram.\n");
	printf("load - Loads saved weights.\n");
	printf("train - Trains the network.\n");
	printf("quit - Quits the program.\n");	
	printf("xboard - Starts xboard.\n");
}

/*
<b>Train()</b> iterates through the epochs. Each epoch, it plays four training batches between 
the student and the frozen opponent. Players swap sides after each batch. Batches consist of 
100 games, each starting with a different opening line. This introduces variety into positions. 
<p>
This is followed by a 100-game match, in which no training happens. 
The student's percentage score is calculated. 
*/
int Train()
{
    std::ofstream logfile("train_log.txt");
    int result;
    int updates = 0;
    float rate = 0.999f;
    float threshold = 75.0f;
    float old_percent = 0.f;
    int exceed = 0;
    start_time = GetTime();
    int start_epochs = 0; 

    for(epochs=start_epochs;epochs<start_epochs+50;epochs++)
    {
        white_wins = 0;
        black_wins = 0;
        draws = 0;

        for(int x=0;x<100;x++)
        {
            result = PlayBatch(0, x, NN_TRAIN, NN_PLAY);
        }
        for(int x=0;x<100;x++)
        {
            result = PlayBatch(1, x, NN_TRAIN, NN_PLAY);
        }
        for(int x=0;x<100;x++)
        {
            result = PlayBatch(0, x, NN_TRAIN, NN_PLAY);
        }
        for(int x=0;x<100;x++)
        {
            result = PlayBatch(1, x, NN_TRAIN, NN_PLAY);
        }
        white_wins = 0;
        black_wins = 0;
        draws = 0;
        for(int x=0;x<100;x++)
        {
            result = PlayBatch(0, x, NN_MATCH, NN_OPPONENT);
            if(result==WHITE_WIN)
                white_wins++;
            else if(result==BLACK_WIN)
                black_wins++;
            else if(result==DRAWN)
                draws++;
        }
        percent = white_wins+draws/2;
        old_percent = percent;
        if(percent >= threshold)
        {
            exceed++;
            printf("exceed %d \n",exceed);
            printf("time %d \n",(GetTime() - start_time) / 10);
            if(exceed > 2)
            {
                printf("\n break \n");
                exceed = 0;
                highest_percent = 0;
                updates++;
                UpdateBest();
                UpdateOldWeights();
                SetLearningRate(rate);
            }
        }
		else
			exceed = 0;
        if(percent > highest_percent)
        {
            highest_percent = percent;    
            UpdateBest();
        }
        printf("\nepochs %d \n",epochs);
        printf("time %d seconds\n",(GetTime() - start_time) / 1000);
        printf("updates %d \n",updates);
        printf("exceed %d \n",exceed);
        printf("trainee wins %f \n",white_wins);
        printf("opponent wins %f \n",black_wins);
        printf("draws %f \n",draws);
        printf("percent %f \n",percent);
        printf("highest percent %f \n",highest_percent);
        printf("learning_rate %f \n", GetLearningRate());
        logfile << "epochs " << epochs << " draws " << draws << std::endl;
        logfile << "trainee wins " << white_wins << " frozen wins " << black_wins << std::endl;
        logfile << "percent " << percent << " highest percent " << highest_percent << std::endl;
    }
    UpdateWeights();
    SaveWeights(epochs); 
    _getch();
    return 0;
}
/*
<b>PlayBatch()</b> plays 100 games. After each game, 
the average error is calculated. 
*/
int PlayBatch(int s, int x, int first, int second)
{
    float av = .01f;
    int result;

    NewGame();
    PlayOpening(x);
    while(true)
    {
        if(s==0)
            think(first);
        else
            think(second);
        MakeMove(move_start,move_dest);
        result = GetResult();
        if(result > 0) 
        {
            av = GetAverage();
            //printf("average error %f\n", av);
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
    while(true)
    {
        think(NN_PLAY);
        MakeMove(move_start,move_dest);
        printf("test game ");
        Alg(move_start,move_dest);
        DisplayBoard();
        _getch();
        if(XboardResult(NN_PLAY) > 0) 
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
int ParseMove(char *s)
{
    int start, dest, i;

    if (s[0] < 'a' || s[0] > 'h' ||
        s[1] < '0' || s[1] > '9' ||
        s[2] < 'a' || s[2] > 'h' ||
        s[3] < '0' || s[3] > '9')
        return -1;

    start = s[0] - 'a';
    start += ((s[1] - '0') - 1)*8;
    dest = s[2] - 'a';
    dest += ((s[3] - '0') - 1)*8;

    for (i = 0; i < first_move[1]; ++i)
        if (move_list[i].start == start && move_list[i].dest == dest)
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

    int flip = 0;
    int i;
    int x=0;

    int board_color[64] = 
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

    printf("\n8 ");
    if(flip==0)
    {
        for (int j = 0; j < 64; ++j)
        {
            i = Flip[j];
            {
                switch (color[i])
                {
                case EMPTY:
                    if(board_color[i]==0)
                        text = 127;
                    else
                        text = 34;
                    SetConsoleTextAttribute(hConsole, text);

                    printf("  ");
                    SetConsoleTextAttribute(hConsole, 15);
                    break;
                case 0:
                    if(board_color[i]==0)
                        text = 124;//6;
                    else
                        text = 46;
                    SetConsoleTextAttribute(hConsole, text);
                    printf(" %c", piece_char[board[i]]);
                    SetConsoleTextAttribute(hConsole, 15);
                    break;

                case 1:
                    if(board_color[i]==0)
                        text = 112;
                    else
                        text = 32;
                    SetConsoleTextAttribute(hConsole, text);
                    printf(" %c", piece_char[board[i]] + ('a' - 'A'));
                    SetConsoleTextAttribute(hConsole, 15);
                    break;

                default:
                    printf(" %d.",color[i]);
                    break;

                }
                if((color[i]==0 || color[i]==1) && board[i]==EMPTY)
                    if(x==0)
                        printf(" %d",color[i]);
                    else
                        printf("%d ",color[i]);
                if(board[i]<0 || board[i]>6)
                    if(x==0)
                        printf(" %d.",board[i]);
                    else
                        printf("%d ",board[i]);
            }
            if ((j + 1) % 8 == 0 && j != 63)
                printf("\n%d ", row[i]);
        }
        printf("\n\n   a b c d e f g h\n\n");
    }

    if(flip==1)
    {
        for (int j = 0; j < 64; ++j) {
            i = 63-Flip[j];
            switch (color[i]) 
            {
            case EMPTY:
                printf(" .");
                break;
            case 0:
                printf(" %c", piece_char[board[i]]);
                break;
            case 1:
                printf(" %c", piece_char[board[i]] + ('a' - 'A'));
                break;
            }
            if ((j + 1) % 8 == 0 && row[i] != 7)
                printf("\n%d ",  row[j]+2);//7-
        }
        printf("\n\n   h g f e d c b a\n\n");
    }
}
/*

<b>xboard()</b> is used when the engine is playing with an interface like Arena or Winboard.

*/
void xboard()
{
    int computer_side;
    char line[256], command[256];
    int m;
    int post = 0;
    int analyze = 0;

    signal(SIGINT, SIG_IGN);
    NewGame();
    fixed_time = 0;
    computer_side = EMPTY;
    LoadWeights();
    //LoadWeightsBinary("binary.txt");
    //computer_side = 0;//??test

    while(true) 
    {
        fflush(stdout);
        if (side == computer_side) 
        {
            think(NN_XBOARD);
            SetMaterial();
            Gen();

            move_list[WHITE].start = move_start;
            move_list[WHITE].dest = move_dest;
            printf("move %s\n", MoveString(move_start,move_dest,0));
            fflush(stdout);

            MakeMove(move_start,move_dest);

            ply = 0;
            Gen();
            if(XboardResult(NN_XBOARD) > 0)
            {
                NewGame();
                computer_side = EMPTY;
            }

            continue;
        }
        if (!fgets(line, 256, stdin))
            return;
        if (line[0] == '\n')
            continue;
        sscanf_s(line, "%255s", command);
        if (!strcmp(command, "xboard"))
            continue;
        if (!strncmp(line, "ping", 4)) 
        {
            int n = 0;
            sscanf_s(line, "ping %d", &n);
            printf("pong %d\n", n);
            fflush(stdout);
            continue;
        }
        if (!strcmp(command, "new")) 
        {
            NewGame();
            computer_side = EMPTY;
            continue;
        }
        if (!strcmp(command, "quit"))
        {
            return;
        }
        if (!strcmp(command, "force")) 
        {
            computer_side = EMPTY;
            continue;
        }
        if (!strcmp(command, "white")) 
        {
            side = 0;
            xside = 1;
            Gen();
            computer_side = EMPTY;
            continue;
        }
        if (!strcmp(command, "black")) 
        {
            side = 1;
            xside = 0;
            Gen();
            computer_side = EMPTY;
            continue;
        }
        if (!strcmp(command, "st")) {               
            int st_sec = 0;
            sscanf_s(line, "st %d", &st_sec);
            max_time = st_sec * 1000;               
            fixed_time = 1;
            max_depth = MAX_PLY;
            continue;
        }
        if (!strcmp(command, "sd")) 
        {
            sscanf_s(line, "sd %d", &max_depth);
            max_time = 1 << 25;
            continue;
        }
        if (!strcmp(command, "time")) 
        {             
            int cs = 0;
            sscanf_s(line, "time %d", &cs);
            int ms = (cs * 10);
            max_time = ms / 30;                     
            if (max_time < 50) 
                max_time = 50;       
            fixed_time = 0;
            max_depth = MAX_PLY;
            continue;
        }
        if (!strcmp(command, "otim")) 
        {
            continue;
        }
        if (!strcmp(command, "go")) 
        {
            computer_side = side;
            continue;
        }
        if (!strncmp(line, "protover", 8)) 
        {
            printf("feature usermove=0 ping=1 setboard=0 colors=1 sigint=0 san=0 done=1\n");
            fflush(stdout);
            continue;
        }
        if (!strcmp(command, "random")) 
            continue;
        if (!strcmp(command, "level")) 
            continue;
        if (!strcmp(command, "hard")) 
            continue;
        if (!strcmp(command, "easy")) 
            continue;
        if (!strcmp(command, "undo")) 
        {
            if (!hply)
                continue;
            TakeBack();
            ply = 0;
            Gen();
            continue;
        }
        if (!strcmp(command, "remove")) 
        {
            if (hply < 2)
                continue;
            TakeBack();
            TakeBack();
            ply = 0;
            Gen();
            continue;
        }
        if (!strcmp(command, "post")) 
        {
            post = 2;
            continue;
        }
        if (!strcmp(command, "nopost")) 
        {
            post = 0;
            continue;
        }

        first_move[0] = 0;
        Gen();
        m = ParseMove(line);
        if (m == -1 || !MakeMove(move_list[m].start, move_list[m].dest)) 
        {
            fprintf(stderr, "Error (unknown command): %s\n", command);
        } 
        else 
        {
            ply = 0;
            Gen();
            if (XboardResult(NN_XBOARD) > 0) 
            {
                NewGame();
                computer_side = EMPTY;
            } 
            else 
            {
                if (computer_side == EMPTY) 
                {
                    computer_side = side;      
                }
            }
            continue;                           
        }
    }
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
    int flag=0;

    SetMaterial();
    if(pawn_mat[WHITE]==0 && pawn_mat[BLACK]==0 && piece_mat[WHITE]<=300 && piece_mat[BLACK]<=300)
    {
            return DRAWN;
    }
    if(pawn_mat[side]==0 && piece_mat[side]==0 && 
        (piece_mat[xside]==500 || piece_mat[xside]==800 || piece_mat[xside]==900))
    {
        if(side==0)
            return BLACK_WIN;
        else
            return WHITE_WIN;
    }
    ply = 0;

    Gen();
    for (i = 0; i < first_move[1]; ++i)
        if (MakeMove(move_list[i].start,move_list[i].dest))
        {
            TakeBack();
            flag=1;
            break;
        }
        if (i == first_move[1] && flag==0)
        {
            if (Attack(xside,kingloc[side]))
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
            return 1;
        }
        if(pawn_mat[BLACK]==0 && piece_mat[BLACK]==0 && pawn_mat[WHITE] + piece_mat[WHITE]>=900)
        {
            return WHITE_WIN;
        }
        if(pawn_mat[WHITE]==0 && piece_mat[WHITE]==0 && pawn_mat[BLACK] + piece_mat[BLACK]>=900)
        {
            return BLACK_WIN;
        }
        if (reps() >= 3)
        {    
            return DRAWN;
        }
        else if (fifty >= 100)
        {
            return DRAWN;
        }
        if(turn>400)
        {
            return DRAWN;
        }
        return 0;
}
/*

<b>XboardResult()</b> is called from <b>xboard</b> and is similar to <b>CheckResult()</b>.

*/
int XboardResult(int nn)
{
    int i;
    int flag=0;

    SetMaterial();
    if(pawn_mat[WHITE]==0 && pawn_mat[BLACK]==0 && piece_mat[WHITE]<=300 && piece_mat[BLACK]<=300)
    {
        printf("1/2-1/2 {Material}\n");
        draws++;
        return 1;
    }
    ply = 0;

    Gen();
    for (i = 0; i < first_move[1]; ++i)
        if (MakeMove(move_list[i].start,move_list[i].dest))
        {
            TakeBack();
            flag=1;
            break;
        }
        if (i == first_move[1] && flag==0)
        {
            if (Attack(xside,kingloc[side]))
            {
                if (side == 0)
                {
                    printf("0-1 {Black mates}\n");
                    black_wins++;
                }
                else
                {
                    printf("1-0 {White mates}\n");
                    white_wins++;
                }
            }
            else
            {
                printf("1/2-1/2 {Stalemate}\n");
                draws++;
            }
            return 1;
        }
        if(nn != NN_XBOARD)
        {
            if(pawn_mat[BLACK]==0 && piece_mat[BLACK]==0 && pawn_mat[WHITE] + piece_mat[WHITE]>=900)
            {
                printf("1-0 {white wins}\n");
                white_wins++;
                return 1;
            }
            if(pawn_mat[WHITE]==0 && piece_mat[WHITE]==0 && pawn_mat[BLACK] + piece_mat[BLACK]>=900)
            {
                printf("0-1 {black wins}\n");
                black_wins++;
                return 1;
            }
            if(turn>400)
            {
                printf("1/2-1/2 {>400 moves}\n");
                draws++;
                return 1;
            }
        }
        if (reps() >= 3)
        {    
            printf("1/2-1/2 {Draw by repetition}\n");
            draws++;
            return 1;
        }
        else if (fifty >= 100)
        {
            printf("1/2-1/2 {Draw by fifty move rule}\n");
            draws++;
            return 1;
        }
        return 0;
}
/*

<b>reps()</b> checks to see if a game is drawn by triple repetition.

*/
int reps()
{
    int r = 0;
    for (int i = hply; i >= hply-fifty; i-=4)
        if (game_list[i].hash == currentkey && game_list[i].lock == currentlock)
            r++;
    return r;
}
/*

<b>LoadDiagram()</b> loads a position from a <b>fen</b> file.

*/
int LoadDiagram(char* file,int num)
{
    int x,n=0;
    static int count=1;
    char ts[200];

    diagram_file = fopen(file, "r");
	
    if (!diagram_file)
    {
        printf("Diagram missing.\n");
        return -1;
    }

    strcpy_s(fen_name,file);

    for(x=0;x<num;x++)
    {
        fgets(ts, 256, diagram_file);
        if(!ts) break;
    }

    for(x=0;x<64;x++)
    {
        board[x]=EMPTY;
        color[x]=EMPTY;
    }
    int c=0,i=0,j;

    while(ts)
    {
        if(ts[c]>='0' && ts[c]<='8')
            i += ts[c]-48;
        if(ts[c]=='\\')
            continue;
        j=Flip[i];

        switch(ts[c])
        {
        case 'K': board[j]=K; color[j]=0;i++;
            kingloc[WHITE]=j;break;
        case 'Q': board[j]=Q;color[j]=0;i++;break;
        case 'R': board[j]=R; color[j]=0;i++;break;
        case 'B': board[j]=B; color[j]=0;i++;break;
        case 'N': board[j]=N; color[j]=0;i++;break;
        case 'P': board[j]=P; color[j]=0;i++;break;
        case 'k': board[j]=K; color[j]=1;i++;

            kingloc[BLACK]=j;break;
        case 'q': board[j]=Q;color[j]=1;i++;break;
        case 'r': board[j]=R; color[j]=1;i++;break;
        case 'b': board[j]=B; color[j]=1;i++;break;
        case 'n': board[j]=N; color[j]=1;i++;break;
        case 'p': board[j]=P; color[j]=1;i++;break;
        }
        c++;
        if(ts[c]==' ')
            break;
        if(i>63)
            break;
    }
    if(ts[c]==' ' && ts[c+2]==' ')
    {
        if(ts[c+1]=='w')
        {
            side=0;xside=1;
        }
        if(ts[c+1]=='b')
        {
            side=1;xside=0;
        }
    }

    game_list[0].castle_q[WHITE] = 0;
    game_list[0].castle_q[BLACK] = 0;
    game_list[0].castle_k[WHITE] = 0;
    game_list[0].castle_k[BLACK] = 0;

    while(ts[c])
    {
        switch(ts[c])
        {
        case '-': break;
        case 'K':if(board[E1]==K && color[E1]==WHITE) 
                     game_list[0].castle_q[WHITE] = 1;break;
        case 'Q':if(board[E1]==K && color[E1]==WHITE) 
                     game_list[0].castle_q[BLACK] = 1;break;
        case 'k':if(board[E8]==K && color[E8]==BLACK) 
                     game_list[0].castle_k[WHITE] = 1;break;
        case 'q':if(board[E8]==K && color[E8]==BLACK) 
                     game_list[0].castle_k[BLACK] = 1;break;
        default:break;
        }
        c++;
    }

    CloseDiagram();
    DisplayBoard();
    NewPosition();
    Gen();
    printf(" diagram # %d \n",num+count);
    count++;
    if(side==0)
        printf("White to move\n");
    else
        printf("Black to move\n");
    printf(" %s \n",ts);
    return 0;
}
/*

<b>CloseDiagram()</b> is used with <b>LoadDiagram()</b>.

*/
void CloseDiagram()
{
    if (diagram_file)
        fclose(diagram_file);
    diagram_file = NULL;
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
    player[WHITE] =0;
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
    pawn_mat[WHITE]=0;
    pawn_mat[BLACK]=0;
    piece_mat[WHITE]=0;
    piece_mat[BLACK]=0;
    for(int x=0;x<64;x++)
    {
        if(board[x] != EMPTY)
        {
            if(board[x] == K)
                kingloc[color[x]] = x;
            else
            {
                if(board[x]==P)
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
char *MoveString(int start,int dest,int promote)
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

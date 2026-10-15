#include "globals.h"

#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;

int input_list[32];
int list_count;
int index[2][2][6][64];

void SetIndex();
void SetInput(const int s, const int ep);

float ForwardProg(const int s, const int ep);
void BackProg(const int s, const float target, const float output, const int ep);
int NN_Play(const int s);

float GetLearningRate();
void SetLearningRate(const float rate);
void ReduceLearningRate();
int GetEpFile();

float ClampOutput(float output);

const int PIECES = 6;
const int SQUARES = 64;
const int INPUT_SIZE = 780;
const int HIDDEN_SIZE = 64;

const float SCALE_CP = 400.0f;

const int CASTLE_STM_K = 768;
const int CASTLE_STM_Q = 769;
const int CASTLE_OTM_K = 770;
const int CASTLE_OTM_Q = 771;

const int ep_file[8] = { 772, 773, 774, 775, 776, 777, 778, 779 };

int total_error = 0;
float errors[1000000];
const int MAX_ERRORS = 1000000;

float input_weight[INPUT_SIZE][HIDDEN_SIZE];
float hidden_bias[HIDDEN_SIZE];
float hidden_weight[HIDDEN_SIZE];
float hidden[HIDDEN_SIZE];
float output_bias;

float learning_rate = 0.002f;

float GetLearningRate()
{
    return learning_rate;
}

void SetLearningRate(const float rate)
{
    learning_rate *= rate;
}

void ReduceLearningRate()
{
    learning_rate *= 0.97f;
    cout << "learning_rate" << learning_rate;
}

int NN_Train(const int s, const int target)
{
    int ep = GetEpFile();
    float output = ForwardProg(s, ep);
    BackProg(s, target, output, ep);
    output = ClampOutput(output);
    return (int)(SCALE_CP * output);
}

int NN_Play(const int s)
{
    int ep = GetEpFile();
    float output = ForwardProg(s, ep);
    output = ClampOutput(output);
    return (int)(SCALE_CP * output);
}

void SetIndex()
{
    int count = 0;

    for (int s = 0; s < 2; s++)
        for (int x = 0; x < PIECES; x++)
            for (int y = 0; y < SQUARES; y++)
                index[0][s][x][y] = count++;

    for (int s = 0; s < 2; s++)
        for (int x = 0; x < PIECES; x++)
            for (int y = 0; y < SQUARES; y++)
                index[1][s][x][y] = index[0][!s][x][Flip[y]];
}

void SetInput(const int s, const int ep)
{
    list_count = 0;

    const bool any_castle =
        game_list[hply].castle_k[WHITE] || game_list[hply].castle_q[WHITE] ||
        game_list[hply].castle_k[BLACK] || game_list[hply].castle_q[BLACK];

    const bool king_on_right = (col[kingloc[s]] > 3);
    const bool king_on_left = (!king_on_right) && !any_castle && (ep == -1);

    if (s == 0)
    {
        if (king_on_left)
        {
            for (int sq = 0; sq < 64; sq++)
                if (board[sq] != EMPTY)
                    input_list[list_count++] = index[s][color[sq]][board[sq]][Mirror[sq]];
        }
        else
        {
            for (int sq = 0; sq < 64; sq++)
                if (board[sq] != EMPTY)
                    input_list[list_count++] = index[s][color[sq]][board[sq]][sq];
        }
    }
    else
    {
        if (king_on_left)
        {
            for (int sq = 0; sq < 64; sq++)
                if (board[sq] != EMPTY)
                    input_list[list_count++] = index[s][!color[sq]][board[sq]][Rotate[sq]];
        }
        else
        {
            for (int sq = 0; sq < 64; sq++)
                if (board[sq] != EMPTY)
                    input_list[list_count++] = index[s][!color[sq]][board[sq]][Flip[sq]];
        }
    }
}

float ForwardProg(const int s, const int ep)
{
    SetInput(s, ep);

    for (int i = 0; i < HIDDEN_SIZE; ++i)
    {
        float sum = hidden_bias[i];
        for (int j = 0; j < list_count; ++j)
        {
            sum += input_weight[input_list[j]][i];
        }
        if (game_list[hply].castle_k[s]) sum += input_weight[CASTLE_STM_K][i];
        if (game_list[hply].castle_q[s]) sum += input_weight[CASTLE_STM_Q][i];
        if (game_list[hply].castle_k[!s]) sum += input_weight[CASTLE_OTM_K][i];
        if (game_list[hply].castle_q[!s]) sum += input_weight[CASTLE_OTM_Q][i];

        if (ep > -1) 
        {
            sum += input_weight[ep_file[ep]][i];
        }

        hidden[i] = (sum > 0.0f) ? sum : sum * 0.01f; 
    }

    float output = output_bias;
    for (int i = 0; i < HIDDEN_SIZE; i++)
        output += hidden_weight[i] * hidden[i]; 
    return output;
}

void BackProg(const int s, const float target, const float output, const int ep)
{
    float raw_cp = target;
    if (raw_cp > SCALE_CP)  raw_cp = SCALE_CP;
    if (raw_cp < -SCALE_CP) raw_cp = -SCALE_CP;
    float target_n = raw_cp / SCALE_CP;

    float error = output - target_n; 
    float abs_e = fabsf(error);
    const float huber_delta = 0.25f; 
    float delta_out;

    if (abs_e <= huber_delta) 
    {
        delta_out = error;
    }
    else 
    {
        delta_out = (error > 0.0f) ? huber_delta : -huber_delta;
    }

    delta_out *= 1.0f / float(list_count);

    float delta_hidden[HIDDEN_SIZE];
    for (int i = 0; i < HIDDEN_SIZE; ++i)
    {
        float delta_active = (hidden[i] > 0.0f) ? 1.0f : 0.01f; 
        delta_hidden[i] = delta_out * hidden_weight[i] * delta_active;
    }
    for (int i = 0; i < HIDDEN_SIZE; ++i)
        hidden_weight[i] -= learning_rate * (delta_out * hidden[i]);
    output_bias -= learning_rate * delta_out;

    for (int i = 0; i < HIDDEN_SIZE; ++i)
    {
        for (int j = 0; j < list_count; ++j)
            input_weight[input_list[j]][i] -= learning_rate * delta_hidden[i];

        hidden_bias[i] -= learning_rate * delta_hidden[i];

        if (game_list[hply].castle_k[s])
            input_weight[CASTLE_STM_K][i] -= learning_rate * delta_hidden[i];
        if (game_list[hply].castle_q[s])
            input_weight[CASTLE_STM_Q][i] -= learning_rate * delta_hidden[i];
        if (game_list[hply].castle_k[!s])
            input_weight[CASTLE_OTM_K][i] -= learning_rate * delta_hidden[i];
        if (game_list[hply].castle_q[!s])
            input_weight[CASTLE_OTM_Q][i] -= learning_rate * delta_hidden[i];

        if (ep > -1)
        {
            input_weight[ep_file[ep]][i] -=
                learning_rate * delta_hidden[i];
        }
    }
    float error_cp = SCALE_CP * error;
    if (total_error < MAX_ERRORS) errors[total_error++] = error_cp;
}

float ClampOutput(float output)
{
    if (output > 1.0f) output = 1.0f;
    if (output < -1.0f) output = -1.0f;
    return output;
}

void InitWeights()
{
    for (int i = 0; i < INPUT_SIZE; ++i)
    {
        for (int j = 0; j < HIDDEN_SIZE; ++j)
        {
            input_weight[i][j] = ((float)rand() / RAND_MAX) * 2 - 1;
            input_weight[i][j] *= sqrt(2.0f / INPUT_SIZE); 
        }
    }
    for (int j = 0; j < HIDDEN_SIZE; ++j)
    {
        hidden_bias[j] = 0.01f;
    }

    for (int j = 0; j < HIDDEN_SIZE; ++j)
    {
        hidden_weight[j] = ((float)rand() / RAND_MAX) * 2 - 1;
        hidden_weight[j] *= sqrt(2.0f / HIDDEN_SIZE); 
    }
    output_bias = 0.0;

    SetIndex();
}

int SaveWeights(int epochs, U64 time)
{
    std::ofstream file("weights64.txt");
    if (!file) {
        std::cerr << "Could not open weights64.txt for writing." << endl;
        return 1;
    }

    file.imbue(std::locale::classic());
    file << epochs << '\n';
    file << time << '\n';

    for (int x = 0; x < INPUT_SIZE; ++x)
        for (int y = 0; y < HIDDEN_SIZE; ++y)
            file << input_weight[x][y] << '\n';

    for (int y = 0; y < HIDDEN_SIZE; ++y)
        file << hidden_weight[y] << '\n';

    for (int y = 0; y < HIDDEN_SIZE; ++y)
        file << hidden_bias[y] << '\n';

    file << output_bias << '\n';

    return file ? 0 : 2;
}

int LoadWeights()
{
    std::ifstream file("weights64.txt");
    if (!file) {
        std::cerr << "weights64.txt not found!" << endl;
        return 1;
    }

    file.imbue(std::locale::classic());

    int epochs;

    if (!(file >> epochs)) {
        cerr << "Failed to read epochs header." << endl;
        return 2;
    }

    for (int x = 0; x < INPUT_SIZE; ++x)
        for (int y = 0; y < HIDDEN_SIZE; ++y)
            if (!(file >> input_weight[x][y])) return 3;

    for (int y = 0; y < HIDDEN_SIZE; ++y)
        if (!(file >> hidden_weight[y])) return 4;

    for (int y = 0; y < HIDDEN_SIZE; ++y)
        if (!(file >> hidden_bias[y])) return 5;

    if (!(file >> output_bias)) return 6;

    return epochs;
}

int GetEpFile()
{
    if (hply <= 0) return -1;

    int ep = GetHistoryTo(hply - 1);

    if (board[ep] == 0 && abs(GetHistoryFrom(hply - 1) - ep) == 16)
    {
        if (col[ep] > 0 && color[ep - 1] == side && board[ep - 1] == P)
        {
            return col[ep];
        }
        if (col[ep] < 7 && color[ep + 1] == side && board[ep + 1] == P)
        {
            return col[ep];
        }
    }
    return -1;
}

#include "globals.h"//18/10

#if LAYERS == 2

#include <fstream>
#include <iomanip>
#include <iostream>
#include <assert.h>
#include <math.h>

int total_epochs;
int total_updates;
U64 total_time;

//extern float learning_rate;
#define SCALE_CP 800.0f
#define LEAKY_SLOPE 0.01f

const int INPUT_SIZE = 768;
const int H1_SIZE = 128;
const int H2_SIZE = 64;

#define PIECES 6
#define SQUARES 64

void GetAverage2();//

/* Model params */
float input_weight[INPUT_SIZE][H1_SIZE]; // feature-major for easy incremental updates
float input2_weight[H1_SIZE][H2_SIZE];
float hidden1_bias[H1_SIZE];
float hidden2_bias[H2_SIZE];
float output_weight[H2_SIZE];
static float output_bias;

/* Cached activations from ForwardProg (fill these there) */
float cache_h1[H1_SIZE];
float cache_h2[H2_SIZE];

/* Sparse feature plumbing from SetInput */
static int input_list[32];
static int list_count;

/* Optional error logging */
const int MAX_ERRORS = 1000000;
static float errors[MAX_ERRORS];
static int total_error;

int input_index[2][2][6][64];
int visits[6][64];

void SetIndex();
void SetInput(int s);

float ForwardProg(int s);
void BackProg(int s, float target, float output);
int NN_Play(int s);

void SetLearningRate(float rate);
void ResetLearningRate();
void ReduceLearningRate();

static float learning_rate = 0.002f;//0.005 0.01

#define TRAIN_SCALE_CP 400.0f
#define EVAL_SCALE_CP  100.0f   // max non-material contribution in cp

// (optional) activations/accumulator for incremental eval
float acc1[H1_SIZE];            // sum of active input_weight columns
/*
float GetLearningRate()
{
    return learning_rate;
}
*/
/*
void SetLearningRate(float rate)
{
    learning_rate *= rate;
    printf("learning_rate %f \n", learning_rate);
}
*/
/*
void ResetLearningRate()
{
    learning_rate = 0.001f;
}
*/
/*
void ReduceLearningRate()
{
    learning_rate *= 0.97f;
    printf("learning_rate %f \n",learning_rate);
}
*/

// Activation functions
float relu(float x) {
    return x > 0 ? x : 0;
}
float relu_deriv(float x) {
    return x > 0 ? 1.0f : 0.0f;
}

void SetInput(int s)
{
    list_count = 0;

    const bool any_castle =
        game_list[hply].castle_k[0] || game_list[hply].castle_q[0] ||
        game_list[hply].castle_k[1] || game_list[hply].castle_q[1];

    const bool king_on_right = (col[kingloc[s]] > 3); // files e h
    const bool do_lr_mirror = (!king_on_right) && !any_castle;

    if (s == 0) {
        if (do_lr_mirror) {
            for (int sq = 0; sq < 64; ++sq)
                if (board[sq] != EMPTY)
                {
                    int n = input_index[s][color[sq]][board[sq]][Mirror[sq]];
                    assert(n >= 0 && n < 780);
                    input_list[list_count++] = input_index[s][color[sq]][board[sq]][Mirror[sq]];
                }

        }
        else {
            for (int sq = 0; sq < 64; ++sq)
                if (board[sq] != EMPTY)
                {
                    int n = input_index[s][color[sq]][board[sq]][sq];
                    assert(n >= 0 && n < 768);
                    input_list[list_count++] = input_index[s][color[sq]][board[sq]][sq];
                }
        }
    }
    else {
        if (do_lr_mirror) {
            for (int sq = 0; sq < 64; ++sq)
                if (board[sq] != EMPTY)
                {
                    int n = input_index[s][!color[sq]][board[sq]][Rotate[sq]];
                    assert(n >= 0 && n < 768);
                    input_list[list_count++] = input_index[s][!color[sq]][board[sq]][Rotate[sq]];
                }
        }
        else {
            for (int sq = 0; sq < 64; ++sq)
                if (board[sq] != EMPTY)
                {
                    int n = input_index[s][!color[sq]][board[sq]][Flip[sq]];
                    assert(n >= 0 && n < 780);
                    input_list[list_count++] = input_index[s][!color[sq]][board[sq]][Flip[sq]];
                }
        }
    }
}

float ClampOutput(float output)
{
    if (output > 1.0f) output = 1.0f;
    if (output < -1.0f) output = -1.0f;
    return output;
}

int NN_Play(int s)
{
    float output = ForwardProg(s);
    output = ClampOutput(output);//
    return (int)(SCALE_CP * output);
}

int NN_Train(int s, float target)
{
    float output = ForwardProg(s);
    BackProg(s, target, output);
    output = ClampOutput(output);//
    return (int)(SCALE_CP * output);
}

float ForwardProg(int s)
{
    /* Build sparse features from side-to-move POV */

    SetInput(s); /* fills input_list[0..list_count-1] */

    /* ---- Layer 1: accumulate feature embeddings, then LeakyReLU ---- */
    for (int i = 0; i < H1_SIZE; ++i)
    {
        float sum = hidden1_bias[i];

        /* sum active feature vectors */
        for (int k = 0; k < list_count; ++k)
            sum += input_weight[input_list[k]][i];

        cache_h1[i] = sum;
    }

    for (int i = 0; i < H1_SIZE; ++i) {
        /* LeakyReLU */
        float sum = cache_h1[i];
        if (sum < 0.0f)
            cache_h1[i] = sum * LEAKY_SLOPE;
        else
            cache_h1[i] = sum;
    }

    /* ---- Layer 2: h2 = LeakyReLU(b2 + input2_weight^T * h1) ----
       input2_weight is stored as [H1][H2] so inner loop is contiguous. */

    for (int j = 0; j < H2_SIZE; ++j)
        cache_h2[j] = hidden2_bias[j];

    for (int i = 0; i < H1_SIZE; ++i)
    {
        const float xi = cache_h1[i];

        for (int j = 0; j < H2_SIZE; ++j)
            cache_h2[j] += input2_weight[i][j] * xi;
    }
    for (int j = 0; j < H2_SIZE; ++j)
        if (cache_h2[j] < 0.0f)
            cache_h2[j] *= LEAKY_SLOPE;

    /* ---- Output: linear head ---- */
    float output = output_bias;
    for (int j = 0; j < H2_SIZE; ++j)
        output += output_weight[j] * cache_h2[j];

    return output;
}

void BackProg(int s, float target, float output)
{
    /* ----- 1) Target scaling and Huber loss grad wrt output ----- */
    float raw_cp = target;
    if (raw_cp > SCALE_CP) raw_cp = SCALE_CP;
    if (raw_cp < -SCALE_CP) raw_cp = -SCALE_CP;

    float target_n = raw_cp / SCALE_CP;

    float error = output - target_n;      /* dL/dy uses this sign convention */
    float abs_e = fabsf(error);

    const float huber_delta = 0.25f;
    float dL_dy = error;

    if (abs_e > huber_delta)
    {
        if (error > 0.0f)
            dL_dy = huber_delta;
        else
            dL_dy = -huber_delta;
    }

    /* Optional normalization by number of active features */
    //float norm = (list_count > 0) ? (1.0f / (float)list_count) : 1.0f;
    //dL_dy *= norm;

    /* ----- 2) Backprop to layer-2 (LeakyReLU derivative) ----- */
    float delta2[H2_SIZE];
    for (int j = 0; j < H2_SIZE; ++j) {
        float dact = LEAKY_SLOPE;
        if (cache_h2[j] > 0.0f)
            dact = 1.0f;
        delta2[j] = dL_dy * output_weight[j] * dact;
    }

    /* ----- 3) Backprop to layer-1 (through input2_weight, then LeakyReLU) ----- */
    float delta1[H1_SIZE];
    for (int i = 0; i < H1_SIZE; ++i) {
        float acc = 0.0f;
        for (int j = 0; j < H2_SIZE; ++j)
            acc += input2_weight[i][j] * delta2[j];
        float dact = LEAKY_SLOPE;

        if (cache_h1[i] > 0.0f)
            dact = 1.0f;
        delta1[i] = acc * dact;
    }

    static int debug_count = 0;
/*
    if (++debug_count >= 10000)
    {
        debug_count = 0;

        float avg_d2 = 0.0f;
        float avg_d1 = 0.0f;

        for (int j = 0; j < H2_SIZE; ++j)
            avg_d2 += fabsf(delta2[j]);

        for (int i = 0; i < H1_SIZE; ++i)
            avg_d1 += fabsf(delta1[i]);

        avg_d2 /= H2_SIZE;
        avg_d1 /= H1_SIZE;

        printf("grad: out %.8f  d2 %.8f  d1 %.8f\n",
            fabsf(dL_dy), avg_d2, avg_d1);
    }
    */

    /* ----- 4) Update output layer ----- */
    float lr_dL = learning_rate * dL_dy;

    for (int j = 0; j < H2_SIZE; ++j)
        output_weight[j] -= lr_dL * cache_h2[j];

    output_bias -= learning_rate * dL_dy;

    /* ----- 5) Update input2_weight and hidden2_bias ----- */
    for (int i = 0; i < H1_SIZE; ++i) {
        const float xi = cache_h1[i];
        for (int j = 0; j < H2_SIZE; ++j)
            input2_weight[i][j] -= learning_rate * xi * delta2[j];
    }
    for (int j = 0; j < H2_SIZE; ++j)
        hidden2_bias[j] -= learning_rate * delta2[j];

    /* ----- 6) Update input_weight and hidden1_bias (sparse features) ----- */
    for (int i = 0; i < H1_SIZE; ++i)
        hidden1_bias[i] -= learning_rate * delta1[i];

    /* piece/square active features */
    for (int k = 0; k < list_count; ++k) {
        int f = input_list[k];

        for (int i = 0; i < H1_SIZE; ++i)
            input_weight[f][i] -= learning_rate * delta1[i];
    }

    /* ----- 7) Optional error logging in centipawns ----- */
    if (total_error < MAX_ERRORS)
    {
        float error_cp = SCALE_CP * error;
        errors[total_error++] = error_cp;
    }
}

static inline float RandomRange(float low, float high) 
{
    return low + (high - low) * ((float)rand() / (float)RAND_MAX);
}

void InitWeights(void)
{
    // Random initial weights for leaky-ReLU layers
    const float limit1 = sqrtf(6.0f / (float)INPUT_SIZE); // for input_weight
    const float limit2 = sqrtf(6.0f / (float)H1_SIZE);    // for input2_weight

    // input_weight: [INPUT_SIZE x H1_SIZE]
    for (int i = 0; i < INPUT_SIZE; ++i)
        for (int j = 0; j < H1_SIZE; ++j)
            input_weight[i][j] = RandomRange(-limit1, limit1);

    // b1
    for (int j = 0; j < H1_SIZE; ++j)
        hidden1_bias[j] = 0.0f; // or 0.01f if you prefer leaky-ReLU bias

    // input2_weight: [H1_SIZE x H2_SIZE]
    for (int j = 0; j < H1_SIZE; ++j)
        for (int k = 0; k < H2_SIZE; ++k)
            input2_weight[j][k] = RandomRange(-limit2, limit2);

    // b2
    for (int k = 0; k < H2_SIZE; ++k)
        hidden2_bias[k] = 0.0f;

    // Output head: small init (keeps early outputs near 0)
    const float limit_out = sqrtf(1.0f / (float)H2_SIZE);
    for (int k = 0; k < H2_SIZE; ++k)
        output_weight[k] = RandomRange(-limit_out, limit_out);
    output_bias = 0.0f;

    SetIndex();
}

/*
void InitWeights(void)
{
    const float scale1 = sqrtf(2.0f / INPUT_SIZE);

    for (int i = 0; i < INPUT_SIZE; ++i)
        for (int j = 0; j < H1_SIZE; ++j)
            input_weight[i][j] =
            RandomRange(-1.0f, 1.0f) * scale1;

    for (int j = 0; j < H1_SIZE; ++j)
        hidden1_bias[j] = 0.01f;

    const float scale2 = sqrtf(2.0f / H1_SIZE);

    for (int i = 0; i < H1_SIZE; ++i)
        for (int j = 0; j < H2_SIZE; ++j)
            input2_weight[i][j] =
            RandomRange(-1.0f, 1.0f) * scale2;

    for (int j = 0; j < H2_SIZE; ++j)
        hidden2_bias[j] = 0.01f;

    const float scale_out = sqrtf(2.0f / H2_SIZE);

    for (int j = 0; j < H2_SIZE; ++j)
        output_weight[j] =
        RandomRange(-1.0f, 1.0f) * scale_out;

    output_bias = 0.0f;

    SetIndex();
}
*/

void SetIndex()
{
    int count = 0;

    for (int s = 0; s < 2; s++)
        for (int x = 0; x < PIECES; x++)
            for (int y = 0; y < SQUARES; y++)
                input_index[0][s][x][y] = count++;

    for (int s = 0; s < 2; s++)
        for (int x = 0; x < PIECES; x++)
            for (int y = 0; y < SQUARES; y++)
                input_index[1][s][x][y] = input_index[0][s ^ 1][x][Flip[y]];
}

float GetAverage()
{
    GetAverage2();

    float sum = 0;
    float average = 0;

    for (int x = 0; x < total_error; x++)
    {
        sum += errors[x];
    }
    if (total_error > 0)
        average = sum / total_error;
    total_error = 0;
    return average;
}

void GetAverage2()
{
    float sum = 0;
    float abs_sum = 0;

    for (int x = 0; x < total_error; x++)
    {
        sum += errors[x];
        abs_sum += fabsf(errors[x]);
    }

    if (total_error > 0)
    {
        printf("mean error %.1f\n", sum / total_error);
        printf("mean absolute error %.1f\n",
            abs_sum / total_error);
    }

    //total_error = 0;
}

int GetTotalEpochs()
{
    return total_epochs;
}

int GetTotalUpdates()
{
    return total_updates;
}

U64 GetTotalTime()
{
    return total_time;
}

int SaveWeights(int epochs, U64 time)
{
    ofstream file("weights128.txt");
    if (!file) {
        cerr << "Could not open weights128.txt for writing.\n";
        return 1;
    }

    file.imbue(locale::classic());
    file << setprecision(9);

    // Header
    file << epochs << '\n';
    file << time << '\n';

    // ---- 1. input_weight (780 x 128)
    for (int x = 0; x < INPUT_SIZE; ++x)
        for (int y = 0; y < H1_SIZE; ++y)
            file << input_weight[x][y] << '\n';

    // ---- 2. input2_weight (128 x 64)
    for (int x = 0; x < H1_SIZE; ++x)
        for (int y = 0; y < H2_SIZE; ++y)
            file << input2_weight[x][y] << '\n';

    // ---- 3. hidden1_bias (128)
    for (int i = 0; i < H1_SIZE; ++i)
        file << hidden1_bias[i] << '\n';

    // ---- 4. hidden2_bias (64)
    for (int j = 0; j < H2_SIZE; ++j)
        file << hidden2_bias[j] << '\n';

    // ---- 5. output_weight (64)
    for (int j = 0; j < H2_SIZE; ++j)
        file << output_weight[j] << '\n';

    // ---- 6. output_bias
    file << output_bias << '\n';

    return file ? 0 : 2;
}

int LoadWeights()
{
    ifstream file("weights128.txt");
    if (!file) {
        cerr << "weights128.txt not found!\n";
        return -1;
    }

    file.imbue(locale::classic());

    //int epochs;
    if (!(file >> total_epochs)) return -2;
    if (!(file >> total_time)) return -10;

    // ---- 1. input_weight
    for (int x = 0; x < INPUT_SIZE; ++x)
        for (int y = 0; y < H1_SIZE; ++y)
            if (!(file >> input_weight[x][y])) return -3;

    // ---- 2. input2_weight
    for (int x = 0; x < H1_SIZE; ++x)
        for (int y = 0; y < H2_SIZE; ++y)
            if (!(file >> input2_weight[x][y])) return -4;

    // ---- 3. hidden1_bias
    for (int i = 0; i < H1_SIZE; ++i)
        if (!(file >> hidden1_bias[i])) return -5;

    // ---- 4. hidden2_bias
    for (int j = 0; j < H2_SIZE; ++j)
        if (!(file >> hidden2_bias[j])) return -6;

    // ---- 5. output_weight
    for (int j = 0; j < H2_SIZE; ++j)
        if (!(file >> output_weight[j])) return -7;

    // ---- 6. output_bias
    if (!(file >> output_bias)) return -8;

    return 0;
}

#endif
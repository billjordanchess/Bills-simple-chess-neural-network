#include "stdlib.h"
#include "stdio.h"
#include "conio.h"
#include "math.h"
#include "string"
#include "iostream"
#include "fstream"
#include "iomanip"
#include "cmath"
#include "cstdlib"
#include "ctime"
#include "cstring"

#include "globals.h"//18/10

const int INPUT_SIZE = 30;
const int HIDDEN_SIZE = 16;
const int OUTPUT_SIZE = 1;

const float LEARNING_RATE = 0.01f;

const int EPOCHS = 1000;
const int TRAINING_SAMPLES = 1000;

float ReLU(float x) {
    return x > 0 ? x : 0;
}

float ReLU_deriv(float x) {
    return x > 0 ? 1.0f : 0.0f;
}

float tanh_activation(float x) {
    return tanh(x);
}

float tanh_derivative_from_output(float y) {
    return 1.0f - y * y;
}

float rand_weight() {
    return (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
}

void InputTest(float input[30], float& target) 
{
    int material[2] = { 0, 0 };
    int offset[2] = { 0, 15 };  // 0 for white, 15 for black
    for (int i = 0; i < 30; ++i)
        input[i] = 0;
    for (int side = 0; side < 2; ++side) {
        int base = offset[side];
        for (int i = 0; i < 8; ++i) { // Pawns
            if (rand() % 100 < 50) {
                input[base + i] = 1.0f;
                material[side] += 100;
            }
        }
        for (int i = 8; i < 12; ++i) { // Knights and Bishops
            if (rand() % 100 < 50) {
                input[base + i] = 1.0f;
                material[side] += 300;
            }
        }
        for (int i = 12; i < 14; ++i) { // Rooks
            if (rand() % 100 < 50) {
                input[base + i] = 1.0f;
                material[side] += 500;
            }
        }
        if (rand() % 100 < 50) { // Queen
            input[base + 14] = 1.0f;
            material[side] += 900;
        }
    }
    int diff = material[0] - material[1];
    target = tanh_activation(static_cast<float>(diff) / 400.0f);  // Soft clamping via tanh
}

int TestMat() 
{
    srand(static_cast<unsigned>(time(0)));
    float input_weight[HIDDEN_SIZE][INPUT_SIZE];
    float hidden_bias[HIDDEN_SIZE];
    float hidden_weight[OUTPUT_SIZE][HIDDEN_SIZE];
    float output_bias[OUTPUT_SIZE];

    for (int i = 0; i < HIDDEN_SIZE; ++i)
    {
        for (int j = 0; j < INPUT_SIZE; ++j)
            input_weight[i][j] = rand_weight() * 0.1f;
        hidden_bias[i] = 0.0f;
    }
    for (int i = 0; i < OUTPUT_SIZE; ++i)
    {
        for (int j = 0; j < HIDDEN_SIZE; ++j)
            hidden_weight[i][j] = rand_weight() * 0.1f;
        output_bias[i] = 0.0f;
    }
    float input[INPUT_SIZE];
    float target;
    for (int epoch = 0; epoch <= EPOCHS; ++epoch) {
        float loss_total = 0.0f;
        for (int s = 0; s < TRAINING_SAMPLES; ++s) {
            InputTest(input, target);
            float hidden[HIDDEN_SIZE];
            float hidden_raw[HIDDEN_SIZE];
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                float sum = hidden_bias[i];
                for (int j = 0; j < INPUT_SIZE; ++j)
                    sum += input_weight[i][j] * input[j];
                hidden_raw[i] = sum;
                hidden[i] = ReLU(sum);
            }
            float output_raw = output_bias[0];
            for (int j = 0; j < HIDDEN_SIZE; ++j)
                output_raw += hidden_weight[0][j] * hidden[j];
            float output = tanh_activation(output_raw);
            float error = output - target;
            loss_total += error * error;
            float delta_output = 2 * error * tanh_derivative_from_output(output);
            float delta_hidden[HIDDEN_SIZE];
            for (int i = 0; i < HIDDEN_SIZE; ++i)
                delta_hidden[i] = delta_output * hidden_weight[0][i] * ReLU_deriv(hidden_raw[i]);
            for (int i = 0; i < HIDDEN_SIZE; ++i)
                hidden_weight[0][i] -= LEARNING_RATE * delta_output * hidden[i];
            output_bias[0] -= LEARNING_RATE * delta_output;
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                for (int j = 0; j < INPUT_SIZE; ++j)
                    input_weight[i][j] -= LEARNING_RATE * delta_hidden[i] * input[j];
                hidden_bias[i] -= LEARNING_RATE * delta_hidden[i];
            }
        }
        if (epoch % 10 == 0) {
            InputTest(input, target);
            float hidden[HIDDEN_SIZE];
            float output_raw = output_bias[0];
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                float sum = hidden_bias[i];
                for (int j = 0; j < INPUT_SIZE; ++j)
                    sum += input_weight[i][j] * input[j];
                hidden[i] = ReLU(sum);
                output_raw += hidden_weight[0][i] * hidden[i];
            }
            float output = tanh_activation(output_raw);
            cout << "Epoch " << epoch
                << ", Avg Loss: " << fixed << setprecision(6) << loss_total / TRAINING_SAMPLES
                << ", Target: " << target
                << ", Output: " << output << "\n";
        }
    }
    return 0;
}
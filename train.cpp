#include <fstream>
#include <iomanip>
#include <iostream>

#include "globals.h"

int input_list[32];
int list_count;
int index[2][2][6][64];
int visits[6][64];

void SetIndex();
void SetInput(int s, int ep);

int NN_Match(int s);
float ForwardProg(int s, int ep);
void BackProg(int s, float target, float output, int ep);
int NN_Play(int s);
int NN_Opponent(int s);

void UpdateBest();
void UpdateWeights();
void UpdateOldWeights();

float GetLearningRate();
void SetLearningRate(float rate);
void ResetLearningRate();
void ReduceLearningRate();
int GetEpFile();

#define PIECES 6
#define SQUARES 64
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

float old_input_weight[INPUT_SIZE][HIDDEN_SIZE];
float old_hidden_bias[HIDDEN_SIZE];
float old_hidden_weight[HIDDEN_SIZE];
float old_output_bias;

float best_input_weight[INPUT_SIZE][HIDDEN_SIZE];
float best_hidden_bias[HIDDEN_SIZE];
float best_hidden_weight[HIDDEN_SIZE];
float best_output_bias;

float learning_rate = 0.02f;//0.005–0.01

float GetLearningRate()
{
    return learning_rate;
}

void SetLearningRate(float rate)
{
	learning_rate *= rate;
}

void ResetLearningRate()
{
	learning_rate = 0.001f;
}

void ReduceLearningRate()
{
	learning_rate *= 0.97f;
	printf("learning_rate %f \n",learning_rate);
}

// Activation functions
float relu(float x) {
	return x > 0 ? x : 0;
}
float relu_deriv(float x) {
	return x > 0 ? 1.0f : 0.0f;
}

float tanh_activation(float x) {
	return std::tanh(x);
}
float tanh_derivative_from_output(float y) {
	return 1.0f - y * y;
}

// Weight initialisation helper
float rand_weight() {
	return (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
}

void SetInput(int s, int ep)
{
	list_count = 0;

	const bool any_castle =
		game_list[hply].castle_k[WHITE] || game_list[hply].castle_q[WHITE] ||
		game_list[hply].castle_k[BLACK] || game_list[hply].castle_q[BLACK];

	const bool king_on_right = (col[kingloc[s]] > 3); // files e–h
	const bool do_lr_mirror  = (!king_on_right) && !any_castle && (ep == -1);

	if (s == 0) {
		if (do_lr_mirror) {
			for (int sq = 0; sq < 64; ++sq)
				if (board[sq] != EMPTY)
					input_list[list_count++] = index[s][color[sq]][board[sq]][Mirror[sq]];
		} else {
			for (int sq = 0; sq < 64; ++sq)
				if (board[sq] != EMPTY)
					input_list[list_count++] = index[s][color[sq]][board[sq]][sq];
		}
	} else {
		if (do_lr_mirror) {
			for (int sq = 0; sq < 64; ++sq)
				if (board[sq] != EMPTY)
					input_list[list_count++] = index[s][!color[sq]][board[sq]][Rotate[sq]];
		} else {
			for (int sq = 0; sq < 64; ++sq)
				if (board[sq] != EMPTY)
					input_list[list_count++] = index[s][!color[sq]][board[sq]][Flip[sq]];
		}
	}
}

int NN_Match(int s) 
{
	int ep = GetEpFile();
	float output = ForwardProg(s, ep);

	return (int)(SCALE_CP * output);
}

int NN_Train(int s, float target) 
{
	int ep = GetEpFile();
	float output = ForwardProg(s, ep);

	BackProg(s,target, output, ep);

	// ---- Return eval in centipawns (non-material) ----
	return (int)(SCALE_CP * output);
}

float ForwardProg(int s, int ep) 
{   
	SetInput(s, ep); // builds features from 'side' perspective (non-material target)

	// ---- Forward: hidden (ReLU) ----
	for (int i = 0; i < HIDDEN_SIZE; ++i) 
	{
		float sum = hidden_bias[i];
		for (int j = 0; j < list_count; ++j) 
		{
			sum += input_weight[input_list[j]][i];
		}
		if (game_list[hply].castle_k[s]) sum += input_weight[CASTLE_STM_K][i];
		if (game_list[hply].castle_q[s]) sum += input_weight[CASTLE_STM_Q][i];

		if (ep > -1) {
			// no flip here under strategy A
			sum += input_weight[ep_file[ep]][i];
		}

		hidden[i] = (sum > 0.0f) ? sum : sum * 0.01f; // leaky ReLU
	}

	// ---- Forward: output (LINEAR) ----
	float output = output_bias;
	for (int i = 0; i < HIDDEN_SIZE; i++)
		output += hidden_weight[i] * hidden[i]; // linear head
	return output;
}

void BackProg(int s, float target, float output, int ep)
{
	// ---- Target scaling (linear) ----
	float raw_cp = target;
	if (raw_cp > SCALE_CP)  raw_cp = SCALE_CP;
	if (raw_cp < -SCALE_CP) raw_cp = -SCALE_CP;

	// Model output learns to predict cp / SCALE_CP
	float target_n = raw_cp / SCALE_CP;

	// ---- Loss/gradients (Huber + linear output) ----
	float error  = output - target_n;   // normalised units
	float abs_e  = fabsf(error);
	const float huber_delta = 0.25f;     // switch point (tune 0.5–2.0 if needed)
	float delta_out;

	if (abs_e <= huber_delta) {
		// Quadratic region: grad = error
		delta_out = error;
	} else {
		// Linear region: grad = huber_delta * sign(error)
		delta_out = (error > 0.0f) ? huber_delta : -huber_delta;
	}

	delta_out *= 1.0f / float(list_count);

	// ---- Clamp backprop signal for stability ----
	const float GRAD_CLAMP = 0.2f;
	if (delta_out >  GRAD_CLAMP) delta_out =  GRAD_CLAMP;
	if (delta_out < -GRAD_CLAMP) delta_out = -GRAD_CLAMP;

	// ---- Backprop to hidden (pure ReLU derivative) ----
	float delta_hidden[HIDDEN_SIZE];
	for (int i = 0; i < HIDDEN_SIZE; ++i) {
		float dact = (hidden[i] > 0.0f) ? 1.0f : 0.01f; // assumes forward used leaky ReLU
		delta_hidden[i] = delta_out * hidden_weight[i] * dact;
	}

	// ---- Update output layer ----
	for (int i = 0; i < HIDDEN_SIZE; ++i)
		hidden_weight[i] -= learning_rate * (delta_out * hidden[i]);
	output_bias -= learning_rate * delta_out;

	// ---- Update input layer (binary features: each active gets same delta) ----
	for (int i = 0; i < HIDDEN_SIZE; ++i) {
		// piece/square features
		for (int j = 0; j < list_count; ++j)
			input_weight[input_list[j]][i] -= learning_rate * delta_hidden[i];

		// bias
		hidden_bias[i] -= learning_rate * delta_hidden[i];

		// castling features (stm/otm)
		if (game_list[hply].castle_k[s])   input_weight[CASTLE_STM_K][i]  -= learning_rate * delta_hidden[i];
		if (game_list[hply].castle_q[s])   input_weight[CASTLE_STM_Q][i]  -= learning_rate * delta_hidden[i];
		//if (game_list[hply].castle_k[!s])  input_weight[CASTLE_OTM_K][i]  -= learning_rate * delta_hidden[i];
		//if (game_list[hply].castle_q[!s])  input_weight[CASTLE_OTM_Q][i]  -= learning_rate * delta_hidden[i];

		// en passant file feature (mirrored by king colour as per your logic)
		if (ep > -1) {
			int ef = (col[kingloc[s]] > 3) ? ep_file[ep] : ep_file[7 - ep];
			input_weight[ef][i] -= learning_rate * delta_hidden[i];
		}
	}

	// ---- Optional logging back in centipawns ----
	float error_cp = SCALE_CP * error;
	if (total_error < MAX_ERRORS) errors[total_error++] = error_cp;
}

int NN_Play(int s) 
{
	int ep = GetEpFile();
	SetInput(s, ep);

	float hidden[HIDDEN_SIZE];
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
			if (col[kingloc[s]] > 3)
				sum += input_weight[ep_file[ep]][i];
			else
				sum += input_weight[ep_file[7 - ep]][i];
		}

		hidden[i] = (sum > 0.0f) ? sum : 0.0f;
	}

	float output = output_bias;
	for (int i = 0; i < HIDDEN_SIZE; i++) 
		output += hidden_weight[i] * hidden[i];

	if (output > 1.0f) output = 1.0f;
	if (output < -1.0f) output = -1.0f;

	return (int)(SCALE_CP * output);
}

int NN_Opponent(int s) 
{
	int ep = GetEpFile();
	SetInput(s, ep);

	float hidden[HIDDEN_SIZE];
	for (int i = 0; i < HIDDEN_SIZE; ++i) 
	{
		float sum = old_hidden_bias[i];
		for (int j = 0; j < list_count; ++j) 
		{
			sum += old_input_weight[input_list[j]][i];
		}

		if (game_list[hply].castle_k[s]) sum += old_input_weight[CASTLE_STM_K][i];
		if (game_list[hply].castle_q[s]) sum += old_input_weight[CASTLE_STM_Q][i];
		if (game_list[hply].castle_k[!s]) sum += old_input_weight[CASTLE_OTM_K][i];
		if (game_list[hply].castle_q[!s]) sum += old_input_weight[CASTLE_OTM_Q][i];

		if (ep > -1)
		{
			if (col[kingloc[s]] > 3)
				sum += old_input_weight[ep_file[ep]][i];
			else
				sum += old_input_weight[ep_file[7 - ep]][i];
		}

		hidden[i] = (sum > 0.0f) ? sum : 0.0f;
	}

	float output = old_output_bias;
	for (int i = 0; i < HIDDEN_SIZE; i++) 
		output += old_hidden_weight[i] * hidden[i];

	if (output > 1.0f) output = 1.0f;
	if (output < -1.0f) output = -1.0f;

	return (int)(SCALE_CP * output);
}

void InitWeights()
{
	for (int i = 0; i < INPUT_SIZE; ++i) 
	{
		for (int j = 0; j < HIDDEN_SIZE; ++j)
		{
			input_weight[i][j] = ((float)rand() / RAND_MAX) * 2 - 1;
			input_weight[i][j] *= sqrt(2.0f / INPUT_SIZE); // He Initialization
			//input_weight[i][j] = rand_weight() * 0.1f;
			old_input_weight[i][j] = input_weight[i][j];
		}
	}
	for (int j = 0; j < HIDDEN_SIZE; ++j)
	{
		hidden_bias[j] = 0.01f;
		old_hidden_bias[j] = hidden_bias[j];
	}

	for (int j = 0; j < HIDDEN_SIZE; ++j)
	{
		hidden_weight[j] = ((float)rand() / RAND_MAX) * 2 - 1;
		hidden_weight[j] *= sqrt(2.0f / HIDDEN_SIZE); // He Initialization
		//hidden_weight[j] = rand_weight() * 0.1f;
		old_hidden_weight[j] = hidden_weight[j];
	}
	output_bias = 0.0;
	old_output_bias = output_bias;

	SetIndex();
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

float GetAverage()
{
	float average = 0;
	float av = 0.f;
	for (int x = 0; x < total_error; x++)
	{
		average += errors[x];
	}
	if (total_error > 0)
		av = average / total_error;
	total_error = 0;
	return av;
}

int SaveWeights(int epochs)
{
	std::ofstream file("output.txt");
	if (!file) {
		std::cerr << "Could not open output.txt for writing.\n";
		return 1;
	}

	file.imbue(std::locale::classic());

	// Write header
	file << epochs << '\n';

	// Switch to hexfloat for exact representation
	file << std::hexfloat;

	// input -> hidden
	for (int x = 0; x < INPUT_SIZE; ++x)
		for (int y = 0; y < HIDDEN_SIZE; ++y)
			file << input_weight[x][y] << '\n';

	// hidden -> output
	for (int y = 0; y < HIDDEN_SIZE; ++y)
		file << hidden_weight[y] << '\n';

	// hidden bias
	for (int y = 0; y < HIDDEN_SIZE; ++y)
		file << hidden_bias[y] << '\n';

	// output bias
	file << output_bias << '\n';

	// Restore stream flags (optional)
	file << std::defaultfloat;

	return file ? 0 : 2;
}

int LoadWeights()
{
	std::ifstream file("output.txt");
	if (!file) {
		std::cerr << "File not found!\n";
		return 1;
	}

	file.imbue(std::locale::classic());

	// Read header
	int epochs;

	if (!(file >> epochs)) {
		std::cerr << "Failed to read epochs header.\n";
		return 2;
	}

	// input -> hidden
	for (int x = 0; x < INPUT_SIZE; ++x)
		for (int y = 0; y < HIDDEN_SIZE; ++y)
			if (!(file >> input_weight[x][y])) return 3;

	// hidden -> output
	for (int y = 0; y < HIDDEN_SIZE; ++y)
		if (!(file >> hidden_weight[y])) return 4;

	// hidden bias
	for (int y = 0; y < HIDDEN_SIZE; ++y)
		if (!(file >> hidden_bias[y])) return 5;

	// output bias
	if (!(file >> output_bias)) return 6;

	return 0;
}

void UpdateBest()
{
	for (int i = 0; i < INPUT_SIZE; ++i) 
	{
		for (int j = 0; j < HIDDEN_SIZE; ++j)
		{
			best_input_weight[i][j] = input_weight[i][j];
		}
	}
	for (int i = 0; i < HIDDEN_SIZE; ++i)
	{
		best_hidden_bias[i] = hidden_bias[i];
		best_hidden_weight[i] = hidden_weight[i];
	}
	best_output_bias = output_bias;
}

void UpdateWeights()
{
	for (int i = 0; i < INPUT_SIZE; ++i) 
	{
		for (int j = 0; j < HIDDEN_SIZE; ++j)
		{
			input_weight[i][j] = best_input_weight[i][j];
		}
	}
	for (int i = 0; i < HIDDEN_SIZE; ++i)
	{
		hidden_bias[i] = best_hidden_bias[i];
		hidden_weight[i] = best_hidden_weight[i];
	}
	output_bias = best_output_bias;
}

void UpdateOldWeights()
{
	for (int i = 0; i < INPUT_SIZE; ++i) 
	{
		for (int j = 0; j < HIDDEN_SIZE; ++j)
		{
			old_input_weight[i][j] = best_input_weight[i][j];
		}
	}
	for (int i = 0; i < HIDDEN_SIZE; ++i)
	{
		old_hidden_bias[i] = best_hidden_bias[i];
		old_hidden_weight[i] = best_hidden_weight[i];
	}
	old_output_bias = best_output_bias;
}

void CompWeights()
{
	for (int i = 0; i < INPUT_SIZE; ++i) 
	{
		for (int j = 0; j < HIDDEN_SIZE; ++j)
		{
			if(input_weight[i][j] != old_input_weight[i][j])// && input_weight[i][j] != NaN)
				printf(" I %f %f ",input_weight[i][j], old_input_weight[i][j]);
		}
	}
	for (int i = 0; i < HIDDEN_SIZE; ++i)
	{
		if(hidden_bias[i] != old_hidden_bias[i])
			printf("2");
		if(hidden_weight[i] != old_hidden_weight[i])
			printf(" H %f %f ",hidden_weight[i], old_hidden_weight[i]);
	}
	if(output_bias != old_output_bias)
		printf("4");
}

int GetEpFile()
{
	int ep = GetHistoryDest(hply - 1);

	if (board[ep] == 0 && abs(GetHistoryStart(hply - 1) - ep) == 16)
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

static int write_all(const void* p, size_t sz, size_t n, FILE* f) 
{
	return (std::fwrite(p, sz, n, f) == n) ? 0 : -1;
}
static int read_all(void* p, size_t sz, size_t n, FILE* f) 
{
	return (std::fread(p, sz, n, f) == n) ? 0 : -1;
}

int SaveWeightsBinary(int epochs, const char* path) 
{
	const char* fname = path ? path : "weights.bin";
	FILE* f = std::fopen(fname, "wb");
	if (!f) { std::perror("SaveWeightsBinSimple fopen"); return 1; }

	// ASCII header line
	std::fprintf(f, "epochs %d %d %d\n", epochs, INPUT_SIZE, HIDDEN_SIZE);

	// input -> hidden (row-major: input first)
	for (int x = 0; x < INPUT_SIZE; ++x) {
		if (write_all(&input_weight[x][0], sizeof(float), HIDDEN_SIZE, f)) {
			std::cerr << "Save: input_weight row write failed at row " << x << "\n";
			std::fclose(f); return 2;
		}
	}
	// hidden -> output
	if (write_all(&hidden_weight[0], sizeof(float), HIDDEN_SIZE, f)) 
	{ std::cerr << "Save: hidden_weight\n"; std::fclose(f); return 3; }
	// hidden bias
	if (write_all(&hidden_bias[0], sizeof(float), HIDDEN_SIZE, f))   
	{ std::cerr << "Save: hidden_bias\n";   std::fclose(f); return 4; }
	// output bias
	if (write_all(&output_bias, sizeof(float), 1, f))                
	{ std::cerr << "Save: output_bias\n";  std::fclose(f); return 5; }

	std::fclose(f);
	return 0;
}

int LoadWeightsBinary(const char* path) 
{
	const char* fname = path ? path : "weights.bin";
	FILE* f = std::fopen(fname, "rb");
	if (!f) { std::perror("LoadWeightsBinSimple fopen"); return 1; }

	// Read ASCII header line
	char header[128] = {0};
	if (!std::fgets(header, sizeof(header), f)) 
	{ std::cerr << "Load: header read failed\n"; std::fclose(f); return 2; }

	int epochs = 0, in_sz = 0, hid_sz = 0;
	int parsed = std::sscanf(header, "epochs %d %d %d", &epochs, &in_sz, &hid_sz);
	if (parsed < 1) { std::cerr << "Load: bad header format\n"; std::fclose(f); return 3; }

	// Optional size check (keeps you safe from mismatches)
	if (parsed >= 3) {
		if (in_sz != INPUT_SIZE || hid_sz != HIDDEN_SIZE) {
			std::cerr << "Load: size mismatch (file " << in_sz << "x" << hid_sz
				<< " vs build " << INPUT_SIZE << "x" << HIDDEN_SIZE << ")\n";
			std::fclose(f); return 4;
		}
	}

	// input -> hidden
	for (int x = 0; x < INPUT_SIZE; ++x) {
		if (read_all(&input_weight[x][0], sizeof(float), HIDDEN_SIZE, f)) {
			std::cerr << "Load: input_weight row read failed at row " << x << "\n";
			std::fclose(f); return 5;
		}
	}
	// hidden -> output
	if (read_all(&hidden_weight[0], sizeof(float), HIDDEN_SIZE, f)) 
	{ std::cerr << "Load: hidden_weight\n"; std::fclose(f); return 6; }
	// hidden bias
	if (read_all(&hidden_bias[0], sizeof(float), HIDDEN_SIZE, f))   
	{ std::cerr << "Load: hidden_bias\n";   std::fclose(f); return 7; }
	// output bias
	if (read_all(&output_bias, sizeof(float), 1, f))                
	{ std::cerr << "Load: output_bias\n";  std::fclose(f); return 8; }

	std::fclose(f);
	return epochs;
}
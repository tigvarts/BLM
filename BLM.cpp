#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <cmath>
#include <vector>
#include <locale>

#include "utils.h"
#include "thread_class.h"

using namespace std;

int number_of_nodes, number_of_edges;

float **In;
float **Out;
float *Z;

vector<int> edges_src, edges_dst;
vector<float> edges_weight;

int nu;
float gamma_;
float eta;
int vector_size;
int number_of_threads;
int number_of_epoch;

float *p_n;

map<string, string> config;

map<string, int> encode_node;
vector<string> decode_node;

void load_network(const char *filename)
{
    number_of_nodes = 0;
    number_of_edges = 0;
    
    string line;
    vector<string> tokens;
    ifstream fin(filename);
    while (getline(fin, line)) {
        tokens = get_tokens(line);
        if (line == "" || tokens.size() < 2) {
            continue;
        }
        if (encode_node.count(tokens[0]) == 0) {
            encode_node[tokens[0]] = number_of_nodes++;
            decode_node.push_back(tokens[0]);
        }
        if (encode_node.count(tokens[1]) == 0) {
            encode_node[tokens[1]] = number_of_nodes++;
            decode_node.push_back(tokens[1]);
        }
        float weight = 1;
        if (tokens.size() > 2) {
            weight = string_to_ldouble(tokens[2]);
        }
        edges_src.push_back(encode_node[tokens[0]]);
        edges_dst.push_back(encode_node[tokens[1]]);
        edges_weight.push_back(weight);
        number_of_edges++;
    }
    fin.close();
}

void initialize_variables()
{
    vector_size = string_to_int(config["vector_size"]);
    nu = string_to_int(config["nu"]);
    number_of_epoch = string_to_int(config["number_of_epoch"]);
    number_of_threads = string_to_int(config["number_of_threads"]);
    eta = string_to_ldouble(config["eta"]);
    gamma_ = string_to_ldouble(config["gamma"]);
}

void initialize_model()
{
    Z = (float *)calloc(number_of_nodes, sizeof(*Z));
    In = (float **)calloc(number_of_nodes, sizeof(*In));
    Out = (float **)calloc(number_of_nodes, sizeof(*Out));
    
    float Z_init = log(number_of_nodes);
    for (int i = 0; i < number_of_nodes; ++i) {
        In[i] = (float *)calloc(vector_size, sizeof(*In[i]));
        Out[i] = (float *)calloc(vector_size, sizeof(*Out[i]));
        Z[i] = Z_init;
        
        for (int j = 0; j < vector_size; ++j) {
            In[i][j] = (rand_double() - 0.5) / vector_size;
            Out[i][j] = (rand_double() - 0.5) / vector_size;
        }
    }
    
    p_n = (float *)calloc(number_of_nodes, sizeof(*p_n));
    for (int i = 0; i < number_of_edges; ++i) {
        p_n[edges_src[i]] += 1.0;
        p_n[edges_dst[i]] += 1.0;
    }
    for (int i = 0; i < number_of_nodes; ++i) {
        p_n[i] /= number_of_edges * 2.0;
    }
}

void learn_edge(int u, int v, float w, int k)
{
    float sp = 0;
    
    for (int t = 0; t < vector_size; ++t) {
        sp += In[u][t] * Out[v][t];
    }
    
    float pm0 = exp(sp - Z[u]);
    
    float nu_pn = nu * p_n[v];
    
    float coeff;
    
    if (k == 1) {
        coeff = nu_pn / (pm0 + nu_pn);
    } else {
        coeff = -pm0 / (pm0 + nu_pn);
    }
    
    for (int t = 0; t < vector_size; ++t) {
        In[u][t] += eta * w * (coeff * Out[v][t] - 2.0 * gamma_ * In[u][t]);
        Out[v][t] += eta * w * (coeff * In[u][t] - 2.0 * gamma_ * Out[v][t]);
    }
    Z[u] += -eta * w * coeff;
}

class LearnThread: public Thread
{
    int begin, end;
    unsigned long long rnd_raw;
    enum { rnd_a = 6364136223846793005, rnd_c = 1442695040888963407 };
public:
    LearnThread(int _begin, int _end): begin(_begin), end(_end) {
        rnd_raw = (unsigned long long)(&begin);
    }
    void run() {
        int rnd_border = ((1LL << 31) / (2 * number_of_edges)) * (2 * number_of_edges);
        
        for (int i = begin; i < end; ++i) {
            int u, v;
            u = edges_src[i], v = edges_dst[i];
            float w;
            w = edges_weight[i];
            
            for (int j = 0; j < nu; ++j) {
                int rnd;
                while ((rnd = ((rnd_raw = rnd_raw * rnd_a + rnd_c) >> 33)) >= rnd_border);
                rnd %= 2 * number_of_edges;

                int noise_v;
                if (rnd < number_of_edges) {
                    noise_v = edges_src[rnd];
                } else {
                    noise_v = edges_dst[rnd - number_of_edges];
                }
                
                learn_edge(u, noise_v, w, -1);
            }
            learn_edge(u, v, w, 1);
        }
    }
};

void learn_model()
{
    LearnThread **threads = (LearnThread **)calloc(number_of_threads, sizeof(*threads));
    
	for (int epoch = 0; epoch < number_of_epoch; ++epoch) {
        for (int i = 0; i < number_of_threads; ++i) {
            int begin = (number_of_edges / number_of_threads) * i;
            int end = (number_of_edges / number_of_threads) * (i + 1);
            if (i == number_of_threads - 1) {
                end = number_of_edges;
            }
            
            threads[i] = new LearnThread(begin, end);
            threads[i]->start();
        }
        
        for (int i = 0; i < number_of_threads; ++i) {
            threads[i]->wait();
            delete threads[i];
        }
	}
}

void write_model()
{
    if (config.find("output_textfile_model_filename") != config.end()) {
        string format_string = config["output_textfile_model_format_string"] + "%c";
        const char *format = format_string.c_str();
        FILE *f = fopen(config["output_textfile_model_filename"].c_str(), "wt");
        fprintf(f, "%d %d\n", number_of_nodes, vector_size);
        for (int i = 0; i < number_of_nodes; ++i) {
            fprintf(f, "%s\n", decode_node[i].c_str());
            for (int j = 0; j < vector_size; ++j) {
                fprintf(f, format, In[i][j], " \n"[j == vector_size - 1]);
            }
            for (int j = 0; j < vector_size; ++j) {
                fprintf(f, format, Out[i][j], " \n"[j == vector_size - 1]);
            }
            fprintf(f, format, Z[i], '\n');
        }
        fclose(f);
    }
}

void free_model()
{
    free(Z);
    free(p_n);
    for (int i = 0; i < number_of_nodes; ++i) {
        free(In[i]);
        free(Out[i]);
    }
    free(In);
    free(Out);
}

#include <ctime>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: ./BLM config.cfg");
        exit(EXIT_FAILURE);
    }
    
    config = get_config(argv[1]);
    
    load_network(config["network_filename"].c_str());
    
    initialize_variables();
    
    initialize_model();
    
    learn_model();
    
    write_model();
    
    free_model();
    
    cerr << double(clock()) / CLOCKS_PER_SEC << endl;
}

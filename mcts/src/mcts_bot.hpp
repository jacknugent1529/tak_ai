#include "game.hpp"
#include "ai_model.hpp"
#include <fstream>

typedef struct mcts_node_t {
    tak_game_t game;
    float val;
    std::vector<move_t> moves;
    std::vector<float> P;
    int N;
    bool is_initialized;
    bool game_ended;
    std::vector<mcts_node_t> children;
    mcts_node_t *parent;
    model_t model;
    bool use_ai;
} mcts_node_t;

mcts_node_t new_mcts(tak_game_t game, model_t &model, bool use_ai);

void simulate(tak_game_t game, int repetitions, std::ofstream &file, model_t &model, bool use_ai);

int oppose_bots(tak_game_t game, int repetitions, model_t &model1, model_t &model2, bool use_mcts);


mcts_node_t* mcts_apply_move(mcts_node_t *node, move_t move);

move_t get_move(mcts_node_t *node, int repetitions);

std::string move_to_string(move_t move);

move_t string_to_move(std::string s);

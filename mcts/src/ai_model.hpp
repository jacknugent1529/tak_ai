#include "game.hpp"
#include <torch/script.h>

typedef torch::jit::script::Module model_t;

float get_eval(model_t &model, tak_game_t *game, std::vector<move_t> &moves, std::vector<float> &ps);


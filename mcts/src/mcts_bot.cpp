#include "mcts_bot.hpp"
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <boost/json.hpp>
#include <stdexcept>

namespace json = boost::json;
void write_results(mcts_node_t *final_state, std::ofstream &file);

bool isclose(float a, float b) {
    return abs(a - b) < 1e-6;
}

/* initialize a node; create its children but leave them uninitialized */
void init_node(mcts_node_t *node) {
    std::vector<move_t> valid_moves = available_moves(&node->game);
    
    node->moves = valid_moves;
    if (node->use_ai) {
        std::vector<float> P;
        float val = get_eval(node->model, &node->game, valid_moves, P);
        node->val = val;
        node->P = P;
    } else {
        float p = 1 / ((float) valid_moves.size());
        std::vector<float> P(valid_moves.size(), p);
        node->val = tiles_eval(&node->game);   
        node->P = P; 
    }

    // create children with proper game, N, game_ended, is_initialized fields
    for (auto m: valid_moves) {
        mcts_node_t child = {0};
        child.model = node->model;
        child.parent = node;
        child.use_ai = node->use_ai;
        apply_move(&child.game, &node->game, &m);


        child.N = 0;
        child.val = 0;
        switch (game_outcome(&child.game)) {
            case IN_PROGRESS:
                child.game_ended = false;
                child.is_initialized = false;
                break;
            case P1_WIN:
                child.game_ended = true;
                child.val = (child.game.turn == 1) ? 1 : -1;
                child.is_initialized = true;
                break;
            case P2_WIN:
                child.game_ended = true;
                child.val = (child.game.turn == 2) ? 1 : -1;
                child.is_initialized = true;
                break;
            case TIE:
                child.game_ended = true;
                child.val = 0;
                child.is_initialized = true;
                break;
        }
        node->children.push_back(child);
    }
    node->is_initialized = true;
}

/* perform a step of MCTS search */
float search(mcts_node_t *node, float lambda) {
    if (node->game_ended) {
        return -node->val;
    }

    if (!node->is_initialized) {
        /* set val, moves, P, children for node */
        init_node(node);
        return -node->val;
    }

    float max_ucb = -INFINITY; // max upper confidence bound
    mcts_node_t *best_child = NULL;
    assert(node->children.size() > 0);


    for (int i = 0; i < node->children.size(); i++) {
        mcts_node_t *child = &node->children[i];
        float conf_factor = sqrtf(1 / (1 + (float) child->N));
        float ucb = -child->val + lambda * node->P[i] * conf_factor;
        if (ucb > max_ucb) {
            max_ucb = ucb;
            best_child = child;
        }
    }

    assert(best_child != NULL);
    float val = search(best_child, lambda);
    
    best_child->val = (((float) best_child->N) * best_child->val - val)/((float) best_child->N + 1);
    best_child->N += 1;

    return -val;
}

/* get probability distribution for moves based on MCTS */
std::vector<float> get_prob(mcts_node_t const *node, float temp) {
    assert(isclose(temp, 1)); // TODO: implement different values for temperature
    int N_tot = 0;
    for (auto c: node->children) {
        N_tot += c.N;
    }
    assert(N_tot != 0);

    std::vector<float> P;
    for (auto c: node->children) {
        float p = ((float) c.N) / ((float) N_tot);
        P.push_back(p);
    }
    return P;
}

/* get move based on MCTS */
move_t get_move(mcts_node_t *node, int repetitions) {
    for (int i = 0; i < repetitions; i++) {
        search(node, 1);
    }
    assert(node->is_initialized);
    assert(!node->game_ended);
    assert(node->moves.size() > 0);

    std::vector<float> P = get_prob(node, 1);

    float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

    float tot = 0;
    move_t move;
    for (int i = 0; i < node->children.size(); i++) {
        tot += P[i];
        if (tot > r) {
            move = node->moves[i];
            break;
        }
    }
    return move;
}

/* return the node in the tree search after applying a move */
mcts_node_t* mcts_apply_move(mcts_node_t *node, move_t move) {
    if (!node->is_initialized) {
        init_node(node);
    }
    for (int i = 0; i < node->children.size(); i++) {
        move_t m = node->moves[i];
        if (move_eq(move, m)) {
            return &node->children[i];
        }
    }
    assert(false);
}

/* simulate two bots playing a game */
int oppose_bots_h(tak_game_t game, int repetitions, mcts_node_t root1, mcts_node_t root2) {
    mcts_node_t *mcts1 = &root1;
    mcts_node_t *mcts2 = &root2;

    int c = 0;
    while (!mcts1->game_ended) {
        move_t move1 = get_move(mcts1, repetitions);


        mcts1 = mcts_apply_move(mcts1, move1);
        mcts2 = mcts_apply_move(mcts2, move1);

        if (mcts1->game_ended) {
            break;
        }

        move_t move2 = get_move(mcts2, repetitions);

        mcts1 = mcts_apply_move(mcts1, move2);
        mcts2 = mcts_apply_move(mcts2, move2);

        c++;
    }
    std::cout << "GAME FINISHED after " << c << " turns \n";
    switch (game_outcome(&mcts1->game)) {
        case P1_WIN:
            std::cout << "Player 1 wins!\n";
            return 1;
        case P2_WIN:
            std::cout << "Player 2 wins!\n";
            return 2;
        case TIE:
            std::cout << "Tie!\n";
            return 0;
        default:
            assert(false);
    }
}

/* simulate two botts playing a game */
int oppose_bots(tak_game_t game, int repetitions, model_t &model1, model_t &model2, bool bot2_default) {
    mcts_node_t root1 = new_mcts(game, model1, true);
    mcts_node_t root2 = new_mcts(game, model2, !bot2_default);
    return oppose_bots_h(game, repetitions, root1, root2);
}

/* simulate a bot playing itself */
void simulate(tak_game_t game, int repetitions, std::ofstream &file, model_t &model, bool use_ai) {
    mcts_node_t root1 = new_mcts(game, model, use_ai);

    mcts_node_t *mcts1 = &root1;

    int c = 0;
    while (!mcts1->game_ended) {
        move_t move1 = get_move(mcts1, repetitions);
        mcts1 = mcts_apply_move(mcts1, move1);

        if (mcts1->game_ended) {
            break;
        }
        move_t move2 = get_move(mcts1, repetitions);

        mcts1 = mcts_apply_move(mcts1, move2);
        c++;
    }
    write_results(mcts1, file);
}

/* create a new mcts node */
mcts_node_t new_mcts(tak_game_t game, model_t &model, bool use_ai) {
    /* assumes game is not over */
    mcts_node_t node = {0};
    node.game = game;
    node.is_initialized = false;
    node.game_ended = false;
    node.N = 0;
    node.model = model;
    node.use_ai = use_ai;
    return node;
}

/* SERIALIZATION FUNCTIONS */

void tag_invoke( json::value_from_tag, json::value &jv, tak_game_t const &game) {
    jv = {
        {"turn", game.turn},
        {"p1_pieces_rm", game.p1_pieces_rem},
        {"p2_pieces_rem", game.p2_pieces_rem},
        {"board", json::value_from(game.board)},
    };
}

void tag_invoke( json::value_from_tag, json::value &jv, move_t const &move) {
    switch(move.move) {
        case MOVE:
            jv = {
                {"move", "MOVE"},
                {"i", move.i},
                {"j", move.j},
                {"di", move.di},
                {"dj", move.dj},
                {"drop0", move.drop0},
                {"drop1", move.drop1},
                {"drop2", move.drop2},
            };
            break;
        case WALL:
            jv = {
                {"move", "WALL"},
                {"i", move.i},
                {"j", move.j},
            };
            break;
        case FLAT:
            jv = {
                {"move", "FLAT"},
                {"i", move.i},
                {"j", move.j},
            };
            break;
    }
}

void tag_invoke( json::value_from_tag, json::value &jv, mcts_node_t const &node) {
    std::vector<float> p = get_prob(&node, 1);
    jv = {
        {"game", json::value_from(node.game)},
        {"moves", json::value_from(node.moves)},
        {"p", json::value_from(p)},
        {"val", node.val},
    };
}

std::string move_to_string(move_t move) {
    std::ostringstream stream;
    stream << json::value_from(move);
    return stream.str();
}

move_t string_to_move(std::string s) {
    json::value v = json::parse(s);
    move_t m;
    auto o = v.as_object();
    m.i = (uint8_t) o["i"].as_int64();
    m.j = (uint8_t) o["j"].as_int64();
    auto move_s = o["move"].as_string();
    if (move_s == "FLAT") {
        m.move = FLAT;
    } else if (move_s == "WALL") {
        m.move = WALL;
    } else if (move_s == "MOVE") {
            m.move = MOVE;
            m.di = (uint8_t) o["di"].as_int64();
            m.dj = (uint8_t) o["dj"].as_int64();
            m.drop0 = (uint8_t) o["drop0"].as_int64();
            m.drop1 = (uint8_t) o["drop1"].as_int64();
            m.drop2 = (uint8_t) o["drop2"].as_int64();
    } else {
        throw std::invalid_argument("move not valid");
    }
    return m;
}

/* after a game has finished, record the results as json for the training data
*/

void write_results_r(mcts_node_t *node, float final_val, std::ofstream &file) {
    if (node == NULL) {
        return;
    }
    node->val = final_val;
    file << json::value_from(*node);
    file << ",";

    write_results_r(node->parent, -final_val, file);
}

void write_results(mcts_node_t *final_state, std::ofstream &file) {
    write_results_r(final_state->parent, -final_state->val, file);
}
#include <iostream>

#include <boost/program_options.hpp>
#include <string>

#include "game.hpp"
#include "mcts_bot.hpp"


namespace po = boost::program_options;

bool parse_move(move_t *move, tak_game_t *game) {
    std::string help_str = 
    "move specified by (move type)(location)[direction][drops]\n"
    "for example: fa1 or mb2d1\n"
    "   - move type: f/w/m representing (flat/wall/move tower)\n"
    "   - location: (a-d)(1-4)\n"
    "   - direction: w/a/s/d (move tower in direction)\n"
    "   - drops: [0-8][1-8][1-8] (pieces left behind at each spot, not necessary to specify all)\n";
    char m;
    std::cin >> m;
    if (std::cin.eof()) {
        exit(0);
    }
    switch (m) {
        case 'f':
            move->move = FLAT;
            break;
        case 'w':
            move->move = WALL;
            break;
        case 'm':
            move->move = MOVE;
            break;
        case 'h':
            std::cout << help_str;
            return false;
        default:
            return false;
    }
    char i_c, j_c;
    std::cin >> i_c >> j_c;
    int i = i_c - 'a';
    int j = j_c - '1';
    if (!((0 <= i && i < 4) && (0 <= j && j < 4))) {
        return false;
    }
    move->i = i;
    move->j = j;
    move->di = 0; move->dj = 0; 
    move->drop0 = 0; move->drop1 = 0; move->drop2 = 0;
    if (move->move == MOVE) {
        // move tower
        std::string move_str;
        std::cin >> move_str;
        if (move_str.size() == 0) {
            return false;
        }
        char dir = move_str.at(0);
        
        switch (dir) {
            case 'w':
                move->dj = -1;
                move->di = 0;
                break;
            case 's':
                move->dj = 1;
                move->di = 0;
                break;
            case 'a':
                move->dj = 0;
                move->di = -1;
                break;
            case 'd':
                move->dj = 0;
                move->di = 1;
                break;
            default:
                return false;
        }

        // how many tiles to move to each location
        int h = get_tower_height(game, (uint8_t) i, (uint8_t) j);
        std::vector<int> drops(3);
        int tot = 0;
        for (int i = 0; i < 3; i++) {
            int d;
            if (i+1 >= move_str.size()) {
                d = 0;
            } else {
                d = move_str[i+1] - '0';
            }
            if (i > 0) {
                d = std::max(d, 1);
            }
            drops[i] = std::min(d, h - tot);
            tot += drops[i];
        }
        move->drop0 = drops[0];
        move->drop1 = drops[1];
        move->drop2 = drops[2];
    }
    
    for (auto m: available_moves(game)) {
        if (move_eq(m, *move)) {
            return true;
        }
    }
    return false;
}


void game_tui_2p(tak_game_t game) {
    tak_game_t game_old;
    move_t move;

    std::string s = game_to_string(&game);
    std::cout << s;

    int turn = 0;
    while (game_outcome(&game) == IN_PROGRESS) {
        std::cout << "Enter a move for player " << turn % 2 + 1 << ": ";
        while (!parse_move(&move, &game)) {
            std::cout << "Invalid move, try again (enter h for help): ";
        }

        game_old = game;
        apply_move(&game, &game_old, &move);

        std::string s = game_to_string(&game);
        std::cout << s;
        
        turn++;
    }
    std::cout << "GAME FINISHED\n";
}

void game_tui_bot(tak_game_t game, mcts_node_t *mcts, int repetitions) {
    tak_game_t game_old;
    move_t move;

    std::string s = game_to_string(&game);
    std::cout << s;

    int turn = 0;
    while (game_outcome(&game) == IN_PROGRESS) {
        std::cout << "Enter a move for player " << turn % 2 + 1 << ": ";
        while (!parse_move(&move, &game)) {
            std::cout << "Invalid move, try again (enter h for help): ";
        }

        game_old = game;
        apply_move(&game, &game_old, &move);
        mcts = mcts_apply_move(mcts, move);

        std::cout << game_to_string(&game);

        std::cout << "Bot move: \n";

        move_t bot_move = get_move(mcts, repetitions);
        game_old = game;
        apply_move(&game, &game_old, &bot_move);
        mcts = mcts_apply_move(mcts, bot_move);

        std::cout << game_to_string(&game);
        
        turn++;
    }
    std::cout << "GAME FINISHED\n";
    switch (game_outcome(&game)) {
        case P1_WIN:
            std::cout << "Player 1 wins!" << std::endl;
            break;
        case P2_WIN:
            std::cout << "Player 2 wins!" << std::endl;
            break;
        case TIE:
            std::cout << "It's a tie!" << std::endl;
            break;
        case IN_PROGRESS:
            __builtin_unreachable();
    }
}


int main(int ac, char* av[]) {
    po::options_description desc("Allowed options");
    int iter;
    desc.add_options()
        ("help", "produce help message")
        ("bot", "play against a bot")
        ("model", po::value<std::string>(), "torchScript model file")
        ("iter", po::value<int>(&iter)->default_value(10), "number of mcts iterations")
    ;
    
    po::variables_map vm;        
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    tak_game_t game = new_tak_game();
    
    if (vm.count("bot") > 0) {
        model_t model;
        if (vm.count("model")) {
            try {
                // Deserialize the ScriptModule from a file using torch::jit::load().
                auto file = vm["model"].as<std::string>();
                model = torch::jit::load(file);
            }
            catch (const c10::Error& e) {
                std::cerr << "error loading the model\n";
                return -1;
            }
        } else {
            std::cerr << "please specify a model file with --model";
            return -1;
        }
        mcts_node_t node = new_mcts(game, model, true);
        game_tui_bot(game, &node, iter);
    } else {
        game_tui_2p(game);
    }

}
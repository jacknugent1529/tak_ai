#include <iostream>

#include <boost/program_options.hpp>
#include <string>
#include <thread>
#include <torch/script.h>

#include "game.hpp"
#include "mcts_bot.hpp"


namespace po = boost::program_options;

/* simulate games */
void task(std::string filename, int n_games, int iter, model_t model, bool mcts, tak_game_t game) {
    std::ofstream file;
    file.open(filename);

    file << "[";
    for (int i = 0; i < n_games; i++) {
        if (i % 5 == 0) {
            std::cout << "iter " << i << " / " << n_games << std::endl;

        }
        simulate(game, iter, file, model, !mcts);
    }

    // delete the last comma so json is valid
    long pos = file.tellp();
    file.seekp(pos - 1);
    file << "]";       
}

int main(int ac, char* av[]) {

    srand(time(0));
    std::cout << rand() % 100 << "\n";
    po::options_description desc("Allowed options");
    int iter;
    int nthreads;
    desc.add_options()
        ("help", "produce help message")
        ("ngames,n", po::value<int>(), "number of games")
        ("model1", po::value<std::string>(), "torchScript model file")
        ("model2", po::value<std::string>(), "torchScript model file")
        ("oppose", "oppose ai bot with non-ai")
        ("mcts", "use mcts for single-bot simulation")
        ("iter", po::value<int>(&iter)->default_value(10), "number of mcts iterations")
        ("nthread", po::value<int>(&nthreads)->default_value(1), "number of mcts iterations")
    ;
    

    po::variables_map vm;        
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    bool mcts = vm.count("mcts") > 0;
    bool oppose = vm.count("oppose") > 0;

    model_t model1;
    if (!mcts || oppose) {
        if (vm.count("model1")) {
            try {
                // Deserialize the ScriptModule from a file using torch::jit::load().
                auto file = vm["model1"].as<std::string>();
                model1 = torch::jit::load(file);
            }
            catch (const c10::Error& e) {
                std::cerr << "error loading the model\n";
                return -1;
            }
            std::cout << "LOADED MODEL\n";
        } else {
            std::cout << "No model file specified (1)\n";
            return -1;
        }
    }

    model_t model2;
    if (!mcts || oppose) {
        if (vm.count("model2")) {
            try {
                // Deserialize the ScriptModule from a file using torch::jit::load().
                auto file = vm["model2"].as<std::string>();
                model2 = torch::jit::load(file);
            }
            catch (const c10::Error& e) {
                std::cerr << "error loading the model\n";
                return -1;
            }
            std::cout << "LOADED MODEL\n";
        } else {
            std::cout << "No model file specified (2)\n";
            return -1;
        }
    }
    
    
    int n_games = 1;
    if (vm.count("ngames")) {
        n_games = vm["ngames"].as<int>();
    }

    tak_game_t game = new_tak_game();

    if (oppose) {
        int dnn_wins = 0;
        int mcts_wins = 0;
        for (int i = 0; i < n_games; i++) {
            if (i % 50 == 0) {
                std::cout << "iter " << i << " / " << n_games << std::endl;
            }
            int res = oppose_bots(game, iter, model1, model2, false);
            if (res == 1) {
                dnn_wins++;
            } else if (res == 2) {
                mcts_wins++;
            }
        }
        std::cout << "FINAL RESULT: dnn - " << dnn_wins << " , mcts - " << mcts_wins << std::endl;
    } else {
        if (nthreads == 1) {
            task("out.json", n_games, iter, model1, mcts, game);
        } else {
            std::vector<std::thread*> all_threads;
            int games_per_thread = n_games / nthreads;
            for (int t = 0; t < nthreads; t++) {
                std::ostringstream stream;
                stream << "out" << t << ".json";
                std::string filename = stream.str();
                std::cout << filename <<"\n";
                std::thread *new_thread = new std::thread(task, filename, games_per_thread, iter, model1, mcts, game);
                all_threads.push_back(new_thread);
            }

            for (int i = 0; i < all_threads.size(); i++) {
                all_threads[i]->join();
                delete all_threads[i];
            }

        }
    }  
}
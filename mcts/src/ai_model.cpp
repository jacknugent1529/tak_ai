#include "ai_model.hpp"
#include <math.h>
#define WALL_OFFSET 10

void encode_board(float encoded_board[][4][9], uint8_t board[][4][9]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 9; k++) {
                switch (board[i][j][k]) {
                    case 1:
                        encoded_board[i][j][k] = -1.;
                        break;
                    case 2:
                        encoded_board[i][j][k] = 1.;
                        break;
                    case WALL_OFFSET + 1:
                        encoded_board[i][j][k] = -2.;
                        break;
                    case WALL_OFFSET + 2:
                        encoded_board[i][j][k] = 2.;
                        break;
                    default:
                        encoded_board[i][j][k] = 0.;
                        break;
                }
            }
        }
    }
}

void softmax(std::vector<float> &arr) {
    float tot = 0;
    for (int i = 0; i < arr.size(); i++) {
        float x = exp(arr[i]);
        arr[i] = x;
        tot += x;
    }

    for (int i = 0; i < arr.size(); i++) {
        arr[i] /= tot;
    }
}

float get_eval(model_t &module, tak_game_t *game, std::vector<move_t> &moves, std::vector<float> &ps) {
    float board[4][4][9];
    encode_board(board, game->board);
    
    auto options = torch::TensorOptions().dtype(torch::kF32);
    torch::Tensor B = torch::from_blob(board, {1,4,4,9}, options);
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(B);

    auto output = module.forward(inputs);
    auto output1 = output.toTuple()->elements()[0].toTensor();
    auto output2 = output.toTuple()->elements()[1].toTensor();

    float val = output1[0].item<float>();

    for (auto m: moves) {
        float p;
        switch (m.move) {
            case FLAT:
                p = output2[0][m.i][m.j][0][0][0][0].item<float>();
                break;
            case WALL:
                p = output2[0][m.i][m.j][1][0][0][0].item<float>();
                break;
            case MOVE:
                int idx = 2;
                if (m.di == -1) {idx = 3;}
                if (m.dj == 1) {idx = 4;}
                if (m.dj == -1) {idx = 5;}
                p = output2[0][m.i][m.j][idx][m.drop0][m.drop1][m.drop2].item<float>();
        }
        ps.push_back(p);
    }
    softmax(ps);
    
    return val;
}
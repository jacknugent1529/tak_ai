#include "game.hpp"
#include <string>
#include <sstream>
#include <iostream>
#include <array>
#include <algorithm>
#include <cstring>
#include <cassert>

#define WALL_OFFSET 10

/* helper functions to get tower heights */

int get_tower_height(tak_game_t *game, uint8_t i, uint8_t j) {
    int h = 0;
    while (game->board[i][j][h] != 0 && h < 8) {
        h++;
    }
    return h;
}

int tallest_tower(tak_game_t *game) {
    int max = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            while (max < 8 && game->board[i][j][max] != 0) {
                max++;
            }
        }
    }
    return max;
}

/* methods to mutate board state */
void add_piece(tak_game_t *game, uint8_t i, uint8_t j, int piece) {
    memmove(&game->board[i][j][1], &game->board[i][j][0], 7);
    game->board[i][j][0] = piece;
}

void move_tower(tak_game_t *game, uint8_t i, uint8_t j, uint8_t di, uint8_t dj, uint8_t drop0, uint8_t drop1, uint8_t drop2) {
    uint8_t drop3 = get_tower_height(game, i, j) - drop0 - drop1 - drop2;
    uint8_t drops[3] = {drop1, drop2, drop3};

    for (int c = 1; c <= 3; c++) {
        uint8_t i_dst = i + c * di;
        uint8_t j_dst = j + c * dj;
        uint8_t drop = drops[c - 1];
        
        if (i_dst < 0 || i_dst >= 4 || j_dst < 0 || j_dst >= 4 || drop == 0) {
            continue;
        }

        // move the current tower out of the way
        memmove(&game->board[i_dst][j_dst][drop], &game->board[i_dst][j_dst][0], 8 - drop);

        // move tower into position
        memmove(&game->board[i_dst][j_dst][0], &game->board[i][j][0], drop);

        // shift original tower into position
        memmove(&game->board[i][j][0], &game->board[i][j][drop], 8 - drop);
    }
}

void apply_move(tak_game_t *new_game, tak_game_t *old_game, move_t *move) {
    memcpy(new_game, old_game, sizeof(tak_game_t));

    switch (move->move) {
        case FLAT: 
            add_piece(new_game, move->i, move->j, new_game->turn);
            break;
        case WALL:
            add_piece(new_game, move->i, move->j, new_game->turn + WALL_OFFSET);
            break;
        case MOVE:
            move_tower(new_game, move->i, move->j, move->di, move->dj, move->drop0, move->drop1, move->drop2);
    }
    if (move->move != MOVE) {
        if (new_game->turn == 1) {
            new_game->p1_pieces_rem--;
        } else {
            new_game->p2_pieces_rem--;
        }
    }
    new_game->turn = (new_game->turn % 2) + 1;
}

/* methods to check for available moves */

/* helper function to handle movement of towers */
void search_line(
    tak_game_t *game, uint8_t i, uint8_t j, 
    int tower_height, std::vector<move_t> &moves
) {
    int dis[4] = {1,-1,0,0};
    int djs[4] = {0,0,1,-1};
    for (int k = 0; k < 4; k++) {
        int di = dis[k];
        int dj = djs[k];
    
        int max_drops[4] = {0};
        max_drops[0] = tower_height - 1;

        if (!(di == 0 ^ dj == 0)) {
            continue;
        }
        bool hit_wall = false;
        for (int k = 0; k < 3; k++) {
            if (i + (k+1)*di < 0 || i + (k+1)*di >= 4
                ||j + (k+1)*dj < 0 || j + (k+1)*dj >= 4) {
                break;
            }
            int curr = game->board[i + (k+1)*di][j + (k+1)*dj][0];
            if (curr >= WALL_OFFSET) {
                hit_wall = true;
            }
            max_drops[k+1] = hit_wall ? 0 : std::max(0,tower_height - k);
        }
        for (uint8_t d0 = 0; d0 <= max_drops[0]; d0++) {
            for (uint8_t d1 = 1; d1 <= max_drops[1]; d1++) {
                for (uint8_t d2 = 0; d2 <= max_drops[2]; d2++) {
                    if (d0 + d1 + d2 > tower_height || d0 + d1 + d2 + max_drops[3] < tower_height) {
                        continue;
                    }
                    int d3 = tower_height - d0 - d1 - d2;
                    if ((d0 + get_tower_height(game, i, j) < 8) 
                     && (d1 == 0 || d1 + get_tower_height(game, i + di, j + dj) < 8)
                     && (d2 == 0 || d2 + get_tower_height(game, i + 2*di, j + 2*dj) < 8)
                     && (d3 == 0 || d3 + get_tower_height(game, i + 3*di, j + 3*dj) < 8))
                    {
                        assert(game->board[i+di][j+dj][0] <= WALL_OFFSET);
                        move_t m{MOVE, i, j, (int8_t) di, (int8_t) dj, d0, d1, d2};
                        moves.push_back(m);
                    }
                }
            }
        }
    }
}

std::vector<move_t> available_moves(tak_game_t *game) {
    std::vector<move_t> moves;
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            if (game->board[i][j][0] == 0) {
                // empty; 
                // can place flat piece
                move_t flat{FLAT,i,j,0,0,0,0};
                moves.push_back(flat);
                // can place wall here
                move_t wall{WALL,i,j,0,0,0,0};
                moves.push_back(wall);   
            }
            if (game->board[i][j][0] == game->turn 
                || game->board[i][j][0] == game->turn + WALL_OFFSET) {
                int height = get_tower_height(game, i, j);
                search_line(game, i, j, height, moves);
            }
        }
    }
    return moves;
}

/* simple evaluation function based on the number of squares controlled by each player */
float tiles_eval(tak_game_t *game) {
    float p1_count = 0;
    float p2_count = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (game->board[i][j][0] == 1 || game->board[i][j][0] == 1 + WALL_OFFSET) {
                p1_count += 1;
            } else if (game->board[i][j][0] == 2 || game->board[i][j][0] == 2 + WALL_OFFSET) {
                p2_count += 1;
            }
        }
    }
    if (p1_count + p2_count == 0) {
        return 0;
    }

    if (game->turn == 1) {
        return (p1_count - p2_count) / (p1_count + p2_count);
    } else {
        return (p2_count - p1_count) / (p1_count + p2_count);
    }
}

/* check if two moves are equal */
bool move_eq(move_t move1, move_t move2) {
    bool first_part_eq = (move1.move == move2.move) 
        && (move1.i == move2.i)
        && (move1.j == move2.j);
    if (move1.move == MOVE) {
        bool second_part_eq = (move1.di == move2.di)
        && (move1.dj == move2.dj)
        && (move1.drop0 == move2.drop0)
        && (move1.drop1 == move2.drop1)
        && (move1.drop2 == move2.drop2);
        return first_part_eq && second_part_eq;
    }
    return first_part_eq;
}

/* find method for union-find datastructure */
int find(int *uf, int a) {
    if (uf[a] == -1) {
        return a;
    } else {
        return find(uf, uf[a]);
    }
}

/* 
union method for union-find datastructure; also updates the direction array 
*/
void union_dir(int *uf, bool dirs[][16], int a, int b) {
    a = find(uf, a);
    b = find(uf, b);
    if (a == b) {
        return;
    }
    uf[b] = a;
    dirs[0][a] = dirs[0][a] || dirs[0][b];
    dirs[1][a] = dirs[1][a] || dirs[1][b];
    dirs[2][a] = dirs[2][a] || dirs[2][b];
    dirs[3][a] = dirs[3][a] || dirs[3][b];
}

game_outcome_t game_outcome(tak_game_t *game) {
    /* first check if a player has no pieces left to play */
    if (game->p1_pieces_rem == 0 || game->p2_pieces_rem == 0) {
        int p1_count = 0;
        int p2_count = 0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                switch (game->board[i][j][0]) {
                    case 1:
                        p1_count += 1;
                        break;
                    case 2:
                        p2_count += 1;
                        break;
                }
            }
        }

        if (p1_count > p2_count) {
            return P1_WIN;
        } else if (p2_count > p1_count) {
            return P2_WIN;
        } else {
            return TIE;
        }
    }

    /* check if a player has a "road" across the board.
    This uses a union-find data-structure to track which edges of the board
    are reachable from a connected component. */
    int uf[16];
    std::fill_n(uf, 16, -1);
    bool dirs[4][16] = {0};
    
    for (int i = 0; i < 4; i++) {
        dirs[0][i*4+0] = true; // left
        dirs[1][i*4+3] = true; // right
        dirs[2][0*4+i] = true; // top
        dirs[3][3*4+i] = true; // bottom
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int dis[4] = {1,-1,0,0};
            int djs[4] = {0,0,1,-1};
            for (int k = 0; k < 4; k++) {
                int di = dis[k];
                int dj = djs[k];

                int i_p = i + di;
                int j_p = j + dj;
                if (i_p < 0 || i_p >= 4 || j_p < 0 || j_p >= 4) {
                    continue;
                }

                if (game->board[i][j][0] == game->board[i_p][j_p][0] && (game->board[i][j][0] == 1 || game->board[i][j][0] == 2)) {
                    int a = i * 4 + j;
                    int b = i_p * 4 + j_p;
                    union_dir(uf, dirs, a, b);
                    
                    int a_new = find(uf, a);
                    if ((dirs[0][a_new] && dirs[1][a_new]) || (dirs[2][a_new] && dirs[3][a_new])) {
                        if (game->board[i][j][0] == 1) {
                            return P1_WIN;
                        } else {
                            return P2_WIN;
                        }
                    }
                }
            }
        }
    }

    /* if the board is full, check for winner by piece counts;
    otherwise, the game is in progress */
    int p1_count = 0;
    int p2_count = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            switch (game->board[i][j][0]) {
                case 1:
                    p1_count += 1;
                    break;
                case 2:
                    p2_count += 1;
                    break;
                case 0:
                    return IN_PROGRESS;
            }
        }
    }

    return (p1_count > p2_count) ? P1_WIN : P2_WIN;
}

/* create an empty game */
tak_game_t new_tak_game() {
    tak_game_t g = {0};
    g.turn = 1;
    g.p1_pieces_rem = 15;
    g.p2_pieces_rem = 15;
    return g;
}

/* convert the game to a string for the TUI */
std::string game_to_string(tak_game_t *game) {
    int cell_height = std::max(tallest_tower(game), 1);
    int cell_width = 1;
    /* include extra row and column for coordinate labels*/
    int width = 1 + (1 + cell_width) * 4 + 1 + 1; // last for newline
    int height = 1 + (1 + cell_height) * 4 + 1;
    std::vector<char> s(width * height,  ' ');

    for (int i = 1; i < height; i += (cell_height + 1)) {
        for (int j = 1; j < width; j += 1) {
            s.at(i * width + j) = '-';
        }
    }

    // coordinate labels
    for (int i = 0; i < 4; i++) {
        int s_i = 2 + (1 + cell_height) * i;
        int j_i = 0;
        s.at(s_i * width + j_i) = '1' + i;
    }

    for (int j = 0; j < 4; j++) {
        int s_i = 0;
        int j_i = 2 + 2 * j;
        s.at(s_i * width + j_i) = 'a' + j;
    }

    for (int i = 1; i < height; i++) {
        for (int j = 1; j < width; j += (cell_width + 1)) {
            s.at(i * width + j) = '|';
        }
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // print individual cell
            int s_i = 1+(1+cell_height) * j + cell_height;
            int s_j = 1+(1+cell_width) * i + 1;
            int t_height = get_tower_height(game, i, j);
            for (int di = 0; di < t_height; di++) {
                uint8_t c = game->board[i][j][di];
                if (c != 0){
                    int idx = (s_i - t_height + di + 1) * width + s_j;
                    assert(s_j < width);
                    assert(s_i - t_height + di + 1 < height);
                    s.at(idx) = '0' + c;
                }
            }
        }
    }

    for (int i = 0; i < height; i++) {
        s[i * width + width - 1] = '\n';
    }

    for (int i = 0; i < s.size(); i++) {
        if (s[i] == '0' + 1 + WALL_OFFSET) {
            s.at(i) = 'A';
        } else if (s[i] == '0' + 2 + WALL_OFFSET) {
            s.at(i) = 'B';
        } else {
        }
    }

    std::string out(s.begin(), s.end());
    return out;
}
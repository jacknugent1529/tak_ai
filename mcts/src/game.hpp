#ifndef GAME_H_
#define GAME_H_

#include <cstdint>
#include <vector>
#include <string>

#define MAX_HEIGHT 8

typedef struct {
    uint8_t board[4][4][MAX_HEIGHT + 1]; // top is the 0th element
    // use 1 and 2 to represent flat pieces of p1 and p2
    // use 11 and 12 to represent wall pieces of p1 and p2
    uint8_t p1_pieces_rem;
    uint8_t p2_pieces_rem;
    uint8_t turn; // 1 or 2
} tak_game_t;

typedef enum {
    MOVE,
    FLAT,
    WALL
} move_option_t;

typedef struct {
    move_option_t move;
    uint8_t i;
    uint8_t j;
    int8_t di;
    int8_t dj;
    uint8_t drop0;
    uint8_t drop1;
    uint8_t drop2;
} move_t;

std::vector<move_t> available_moves(tak_game_t *game);

void apply_move(tak_game_t *new_game, tak_game_t *old_game, move_t *move);

std::string game_to_string(tak_game_t *game);

typedef enum {
    IN_PROGRESS,
    P1_WIN,
    P2_WIN,
    TIE
} game_outcome_t;

game_outcome_t game_outcome(tak_game_t *game);

float tiles_eval(tak_game_t *game);

bool move_eq(move_t move1, move_t move2);

tak_game_t new_tak_game();

int get_tower_height(tak_game_t *game, uint8_t i, uint8_t j);

#endif // define GAME_H_
//
// Created by prabh on 11/6/22.
//

#ifndef GAME_GAME_H
#define GAME_GAME_H

#include <stdbool.h>
#include <stdint.h>

/**
 * The number of bytes in the game state array.
 */
#define GAME_STATE_BYTES 9

/**
 * Game
 * <p>
 * Holds the information and interfaces necessary to run the game.
 * </p>
 */
struct Game
{
    char trackGame[GAME_STATE_BYTES];
    char turn;
    int cursor;
    int winCondition;
    
    void (*updateGameState)(struct Game *, const uint8_t *, const char *, const uint8_t *);
    
    void (*updateBoard)(struct Game *);
    
    void (*displayBoardWithCursor)(struct Game *);
    
    bool (*isGameOver)(struct Game *);
};

/**
 * initializeGame
 * <p>
 * Allocate memory for a game struct. Set the default values in the game. Set the function pointers in the game struct.
 * </p>
 * @return - pointer to the initialized game
 */
struct Game *initializeGame(void);

#endif //GAME_GAME_H

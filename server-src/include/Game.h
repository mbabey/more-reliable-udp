//
// Created by prabh on 11/6/22.
//

#ifndef GAME_GAME_H
#define GAME_GAME_H

#include <stdbool.h>

struct Game {
    char trackGame[9];
    char turn;
    int curser;
    int winCondition;
};

//TODO: INITIALIZE STRUCT
/**
 * Pointer to game to be initialized.
 * @param game Pointer to current game.
 */
void initializeGame(struct Game* game);

//TODO: DISPLAY BOARD.
/**
 * Print Board on screen.
 * @param game Pointer to current game.
 */
void displayBoardEnd(struct Game* game);

//TODO: DISPOAY CURSER ON BOARD.
void displayBoardWithCursor(struct Game* game);

//TODO: VALIDATE MOVE.
/**
 * Takes in a cell location and updates board if valid.
 * @param move cell to update.
 * @param game Pointer to current game.
 * @return
 */
bool validateMove(struct Game* game);

//TODO: UPDATE BOARD.
/**
 * Updates a cell to a new character.
 * @param piece Piece to update to.
 * @param cell Location to update.
 */
void updateBoard(struct Game* game);

//TODO: GET BOARD.
/**
 * Clears terminal for board to display.
 */
void refreshCommandLine();

//TODO: CHECK IF THERE IS A WIN
/**
 * Check board if win condition is met or board is full.
 * @param game
 * @return
 */
bool isGameOver(struct Game* game);

/**
 * Update the cursor position for the horizontal value.
 * @param currentGame
 * @param newPosition
 */
void updateCursorHorizontal(struct Game* currentGame, int joystickY);

/**
 * Update the cursor position for the vertical value.
 * @param currentGame
 * @param joystickX
 */
void updateCursorVertical(struct Game* currentGame, int joystickX);

/**
 * Determine if the grid is full.
 * @param currentGame
 * @return
 */
bool isGridFull(struct Game* currentGame);

/**
 * Displays the status of the current game.
 * @param game Pointer to the current game.
 */
void displayDetails(struct Game* game);



#endif //GAME_GAME_H

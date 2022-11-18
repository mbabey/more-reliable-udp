//
// Created by prabh on 11/6/22.
//
#include <stdlib.h>
#include <stdio.h>
#include "../include/Game.h"
#include <ctype.h>
#include <string.h>

/**
 * updateGameState
 * <p>
 * Update the game state with a new cursor position, turn character, and track game array.
 * </p>
 * @param game - the game to update
 * @param new_cursor - the new cursor position
 * @param new_turn - the new turn char
 * @param new_trackGame - the new track game array
 */
void updateGameState(struct Game *game,
                     const uint8_t *new_cursor,
                     const char *new_turn,
                     const uint8_t *new_trackGame);

struct Game *initializeGame(void)
{
    // Make game object.
    struct Game *game = malloc(sizeof(struct Game));
    
    // All cells are empty to start.
    for (int i = 0; i < GAME_STATE_BYTES; i++)
    {
        game->trackGame[i] = ' ';
    }
    
    // X goes first.
    game->turn = 'X';
    
    // Start cursor in the center.
    game->cursor = 4;
    
    game->updateGameState = updateGameState;
    
    game->updateBoard = updateBoard;
    
    game->displayBoardWithCursor = displayBoardWithCursor;
    
    return game;
}

void updateGameState(struct Game *game,
                     const uint8_t *new_cursor,
                     const char *new_turn,
                     const uint8_t *new_trackGame)
{
    if (game == NULL)
    {
        return;
    }
    
    if (new_cursor != NULL)
    {
        game->cursor = *new_cursor;
    }
    if (new_turn != NULL)
    {
        game->turn = *new_turn;
    }
    if (new_trackGame != NULL)
    {
        memcpy(game->trackGame, new_trackGame, GAME_STATE_BYTES);
    }
}

void refreshCommandLine()
{
    system("clear");
}

//TODO: CHANGE COLOR ON WIN OR LOSE.
void displayBoardEnd(struct Game* game) {
    refreshCommandLine();
    char red[] = "\033[0;31m";
    char green[] = "\033[1;32m";
    char endColor[] = "\033[0m";

    // ROW 1
    if(game->winCondition == 1) {
        printf("     |     |     \n");
        printf(" %s%c   |  %c  |  %c%s \n",green, game->trackGame[0], game->trackGame[1], game->trackGame[2], endColor);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    }

    // ROW 2
    else if(game->winCondition == 2) {
        printf("     |     |     \n");
        printf(" %c   |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c  |  %c  |  %c%s \n", green, game->trackGame[3], game->trackGame[4], game->trackGame[5], endColor);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    }

    // ROW 3
    else if(game->winCondition == 3) {
        printf("     |     |     \n");
        printf(" %c   |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c  |  %c  |  %c%s \n", green, game->trackGame[6], game->trackGame[7], game->trackGame[8], endColor);
        printf("     |     |     \n\n");
    }

    // DIAGONAL LEFT
    else if(game->winCondition == 4) {
        printf("     |     |     \n");
        printf(" %s%c%s   |  %c  |  %c \n", green,game->trackGame[0],endColor, game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %s%c%s  |  %c \n", game->trackGame[3], green,game->trackGame[4],endColor, game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %s%c%s \n", game->trackGame[6], game->trackGame[7], green,game->trackGame[8], endColor);
        printf("     |     |     \n\n");
    }

    // DIAGONAL RIGHT
    else if(game->winCondition == 5) {
        printf("     |     |     \n");
        printf(" %c   |  %c  |  %s%c%s \n", game->trackGame[0], game->trackGame[1], green,game->trackGame[2],endColor);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %s%c%s  |  %c \n", game->trackGame[3], green,game->trackGame[4],endColor, game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c%s  |  %c  |  %c \n", green,game->trackGame[6],endColor, game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    }

    // COLUMN 1
    else if(game->winCondition == 6) {
        printf("     |     |     \n");
        printf(" %s%c%s   |  %c  |  %c \n", green,game->trackGame[0],endColor, game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c%s  |  %c  |  %c \n", green,game->trackGame[3],endColor, game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c%s  |  %c  |  %c \n", green,game->trackGame[6],endColor, game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    }

    // COLUMN 2
    else if(game->winCondition == 7) {
        printf("     |     |     \n");
        printf(" %c   |  %s%c%s  |  %c \n", game->trackGame[0], green,game->trackGame[1],endColor, game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %s%c%s  |  %c \n", game->trackGame[3], green,game->trackGame[4],endColor, game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %s%c%s  |  %c \n", game->trackGame[6], green,game->trackGame[7],endColor, game->trackGame[8]);
        printf("     |     |     \n\n");
    }

    // COLUMN 3
    else if(game->winCondition == 8) {
        printf("     |     |     \n");
        printf(" %c   |  %c  |  %s%c%s \n", game->trackGame[0], game->trackGame[1], green,game->trackGame[2],endColor);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %s%c%s \n", game->trackGame[3], game->trackGame[4], green,game->trackGame[5],endColor);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %s%c%s \n", game->trackGame[6], game->trackGame[7], green,game->trackGame[8],endColor);
        printf("     |     |     \n\n");
    }

    // NO WINNERS
    else {
        printf(red);
        printf("     |     |     \n");
        printf(" %c   |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
        printf(endColor);
    }
}

void displayBoardWithCursor(struct Game *game)
{
    refreshCommandLine();
    
    // TODO: CHECK IF WON, IF TIE, OR CONTINUE GAME CHANGE PRINT.
    
    if (game->cursor == 0)
    {
        printf("     |     |     \n");
        printf(" |%c| |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    } else if (game->cursor == 1)
    {
        printf("     |     |     \n");
        printf("  %c  | |%c| |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    } else if (game->cursor == 2)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  | |%c|\n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    } else if (game->cursor == 3)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf(" |%c| |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    } else if (game->cursor == 4)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  | |%c| |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    } else if (game->cursor == 5)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  | |%c|\n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    } else if (game->cursor == 6)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf(" |%c| |  %c  |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    } else if (game->cursor == 7)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  | |%c| |  %c \n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    } else if (game->cursor == 8)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[0], game->trackGame[1], game->trackGame[2]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[3], game->trackGame[4], game->trackGame[5]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  | |%c|\n", game->trackGame[6], game->trackGame[7], game->trackGame[8]);
        printf("     |     |     \n\n");
    }

    // Displays status (selecting, Win, and tie).
    displayDetails(game);
}

//TODO: USE ASCII TO MAKE TIE/WIN SPLASH BIGGER.
void displayDetails(struct Game* game) {

    // Primary status is to check a win.
    if(isGameOver(game)) {
        //TODO: SHOW SPECIAL END GAME SCREEN WITH COLOR.
        displayBoardEnd(game); // end splash no cursor.

        printf("Player");

        if(game->turn == 'X')
            printf(" 1 (X) has WON!!!!!!!!\n");
        else if(game->turn == 'O')
            printf(" 2 (O)  has WON!!!!!!!!\n");
        else
            printf("S BOTH LOSE!!!!\n");
    }

    // Default status is game is still ongoing.
    else {
        printf("Player ");

        if(game->turn == 'X')
            printf("1 (X)");
        else
            printf("2 (O)");

        printf(" is selecting...\n");
    }
}

/**
 * Determine if the grid is full and no more moves can be made.
 * @param game Pointer to the game struct.
 * @return True/False of if the grid is already full.
 */
bool isGridFull(struct Game *game)
{
    for (int i = 0; i < 9; i++)
    {
        if (isspace(game->trackGame[i]))
            return false;
    }
    
    return true;
}

bool isGameOver(struct Game* game) {

    // Row 1.
    if(!isspace(game->trackGame[0]) && game->trackGame[0] == game->trackGame[1] && game->trackGame[1] == game->trackGame[2]) {
        game->turn = game->trackGame[0];
        game->winCondition = 1;
        return true;
    }


    // Row 2.
    else if(!isspace(game->trackGame[3]) && game->trackGame[3] == game->trackGame[4] && game->trackGame[4] == game->trackGame[5]){
        game->turn = game->trackGame[3];
        game->winCondition = 2;
        return true;
    }

    // Row 3.
    else if(!isspace(game->trackGame[6]) && game->trackGame[6] == game->trackGame[7] && game->trackGame[7] == game->trackGame[8]){
        game->turn = game->trackGame[6];
        game->winCondition = 3;
        return true;
    }

    // Diagonal left.
    else if(!isspace(game->trackGame[0]) && game->trackGame[0] == game->trackGame[4] && game->trackGame[4] == game->trackGame[8]){
        game->turn = game->trackGame[0];
        game->winCondition = 4;
        return true;
    }

    // Diagonal right.
    else if(!isspace(game->trackGame[2]) && game->trackGame[2] == game->trackGame[4] && game->trackGame[4] == game->trackGame[6]){
        game->turn = game->trackGame[2];
        game->winCondition = 5;
        return true;
    }

    // Column 1.
    else if(!isspace(game->trackGame[0]) && game->trackGame[0] == game->trackGame[3] && game->trackGame[3] == game->trackGame[6]){
        game->turn = game->trackGame[0];
        game->winCondition = 6;
        return true;
    }

    // Column 2.
    else if(!isspace(game->trackGame[1]) && game->trackGame[1] == game->trackGame[4] && game->trackGame[4] == game->trackGame[7]){
        game->turn = game->trackGame[1];
        game->winCondition = 7;
        return true;
    }

    // Column 3.
    else if(!isspace(game->trackGame[2]) && game->trackGame[2] == game->trackGame[5] && game->trackGame[5] == game->trackGame[8]){
        game->turn = game->trackGame[2];
        game->winCondition = 8;
        return true;
    }

    // Check if tie and game should end.
    else if(isGridFull(game)){
        game->turn = ' ';
        game->winCondition = 0;
        return true;
    }

    // No wins.
    else
        return false;
}

bool validateMove(struct Game *currentGame)
{
    int tempCurser = currentGame->cursor;
    
    printf("Cell:%d character:%c\n", tempCurser, currentGame->trackGame[tempCurser]); // FOR TESTING DELETE LATER.
    return isspace(currentGame->trackGame[tempCurser]);
}

void updateBoard(struct Game* game) {

    if(validateMove(game)) {
        int cell = game->cursor;

        // Update character in the specified cell.
        game->trackGame[cell] = game->turn;

        // Alternate active player.

        if(game->turn == 'X')
            game->turn = 'O';

        else
            game->turn = 'X';
    }
}

void updateCursorVertical(struct Game *currentGame, int joystickX)
{
    int up         = 0;
    int down       = 245;
    int tempCursor = currentGame->cursor;
    
    if (joystickX == up && tempCursor - 3 >= 0)
        currentGame->cursor -= 3;
    
    else if (joystickX >= down && tempCursor + 3 <= 8)
        currentGame->cursor += 3;
}

void updateCursorHorizontal(struct Game *currentGame, int joystickY)
{
    int left       = 245;
    int right      = 0;
    int tempCursor = currentGame->cursor;
    
    if (joystickY >= left && tempCursor - 1 >= 0)
        currentGame->cursor -= 1;
    
    else if (joystickY == right && tempCursor + 1 <= 8)
        currentGame->cursor += 1;
}

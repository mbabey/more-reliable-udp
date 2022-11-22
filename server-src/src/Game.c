//
// Created by prabh on 11/6/22.
//
#include "../../client-src/include/Game.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * ANSI code to clear the terminal window.
 * <ul>
 * <li>033 is an escape character</li>
 * <li>[1:1H moves the cursor to the top left of the terminal window</li>
 * <li>[2J moves all text currently in the terminal to the scroll-back buffer.</li>
 * </ul>
 */
#define CLEAR_SCREEN printf("\033[1;1H\033[2J")

/**
 * ANSI codes used to colour text.
 */
#define GREEN "\033[1;32m"
#define RED "\033[0;31m"
#define TEXT_DEF "\033[0m"

/**
* Board positions translated into array indices.
*/
#define TOP_LEFT 0
#define TOP_MIDDLE 1
#define TOP_RIGHT 2
#define MIDDLE_LEFT 3
#define MIDDLE 4
#define MIDDLE_RIGHT 5
#define BOTTOM_LEFT 6
#define BOTTOM_MIDDLE 7
#define BOTTOM_RIGHT 8

/**
 * Win state codes.
 */
#define TIE 0
#define TOP_ROW 1
#define MIDDLE_ROW 2
#define BOTTOM_ROW 3
#define DIAGONAL_LEFT 4
#define DIAGONAL_RIGHT 5
#define LEFT_COLUMN 6
#define MIDDLE_COLUMN 7
#define RIGHT_COLUMN 8

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
    struct Game* game;
    
    // Make game object.
    if((game = malloc(sizeof(struct Game))) == NULL)
    {
        return NULL;
    }
    
    // All cells are empty to start.
    for (int i = 0; i < GAME_STATE_BYTES; i++)
    {
        game->trackGame[i] = ' ';
    }
    
    // X goes first.
    game->turn = 'X';
    
    // Start cursor in the center.
    game->cursor = MIDDLE;
    
    game->updateGameState = updateGameState;
    
    game->updateBoard = updateBoard;
    
    game->displayBoardWithCursor = displayBoardWithCursor;
    
    game->isGameOver = isGameOver;
    
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

//TODO: CHANGE COLOR ON WIN OR LOSE.
void displayBoardEnd(struct Game* game) {
    
    CLEAR_SCREEN;
    
    // ROW 1
    if(game->winCondition == TOP_ROW) {
        printf("     |     |     \n");
        printf("  %s%c   |  %c  |  %c%s \n",GREEN, game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT], TEXT_DEF);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    }

    // ROW 2
    else if(game->winCondition == MIDDLE_ROW) {
        printf("     |     |     \n");
        printf("  %c   |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c  |  %c  |  %c%s \n", GREEN, game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT], TEXT_DEF);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    }

    // ROW 3
    else if(game->winCondition == BOTTOM_ROW) {
        printf("     |     |     \n");
        printf("  %c   |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c  |  %c  |  %c%s \n", GREEN, game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT], TEXT_DEF);
        printf("     |     |     \n\n");
    }

    // DIAGONAL LEFT
    else if(game->winCondition == DIAGONAL_LEFT) {
        printf("     |     |     \n");
        printf("  %s%c%s   |  %c  |  %c \n", GREEN,game->trackGame[TOP_LEFT],TEXT_DEF, game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %s%c%s  |  %c \n", game->trackGame[MIDDLE_LEFT], GREEN,game->trackGame[MIDDLE],TEXT_DEF, game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %s%c%s \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], GREEN,game->trackGame[BOTTOM_RIGHT], TEXT_DEF);
        printf("     |     |     \n\n");
    }

    // DIAGONAL RIGHT
    else if(game->winCondition == DIAGONAL_RIGHT) {
        printf("     |     |     \n");
        printf("  %c   |  %c  |  %s%c%s \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], GREEN,game->trackGame[TOP_RIGHT],TEXT_DEF);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %s%c%s  |  %c \n", game->trackGame[MIDDLE_LEFT], GREEN,game->trackGame[MIDDLE],TEXT_DEF, game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c%s  |  %c  |  %c \n", GREEN,game->trackGame[BOTTOM_LEFT],TEXT_DEF, game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    }

    // COLUMN 1
    else if(game->winCondition == LEFT_COLUMN) {
        printf("     |     |     \n");
        printf("  %s%c%s   |  %c  |  %c \n", GREEN,game->trackGame[TOP_LEFT],TEXT_DEF, game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c%s  |  %c  |  %c \n", GREEN,game->trackGame[MIDDLE_LEFT],TEXT_DEF, game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %s%c%s  |  %c  |  %c \n", GREEN,game->trackGame[BOTTOM_LEFT],TEXT_DEF, game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    }

    // COLUMN 2
    else if(game->winCondition == MIDDLE_COLUMN) {
        printf("     |     |     \n");
        printf("  %c   |  %s%c%s  |  %c \n", game->trackGame[TOP_LEFT], GREEN,game->trackGame[TOP_MIDDLE],TEXT_DEF, game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %s%c%s  |  %c \n", game->trackGame[MIDDLE_LEFT], GREEN,game->trackGame[MIDDLE],TEXT_DEF, game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %s%c%s  |  %c \n", game->trackGame[BOTTOM_LEFT], GREEN,game->trackGame[BOTTOM_MIDDLE],TEXT_DEF, game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    }

    // COLUMN 3
    else if(game->winCondition == RIGHT_COLUMN) {
        printf("     |     |     \n");
        printf("  %c   |  %c  |  %s%c%s \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], GREEN,game->trackGame[TOP_RIGHT],TEXT_DEF);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %s%c%s \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], GREEN,game->trackGame[MIDDLE_RIGHT],TEXT_DEF);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %s%c%s \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], GREEN,game->trackGame[BOTTOM_RIGHT],TEXT_DEF);
        printf("     |     |     \n\n");
    }

    // NO WINNERS
    else {
        printf("%s", RED);
        printf("     |     |     \n");
        printf("  %c   |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
        printf("%s", TEXT_DEF);
    }
}

void displayBoardWithCursor(struct Game *game)
{
    CLEAR_SCREEN;
    
    // TODO: CHECK IF WON, IF TIE, OR CONTINUE GAME CHANGE PRINT.
    
    if (game->cursor == TOP_LEFT)
    {
        printf("     |     |     \n");
        printf(" |%c| |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    } else if (game->cursor == TOP_MIDDLE)
    {
        printf("     |     |     \n");
        printf("  %c  | |%c| |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    } else if (game->cursor == TOP_RIGHT)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  | |%c|\n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    } else if (game->cursor == MIDDLE_LEFT)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf(" |%c| |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    } else if (game->cursor == MIDDLE)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  | |%c| |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    } else if (game->cursor == MIDDLE_RIGHT)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  | |%c|\n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    } else if (game->cursor == BOTTOM_LEFT)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf(" |%c| |  %c  |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    } else if (game->cursor == BOTTOM_MIDDLE)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  | |%c| |  %c \n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
        printf("     |     |     \n\n");
    } else if (game->cursor == BOTTOM_RIGHT)
    {
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[TOP_LEFT], game->trackGame[TOP_MIDDLE], game->trackGame[TOP_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  |  %c \n", game->trackGame[MIDDLE_LEFT], game->trackGame[MIDDLE], game->trackGame[MIDDLE_RIGHT]);
        printf("_____|_____|_____\n");
        printf("     |     |     \n");
        printf("  %c  |  %c  | |%c|\n", game->trackGame[BOTTOM_LEFT], game->trackGame[BOTTOM_MIDDLE], game->trackGame[BOTTOM_RIGHT]);
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
        { printf(" 1 (X) has WON!!!!!!!!\n"); }
        else if(game->turn == 'O')
        { printf(" 2 (O)  has WON!!!!!!!!\n"); }
        else
        { printf("s BOTH LOSE!!!!\n"); }
    }

    // Default status is game is still ongoing.
    else {
        printf("Player ");

        if(game->turn == 'X')
        { printf("1 (X)"); }
        else
        { printf("2 (O)"); }

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
    for (int i = 0; i < GAME_STATE_BYTES; i++)
    {
        if (isspace(game->trackGame[i]))
        { return false; }
    }
    
    return true;
}

bool isGameOver(struct Game* game) {

    // Row 1.
    if(!isspace(game->trackGame[TOP_LEFT]) && game->trackGame[TOP_LEFT] == game->trackGame[TOP_MIDDLE] && game->trackGame[TOP_MIDDLE] == game->trackGame[TOP_RIGHT]) {
        game->turn = game->trackGame[TOP_LEFT];
        game->winCondition = TOP_ROW;
        return true;
    }

    // Row 2.
    if(!isspace(game->trackGame[MIDDLE_LEFT]) && game->trackGame[MIDDLE_LEFT] == game->trackGame[MIDDLE] && game->trackGame[MIDDLE] == game->trackGame[MIDDLE_RIGHT]){
        game->turn = game->trackGame[MIDDLE_LEFT];
        game->winCondition = MIDDLE_ROW;
        return true;
    }

    // Row 3.
    if(!isspace(game->trackGame[BOTTOM_LEFT]) && game->trackGame[BOTTOM_LEFT] == game->trackGame[BOTTOM_MIDDLE] && game->trackGame[BOTTOM_MIDDLE] == game->trackGame[BOTTOM_RIGHT]){
        game->turn = game->trackGame[BOTTOM_LEFT];
        game->winCondition = BOTTOM_ROW;
        return true;
    }

    // Diagonal left.
    if(!isspace(game->trackGame[TOP_LEFT]) && game->trackGame[TOP_LEFT] == game->trackGame[MIDDLE] && game->trackGame[MIDDLE] == game->trackGame[BOTTOM_RIGHT]){
        game->turn = game->trackGame[TOP_LEFT];
        game->winCondition = DIAGONAL_LEFT;
        return true;
    }

    // Diagonal right.
    if(!isspace(game->trackGame[TOP_RIGHT]) && game->trackGame[TOP_RIGHT] == game->trackGame[MIDDLE] && game->trackGame[MIDDLE] == game->trackGame[BOTTOM_LEFT]){
        game->turn = game->trackGame[TOP_RIGHT];
        game->winCondition = DIAGONAL_RIGHT;
        return true;
    }

    // Column 1.
    if(!isspace(game->trackGame[TOP_LEFT]) && game->trackGame[TOP_LEFT] == game->trackGame[MIDDLE_LEFT] && game->trackGame[MIDDLE_LEFT] == game->trackGame[BOTTOM_LEFT]){
        game->turn = game->trackGame[TOP_LEFT];
        game->winCondition = LEFT_COLUMN;
        return true;
    }

    // Column 2.
    if(!isspace(game->trackGame[TOP_MIDDLE]) && game->trackGame[TOP_MIDDLE] == game->trackGame[MIDDLE] && game->trackGame[MIDDLE] == game->trackGame[BOTTOM_MIDDLE]){
        game->turn = game->trackGame[TOP_MIDDLE];
        game->winCondition = MIDDLE_COLUMN;
        return true;
    }

    // Column 3.
    if(!isspace(game->trackGame[TOP_RIGHT]) && game->trackGame[TOP_RIGHT] == game->trackGame[MIDDLE_RIGHT] && game->trackGame[MIDDLE_RIGHT] == game->trackGame[BOTTOM_RIGHT]){
        game->turn = game->trackGame[TOP_RIGHT];
        game->winCondition = RIGHT_COLUMN;
        return true;
    }

    // Check if tie and game should end.
    if(isGridFull(game)){
        game->turn = ' ';
        game->winCondition = TIE; /* Not really a tie. */
        return true;
    }

    // No wins.
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
        game->turn = (game->turn == 'X') ? 'O' : 'X';
    }
}

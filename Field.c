/*
 * Created on March 8, 2022, 8:41 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "Agent.h"
#include "Ascii.h"
#include "BattleBoats.h"
#include "Buttons.h"
#include "CircularBuffer.h"
#include "Field.h"
#include "FieldOled.h"
#include "Message.h"
#include "Negotiation.h"
#include "Oled.h"
#include "OledDriver.h"
#include "Uart1.h"

#define direction 0x01
/*
 * 
 */

/**
 * This function is optional, but recommended.   It prints a representation of both
 * fields, similar to the OLED display.
 * @param f The field to initialize.
 * @param p The data to initialize the entire field to, should be a member of enum
 *                     SquareStatus.
 */
void FieldPrint_UART(Field *own_field, Field * opp_field) {
    int row;
    int column;
    for (row = 0; row < 6; row++) {
        for (column = 0; column < 10; column++) {
            printf(" %d", own_field->grid[row][column]);
        }
        printf("\n");
    }
    printf("\n");
    for (row = 0; row < 6; row++) {
        for (column = 0; column < 10; column++) {
            printf(" %d", opp_field->grid[row][column]);
        }
        printf("\n");
    }

}

/**
 * FieldInit() will initialize two passed field structs for the beginning of play.
 * Each field's grid should be filled with the appropriate SquareStatus (
 * FIELD_SQUARE_EMPTY for your own field, FIELD_SQUARE_UNKNOWN for opponent's).
 * Additionally, your opponent's field's boatLives parameters should be filled
 *  (your own field's boatLives will be filled when boats are added)
 * 
 * FieldAI_PlaceAllBoats() should NOT be called in this function.
 * 
 * @param own_field     //A field representing the agents own ships
 * @param opp_field     //A field representing the opponent's ships
 */
void FieldInit(Field *own_field, Field * opp_field) {
    for (int i = 0; i < FIELD_ROWS; i++) { //i = rows
        for (int y = 0; y < FIELD_COLS; y++) { //y = columns
            own_field ->grid[y][i] = FIELD_SQUARE_EMPTY;
            opp_field ->grid[y][i] = FIELD_SQUARE_UNKNOWN;
        }
    }

    own_field->smallBoatLives = 3;
    own_field->mediumBoatLives = 4;
    own_field->largeBoatLives = 5;
    own_field->hugeBoatLives = 6;
    opp_field->smallBoatLives = 3;
    opp_field->mediumBoatLives = 4;
    opp_field->largeBoatLives = 5;
    opp_field->hugeBoatLives = 6;

}

/**
 * Retrieves the value at the specified field position.
 * @param f     //The Field being referenced
 * @param row   //The row-component of the location to retrieve
 * @param col   //The column-component of the location to retrieve
 * @return  FIELD_SQUARE_INVALID if row and col are not valid field locations
 *          Otherwise, return the status of the referenced square 
 */
SquareStatus FieldGetSquareStatus(const Field *f, uint8_t row, uint8_t col) {
    if (row > FIELD_ROWS || col > FIELD_COLS || row < 0 || col < 0) {
        return FIELD_SQUARE_INVALID;
    }
    return f->grid[row][col];
}

/**
 * This function provides an interface for setting individual locations within a Field struct. This
 * is useful when FieldAddBoat() doesn't do exactly what you need. For example, if you'd like to use
 * FIELD_SQUARE_CURSOR, this is the function to use.
 * 
 * @param f The Field to modify.
 * @param row The row-component of the location to modify
 * @param col The column-component of the location to modify
 * @param p The new value of the field location
 * @return The old value at that field location
 */
SquareStatus FieldSetSquareStatus(Field *f, uint8_t row, uint8_t col, SquareStatus p) {
    // is this right? I have no idea //
    //                               //
    //                               //
    SquareStatus oldstatus = f->grid[row][col];
    f->grid[row][col] = p;
    return oldstatus;
}

/**
 * FieldAddBoat() places a single ship on the player's field based on arguments 2-5. Arguments 2, 3
 * represent the x, y coordinates of the pivot point of the ship.  Argument 4 represents the
 * direction of the ship, and argument 5 is the length of the ship being placed. 
 * 
 * All spaces that
 * the boat would occupy are checked to be clear before the field is modified so that if the boat
 * can fit in the desired position, the field is modified as SUCCESS is returned. Otherwise the
 * field is unmodified and STANDARD_ERROR is returned. There is no hard-coded limit to how many
 * times a boat can be added to a field within this function.
 * 
 * In addition, this function should update the appropriate boatLives parameter of the field.
 *
 * So this is valid test code:
 * {
 *   Field myField;
 *   FieldInit(&myField,FIELD_SQUARE_EMPTY);
 *   FieldAddBoat(&myField, 0, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_SMALL);
 *   FieldAddBoat(&myField, 1, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_MEDIUM);
 *   FieldAddBoat(&myField, 1, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_HUGE);
 *   FieldAddBoat(&myField, 0, 6, FIELD_BOAT_DIRECTION_SOUTH, FIELD_BOAT_TYPE_SMALL);
 * }
 *
 * should result in a field like:
 *      0 1 2 3 4 5 6 7 8 9
 *     ---------------------
 *  0 [ 3 3 3 . . . 3 . . . ]
 *  1 [ 4 4 4 4 . . 3 . . . ]
 *  2 [ . . . . . . 3 . . . ]
 *  3 [ . . . . . . . . . . ]
 *  4 [ . . . . . . . . . . ]
 *  5 [ . . . . . . . . . . ]
 *     
 * @param f The field to grab data from.
 * @param row The row that the boat will start from, valid range is from 0 and to FIELD_ROWS - 1.
 * @param col The column that the boat will start from, valid range is from 0 and to FIELD_COLS - 1.
 * @param dir The direction that the boat will face once places, from the BoatDirection enum.
 * @param boatType The type of boat to place. Relies on the FIELD_SQUARE_*_BOAT values from the
 * SquareStatus enum.
 * @return SUCCESS for success, STANDARD_ERROR for failure
 */
uint8_t FieldAddBoat(Field *own_field, uint8_t row, uint8_t col, BoatDirection dir, BoatType boat_type) {
    int BoatSize;
    SquareStatus Type;
    switch (boat_type) {
        case FIELD_BOAT_TYPE_SMALL:
            BoatSize = FIELD_BOAT_SIZE_SMALL;
            Type = FIELD_SQUARE_SMALL_BOAT;
            break;
        case FIELD_BOAT_TYPE_MEDIUM:
            BoatSize = FIELD_BOAT_SIZE_MEDIUM;
            Type = FIELD_SQUARE_MEDIUM_BOAT;
            break;
        case FIELD_BOAT_TYPE_LARGE:
            BoatSize = FIELD_BOAT_SIZE_LARGE;
            Type = FIELD_SQUARE_LARGE_BOAT;
            break;
        case FIELD_BOAT_TYPE_HUGE:
            BoatSize = FIELD_BOAT_SIZE_HUGE;
            Type = FIELD_SQUARE_HUGE_BOAT;
            break;
        default:
            return STANDARD_ERROR;
    }
    if (dir == FIELD_DIR_SOUTH) {
        if (row >= 0 && row + BoatSize <= FIELD_ROWS - 1) {
            for (int i = 0; i < BoatSize; i++) {
                own_field->grid[row + i][col] = Type;
                return SUCCESS;
            }
        } else {
            return STANDARD_ERROR;
        }
    } else if (dir == FIELD_DIR_EAST) {
        if (col >= 0 && col + BoatSize <= FIELD_COLS - 1) {
            for (int i = 0; i < BoatSize; i++) {
                own_field->grid[row][col + i] = Type;
                return SUCCESS;
            }
        } else {
            return STANDARD_ERROR;
        }
    } else {
        return STANDARD_ERROR;
    }
    if (boat_type == FIELD_BOAT_TYPE_SMALL) {
        own_field->smallBoatLives = FIELD_BOAT_SIZE_SMALL;
    } else if (boat_type == FIELD_BOAT_TYPE_MEDIUM) {
        own_field->mediumBoatLives = FIELD_BOAT_SIZE_MEDIUM;
    } else if (boat_type == FIELD_BOAT_TYPE_LARGE) {
        own_field->largeBoatLives = FIELD_BOAT_SIZE_LARGE;
    } else if (boat_type == FIELD_BOAT_TYPE_HUGE) {
        own_field->hugeBoatLives = FIELD_BOAT_SIZE_HUGE;
    } else {
        return STANDARD_ERROR;
    }
}

/**
 * This function registers an attack at the gData coordinates on the provided field. This means that
 * 'f' is updated with a FIELD_SQUARE_HIT or FIELD_SQUARE_MISS depending on what was at the
 * coordinates indicated in 'gData'. 'gData' is also updated with the proper HitStatus value
 * depending on what happened AND the value of that field position BEFORE it was attacked. Finally
 * this function also reduces the lives for any boat that was hit from this attack.
 * @param f The field to check against and update.
 * @param gData The coordinates that were guessed. The result is stored in gData->result as an
 *               output.  The result can be a RESULT_HIT, RESULT_MISS, or RESULT_***_SUNK.
 * @return The data that was stored at the field position indicated by gData before this attack.
 */
SquareStatus FieldRegisterEnemyAttack(Field *own_field, GuessData *opp_guess) {
    int row = opp_guess->row;
    int col = opp_guess->col;
    if (own_field->grid[row][col] == FIELD_SQUARE_EMPTY) {
        opp_guess->result = FIELD_SQUARE_MISS;
    } else {
        switch (own_field->grid[row][col]) {
            case FIELD_BOAT_TYPE_SMALL:
                if (own_field->smallBoatLives == 0) {
                    opp_guess->result = RESULT_SMALL_BOAT_SUNK;
                } else {
                    opp_guess->result = FIELD_SQUARE_HIT;
                    own_field->smallBoatLives - 1;
                }
                break;
            case FIELD_BOAT_TYPE_MEDIUM:
                if (own_field->mediumBoatLives == 0) {
                    opp_guess->result = RESULT_MEDIUM_BOAT_SUNK;
                } else {
                    opp_guess->result = FIELD_SQUARE_HIT;
                    own_field->mediumBoatLives - 1;
                }
                break;
            case FIELD_BOAT_TYPE_LARGE:
                if (own_field->largeBoatLives == 0) {
                    opp_guess->result = RESULT_LARGE_BOAT_SUNK;
                } else {
                    opp_guess->result = FIELD_SQUARE_HIT;
                    own_field->largeBoatLives - 1;
                }
                break;
            case FIELD_BOAT_TYPE_HUGE:
                if (own_field->hugeBoatLives == 0) {
                    opp_guess->result = RESULT_HUGE_BOAT_SUNK;
                } else {
                    opp_guess->result = FIELD_SQUARE_HIT;
                    own_field->hugeBoatLives - 1;
                }
                break;
        }
    }
    return own_field->grid[row][col];
}

/**
 * This function updates the FieldState representing the opponent's game board with whether the
 * guess indicated within gData was a hit or not. If it was a hit, then the field is updated with a
 * FIELD_SQUARE_HIT at that position. If it was a miss, display a FIELD_SQUARE_EMPTY instead, as
 * it is now known that there was no boat there. The FieldState struct also contains data on how
 * many lives each ship has. Each hit only reports if it was a hit on any boat or if a specific boat
 * was sunk, this function also clears a boats lives if it detects that the hit was a
 * RESULT_*_BOAT_SUNK.
 * @param f The field to grab data from.
 * @param gData The coordinates that were guessed along with their HitStatus.
 * @return The previous value of that coordinate position in the field before the hit/miss was
 * registered.
 */
SquareStatus FieldUpdateKnowledge(Field *opp_field, const GuessData *own_guess) {
    int row = own_guess->row;
    int col = own_guess->col;
    SquareStatus PreviousSpot = opp_field->grid[row][col];
    if (own_guess->result == RESULT_MISS) {
        opp_field->grid[row][col] = FIELD_SQUARE_MISS;
    } else {
        if (own_guess->result == RESULT_HIT) {
            opp_field->grid[row][col] = FIELD_SQUARE_HIT;
        } else if (own_guess->result == RESULT_SMALL_BOAT_SUNK) {
            opp_field->smallBoatLives = 0;
            opp_field->grid[row][col] = FIELD_SQUARE_HIT;
        }//once sunk we know which ship we sunk and set that boat life to 0
        else if (own_guess->result == RESULT_MEDIUM_BOAT_SUNK) {
            opp_field->mediumBoatLives = 0;
            opp_field->grid[row][col] = FIELD_SQUARE_HIT;
        } else if (own_guess->result == RESULT_LARGE_BOAT_SUNK) {
            opp_field->largeBoatLives = 0;
            opp_field->grid[row][col] = FIELD_SQUARE_HIT;
        } else if (own_guess->result == RESULT_HUGE_BOAT_SUNK) {
            opp_field->hugeBoatLives = 0;
            opp_field->grid[row][col] = FIELD_SQUARE_HIT;
        }
    }
    return PreviousSpot;
}

/**
 * This function returns the alive states of all 4 boats as a 4-bit bitfield (stored as a uint8).
 * The boats are ordered from smallest to largest starting at the least-significant bit. So that:
 * 0b00001010 indicates that the small boat and large boat are sunk, while the medium and huge boat
 * are still alive. See the BoatStatus enum for the bit arrangement.
 * @param f The field to grab data from.
 * @return A 4-bit value with each bit corresponding to whether each ship is alive or not.
 */
uint8_t FieldGetBoatStates(const Field *f) {
    uint8_t boatbits = 0x00;
    if (f->smallBoatLives > 0) {
        boatbits = boatbits | FIELD_BOAT_SIZE_SMALL;
    }
    if (f->mediumBoatLives > 0) {
        boatbits = boatbits | FIELD_BOAT_STATUS_MEDIUM;
    }
    if (f->largeBoatLives > 0) {
        boatbits = boatbits | FIELD_BOAT_STATUS_LARGE;
    }
    if (f->hugeBoatLives > 0) {
        boatbits = boatbits | FIELD_BOAT_STATUS_HUGE;
    }
    return boatbits;

}

/**
 * This function is responsible for placing all four of the boats on a field.
 * 
 * @param f         //agent's own field, to be modified in place.
 * @return SUCCESS if all boats could be placed, STANDARD_ERROR otherwise.
 * 
 * This function should never fail when passed a properly initialized field!
 */
uint8_t FieldAIPlaceAllBoats(Field *own_field) {
    uint8_t dir = rand() % direction; //starting direction
    int rows;
    int cols;
    BOOL BoatSmallCheck = FALSE;
    BOOL BoatMediumCheck = FALSE;
    BOOL BoatLargeCheck = FALSE;
    BOOL BoatHugeCheck = FALSE;
    //randomly finds a position to put the boat
    while (BoatSmallCheck = FALSE) {
        if (direction == FIELD_DIR_SOUTH) {
            rows = rand() % FIELD_ROWS - FIELD_BOAT_SIZE_SMALL + 1;
            cols = rand() % FIELD_COLS;
        } else {
            rows = rand() % FIELD_ROWS;
            cols = rand() % FIELD_COLS - FIELD_BOAT_SIZE_SMALL + 1;
        }
        if (FieldAddBoat(own_field, rows, cols, direction, FIELD_BOAT_TYPE_SMALL) == SUCCESS) {
            BoatSmallCheck = TRUE;
        }
        //change direction of the boat incase its not fitting its not needed for the small boat but you never know
        dir = dir ^ direction;
    }
    //randomly finds a position to put the boat
    while (BoatMediumCheck = FALSE) {
        if (direction == FIELD_DIR_SOUTH) {
            rows = rand() % FIELD_ROWS - FIELD_BOAT_SIZE_MEDIUM + 1;
            cols = rand() % FIELD_COLS;
        } else {
            rows = rand() % FIELD_ROWS;
            cols = rand() % FIELD_COLS - FIELD_BOAT_SIZE_MEDIUM + 1;
        }
        if (FieldAddBoat(own_field, rows, cols, direction, FIELD_BOAT_TYPE_MEDIUM) == SUCCESS) {
            BoatMediumCheck = TRUE;
        }
        //change direction of the boat
        dir = dir ^ direction;
    }
    while (BoatLargeCheck = FALSE) {
        if (direction == FIELD_DIR_SOUTH) {
            rows = rand() % FIELD_ROWS - FIELD_BOAT_SIZE_LARGE + 1;
            cols = rand() % FIELD_COLS;
        } else {
            rows = rand() % FIELD_ROWS;
            cols = rand() % FIELD_COLS - FIELD_BOAT_SIZE_LARGE + 1;
        }
        if (FieldAddBoat(own_field, rows, cols, direction, FIELD_BOAT_TYPE_MEDIUM) == SUCCESS) {
            BoatLargeCheck = TRUE;
        }
        //change direction of the boat
        dir = dir ^ direction;
    }
    while (BoatHugeCheck = FALSE) {
        if (direction == FIELD_DIR_SOUTH) {
            rows = 0;
            cols = rand() % FIELD_COLS;
        } else {
            rows = rand() % FIELD_ROWS;
            cols = rand() % FIELD_COLS - FIELD_BOAT_SIZE_HUGE + 1;
        }
        if (FieldAddBoat(own_field, rows, cols, direction, FIELD_BOAT_TYPE_MEDIUM) == SUCCESS) {
            BoatHugeCheck = TRUE;
        }
        //change direction of the boat
        dir = dir ^ direction;
    }
    if (BoatSmallCheck == TRUE && BoatMediumCheck == TRUE && BoatLargeCheck == TRUE && BoatHugeCheck == TRUE) {
        return SUCCESS;
    }
    return STANDARD_ERROR;
}

/**
 * Given a field, decide the next guess.
 *
 * This function should not attempt to shoot a square which has already been guessed.
 *
 * You may wish to give this function static variables.  If so, that data should be
 * reset when FieldInit() is called.
 * 
 * @param f an opponent's field.
 * @return a GuessData struct whose row and col parameters are the coordinates of the guess.  The 
 *           result parameter is irrelevant.
 */
GuessData FieldAIDecideGuess(const Field *opp_field) {
    GuessData Shot;
    int rows = 0;
    int cols = 0;
    BOOL FIRE = FALSE;
    while (FIRE = FALSE) {
        rows = rand() % FIELD_ROWS;
        cols = rand() & FIELD_COLS;
        if (opp_field->grid[rows][cols] == FIELD_SQUARE_UNKNOWN) {
            FIRE = TRUE;
        }
    }
    Shot.col = cols;
    Shot.row = rows;
    return Shot;
}

/** 
 * For Extra Credit:  Make the two "AI" functions above 
 * smart enough to beat our AI in more than 55% of games.
 */



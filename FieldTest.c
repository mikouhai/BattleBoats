/* 
 * File:   MessageTest.c
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include "Field.h"
#include "Uart1.h"
#include "BOARD.h"

static Field homeField;
static Field opponentField;
static Field testerField;
static GuessData guess;

int main()
{
    printf("Testing Field.c\n");
    printf("\nTesting FieldInit and FieldPrint_UART\n");
    FieldInit(&homeField, &opponentField);
    printf("\nDisplaying empty home field and unkown opponent field\n");
    FieldPrint_UART(&homeField, &opponentField);
    
    printf("\nTesting FieldGetSquareStatus\n");
    if (FieldGetSquareStatus(&homeField, 3, 3) == FIELD_SQUARE_EMPTY)
    {
        printf("Passed first test\n");
    }
    else 
    {
        printf("Failed first test\n");
    }
    if (FieldGetSquareStatus(&opponentField, 3, 3) == FIELD_SQUARE_UNKNOWN)
    {
        printf("Passed second  test\n");
    }
    else 
    {
        printf("Failed second test\n");
    }
    
    printf("\nTesting FieldSetSquareStatus\n");
    FieldSetSquareStatus(&homeField, 0, 0, FIELD_SQUARE_SMALL_BOAT);
    if (FieldGetSquareStatus(&homeField, 0, 0) == FIELD_SQUARE_SMALL_BOAT)
    {
        printf("Passed first test\n");
    } 
    else
    {
        printf("Failed first test\n");
    }
    FieldSetSquareStatus(&homeField, 5, 5, FIELD_SQUARE_HUGE_BOAT);
    if (FieldGetSquareStatus(&homeField, 5, 5) == FIELD_SQUARE_HUGE_BOAT)
    {
        printf("Passed second test\n");
    } 
    else
    {
        printf("Failed second test\n");
    }
    FieldInit(&homeField, &opponentField);
    
    printf("\nTesting FieldAddBoat\n");
    FieldAddBoat(&homeField, 5, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_HUGE);
    if  ((FieldGetSquareStatus(&homeField, 5, 0) == FIELD_SQUARE_HUGE_BOAT) && (FieldGetSquareStatus(&homeField, 5, 1) == FIELD_SQUARE_HUGE_BOAT)
        && (FieldGetSquareStatus(&homeField, 5, 2) == FIELD_SQUARE_HUGE_BOAT) && (FieldGetSquareStatus(&homeField, 5, 3) == FIELD_SQUARE_HUGE_BOAT)
        && (FieldGetSquareStatus(&homeField, 5, 4) == FIELD_SQUARE_HUGE_BOAT) && (FieldGetSquareStatus(&homeField, 5, 5) == FIELD_SQUARE_HUGE_BOAT))
    {
        printf("Passed first test\n");
    }
    else 
    {
        printf("Failed first test\n");
    }
    FieldAddBoat(&homeField, 0, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_SMALL);
    if ((FieldGetSquareStatus(&homeField, 0, 0) == FIELD_SQUARE_SMALL_BOAT) && (FieldGetSquareStatus(&homeField, 0, 1) == FIELD_SQUARE_SMALL_BOAT)
       && (FieldGetSquareStatus(&homeField, 0, 2) == FIELD_SQUARE_SMALL_BOAT))
    {
        printf("Passed second test\n");
    }
    else
    {
        printf("Failed second test\n");
    }
    
    FieldInit(&homeField, &opponentField);
    
    printf("\nTesting FieldAIPlaceAllBoats\n");
    if (FieldAIPlaceAllBoats(&homeField) == SUCCESS)
    {
        printf("Passed first test\n");
        FieldInit(&homeField, &opponentField);
        printf("Will now print AI field on home field twice boat patters should be different\n\n");
        FieldAIPlaceAllBoats(&homeField);
        FieldPrint_UART(&homeField, &opponentField);
        printf("\n");
        FieldInit(&homeField, &opponentField);
        FieldAIPlaceAllBoats(&homeField);
        FieldPrint_UART(&homeField, &opponentField);        
        
        
    }
    else
    {
        printf("something failed");
    }
       
    
    printf("\nTesting FieldAIDecideGuess and FieldRegisterEnemyAttack\n");
    FieldInit(&homeField,&opponentField);
    guess = FieldAIDecideGuess(&opponentField);
    SquareStatus shot = FieldRegisterEnemyAttack(&testerField, &guess);
    if (shot == FieldGetSquareStatus(&testerField, guess.row, guess.col))
    {
        printf("test passed\n");
    }
    
    else 
    {
        printf("test failed\n");
    }
    
    printf("\nTesting FieldGetBoatStates\n");
    FieldUpdateKnowledge(&opponentField, &guess);
    if (FieldGetBoatStates(&testerField) == 0b1111)
    {
        printf("test passed\n");
    }
    while(1);
}
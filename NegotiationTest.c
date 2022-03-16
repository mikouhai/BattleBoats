/* 
 * File:   NegotiationTest.c
 * Author: rahua
 *
 * Created on March 11, 2022, 10:35 PM
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include "Message.h"
#include "OledDriver.h"
#include "Oled.h"
#include "Ascii.h"
#include "Negotiation.h"
/*
 * 
 */
int main(void){
    if(NegotiateCoinFlip(10, 10) == TAILS){
        printf("pass\n");
    }
    else{
        printf("FAILED\n");
    }
    printf("\nTesting NegotiationHash\n");
    NegotiationData toBeHashed = 33;
    NegotiationData Hashed = (toBeHashed * toBeHashed) % 0xBEEF;
    if (NegotiationHash(toBeHashed) == Hashed) 
    {
        printf("Passed test\n");
    } 
    else
    {
        printf("Failed Test\n");
    }
    return 0;
}


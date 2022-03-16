#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include "Message.h"
#include "OledDriver.h"
#include "Oled.h"
#include "Ascii.h"

static uint8_t checksum;
static char *param;
static char *parseString;
static int parseResult;
static BB_Event eventTester;
static Message messageTester;
static char message[82];
static char *messageTesterTwo;

int main() 
{
    printf("Testing Message.c Functions\n");
    
    printf("\nTesting Message_CalculateChecksum\n");
    param = "CHA,2";
    checksum = Message_CalculateChecksum(param);
    if (checksum == 0x54)
    {
        printf("Passed first test\n");
    }
    else 
    {
        printf("Failed first test\n");
    }
    param = "REV,4";
    checksum = Message_CalculateChecksum(param);
    if (checksum == 0x59)
    {
        printf("Passed second test\n");
    }
    else 
    {
        printf("Failed second test\n");
    }

    printf("\nTesting Message_ParseMessage\n");
    param = "CHA,2";
    parseString = "54";
    parseResult = Message_ParseMessage(param, parseString, &eventTester);
    if (parseResult && eventTester.type == BB_EVENT_CHA_RECEIVED && eventTester.param0 == 2) 
    {
        printf("Passed first test\n");
    }
    else
    {
        printf("Failed first test\n");
    }
    
    param = "REV,4";
    parseString = "59";
    parseResult = Message_ParseMessage(param, parseString, &eventTester);
    if (parseResult && eventTester.type == BB_EVENT_REV_RECEIVED && eventTester.param0 == 4) 
    {
        printf("Passed second test\n");
    }
    else
    {
        printf("Failed second test\n");
    }
    
    printf("\nTesting Message_Encode\n");
    messageTester.type = MESSAGE_CHA;
    messageTester.param0 = 2;
    Message_Encode(message, messageTester);
    if (strcmp(message, "$CHA,2*54\n") == 0)
    {
        printf("First test passed\n");
    }
    else
    {
       printf("First test failed\n"); 
    }
    
    messageTester.type = MESSAGE_REV;
    messageTester.param0 = 4;
    Message_Encode(message, messageTester);
    if (strcmp(message, "$REV,4*59\n") == 0)
    {
        printf("Second test passed\n");
    }
    else
    {
       printf("Second test failed\n"); 
    }
    
    printf("\nTesting Message_Decode\n");
    int i;
    messageTesterTwo = "$CHA,2*54\n";
    for (i = 0; i < strlen(messageTesterTwo); i++)
    {
        Message_Decode(messageTesterTwo[i], &eventTester);
    }
    if (eventTester.type == BB_EVENT_CHA_RECEIVED && eventTester.param0 == 2)
    {
        printf("First test passed\n");
    }
    else
    {
        printf("First test Failed\n");
    }
    
    messageTesterTwo = "$REV,4*59\n";
    for (i = 0; i < strlen(messageTesterTwo); i++)
    {
        Message_Decode(messageTesterTwo[i], &eventTester);
    }
    if (eventTester.type == BB_EVENT_REV_RECEIVED && eventTester.param0 == 4)
    {
        printf("Second test passed\n");
    }
    else
    {
        printf("Second test Failed\n");
    }
    
    while(1);
}

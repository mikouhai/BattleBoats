/* 
 * File:   Message.c
 * Author: rhuang43
 *
 * Created on March 8, 2022, 8:41 PM
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Message.h"
#include "BOARD.h"
#include "BattleBoats.h"
static int Check = 0; // checks if we are doing checksum or payload
static int i = 0; // index for payload decode
#define pay 1
#define csum 2
/*
 * 
 */

/**
 * Given a payload string, calculate its checksum
 * 
 * @param payload       //the string whose checksum we wish to calculate
 * @return   //The resulting 8-bit checksum 
 */
// Below is a helper function to convert ascii numbers to hex so we can do xor
BB_EventType BB_Ev;

uint8_t Message_CalculateChecksum(const char* payload) {
    int i = 0;
    uint8_t Checksum = payload[i];
    i++;
    for (; i < strlen(payload); i++) {
        Checksum = Checksum ^ payload[i];
    }
    return Checksum;
}

/**
 * ParseMessage() converts a message string into a BB_Event.  The payload and
 * checksum of a message are passed into ParseMessage(), and it modifies a
 * BB_Event struct in place to reflect the contents of the message.
 * 
 * @param payload       //the payload of a message
 * @param checksum      //the checksum (in string form) of  a message,
 *                          should be exactly 2 chars long, plus a null char
 * @param message_event //A BB_Event which will be modified by this function.
 *                      //If the message could be parsed successfully,
 *                          message_event's type will correspond to the message type and 
 *                          its parameters will match the message's data fields.
 *                      //If the message could not be parsed,
 *                          message_events type will be BB_EVENT_ERROR
 * 
 * @return STANDARD_ERROR if:
 *              the payload does not match the checksum
 *              the checksum string is not two characters long
 *              the message does not match any message template
 *          SUCCESS otherwise
 * 
 * Please note!  sscanf() has a couple compiler bugs that make it a very
 * unreliable tool for implementing this function. * 
 */
int Message_ParseMessage(const char* payload,
        const char* checksum_string, BB_Event * message_event) {
    //variable to check the data fields
    char *check;
    //puts a flag down to know which BB event to set message event to
    char *flag;
    if (strlen(checksum_string) != MESSAGE_CHECKSUM_LEN) {
        return STANDARD_ERROR;
    }
    int CheckSum = Message_CalculateChecksum(payload);
    if (CheckSum != (int) checksum_string) {
        return STANDARD_ERROR;
    }
    char *messageID = strtok((char*) payload, ",");
    if (strlen(messageID) != 3) {
        return STANDARD_ERROR;
    }
    char* DataFields = strtok(NULL, "*");
    if (messageID == "RES") {
        if (strlen(DataFields) > 3) {
            return STANDARD_ERROR;
        }
        flag = "RES";

    } else if (messageID == "ACC") {
        if (strlen(DataFields) > 1) {
            return STANDARD_ERROR;
        }
        flag = "ACC";

    } else if (messageID == "REV") {

        if (strlen(DataFields) > 1) {
            return STANDARD_ERROR;
        }
        flag = "REV";
    } else if (messageID == "SHO") {
        if (strlen(DataFields) > 2) {
            return STANDARD_ERROR;
        }
        flag = "SHO";
    } else if (messageID == "CHA") {
        if (strlen(DataFields) > 1) {
            return STANDARD_ERROR;
        }
        flag = "CHA";
    } else {
        message_event = (BB_Event *) BB_EVENT_ERROR;
        return STANDARD_ERROR;

    }
    check = strtok(DataFields, ",");
    if (check > 0) {
        return STANDARD_ERROR;
    }
    while (check != NULL) {
        check = strtok(DataFields, ",");
        if (check > 0) {
            return STANDARD_ERROR;
        }
    }
    if (flag == "RES") {
        message_event = (BB_Event *) BB_EVENT_RES_RECEIVED;
    } else if (flag == "CHA") {
        message_event = (BB_Event *) BB_EVENT_CHA_RECEIVED;
    } else if (flag == "ACC") {
        message_event = (BB_Event *) BB_EVENT_ACC_RECEIVED;
    } else if (flag == "REV") {
        message_event = (BB_Event *) BB_EVENT_REV_RECEIVED;
    } else if (flag == "SHO") {
        message_event = (BB_Event *) BB_EVENT_SHO_RECEIVED;
    } else {
        message_event = (BB_Event *) BB_EVENT_ERROR;
    }
    return SUCCESS;
}

/**
 * Encodes the coordinate data for a guess into the string `message`. This string must be big
 * enough to contain all of the necessary data. The format is specified in PAYLOAD_TEMPLATE_COO,
 * which is then wrapped within the message as defined by MESSAGE_TEMPLATE. 
 * 
 * The final length of this
 * message is then returned. There is no failure mode for this function as there is no checking
 * for NULL pointers.
 * 
 * @param message            The character array used for storing the output. 
 *                              Must be long enough to store the entire string,
 *                              see MESSAGE_MAX_LEN.
 * @param message_to_encode  A message to encode
 * @return                   The length of the string stored into 'message_string'.
                             Return 0 if message type is MESSAGE_NONE.
 */
int Message_Encode(char *message_string, Message message_to_encode) {
    switch (message_to_encode.type) {
        case MESSAGE_NONE:
            return 0;
            break;
        case MESSAGE_CHA:
            sprintf(message_string, "$%s%s%s\n", message_to_encode, PAYLOAD_TEMPLATE_CHA, message_to_encode.param0);
            break;
        case MESSAGE_ACC:
            sprintf(message_string, "$%s%s%s\n", message_to_encode, PAYLOAD_TEMPLATE_ACC, message_to_encode.param0);
            break;
        case MESSAGE_REV:
            sprintf(message_string, "$%s%s%s\n", message_to_encode, PAYLOAD_TEMPLATE_REV, message_to_encode.param0);
            break;
        case MESSAGE_SHO:
            sprintf(message_string, "$%s%s%s%s\n", message_to_encode, PAYLOAD_TEMPLATE_SHO, message_to_encode.param0, message_to_encode.param1);
            break;
        case MESSAGE_RES:
            sprintf(message_string, "$%s%s%s%s%s\n", message_to_encode, PAYLOAD_TEMPLATE_RES, message_to_encode.param0, message_to_encode.param1, message_to_encode.param2);
            break;
    }
    return strlen(message_string);
}

/**
 * Message_Decode reads one character at a time.  If it detects a full NMEA message,
 * it translates that message into a BB_Event struct, which can be passed to other 
 * services.
 * 
 * @param char_in - The next character in the NMEA0183 message to be decoded.
 * @param decoded_message - a pointer to a message struct, used to "return" a message
 *                          if char_in is the last character of a valid message, 
 *                              then decoded_message
 *                              should have the appropriate message type.
 *                          if char_in is the last character of an invalid message,
 *                              then decoded_message should have an ERROR type.
 *                          otherwise, it should have type NO_EVENT.
 * @return SUCCESS if no error was detected
 *         STANDARD_ERROR if an error was detected
 * 
 * note that ANY call to Message_Decode may modify decoded_message.
 */
int Message_Decode(unsigned char char_in, BB_Event * decoded_message_event) {
    static char payload[MESSAGE_MAX_PAYLOAD_LEN];
    static char checksum[MESSAGE_CHECKSUM_LEN];
    if (char_in == '$') {
        Check = pay;
    } else if (char_in == '*') {
        Check = csum;
        i = 0;
        payload[i] = '\0';
    }
    else if(Check == 0){
        return STANDARD_ERROR;
    }
    if (char_in == '\n') {
        Message_ParseMessage(payload, checksum, decoded_message_event);
    }
    switch (Check) {
        case pay:
            payload[i] = char_in;
            i++;
            break;
        case csum:
            if (char_in == 'A' || char_in == 'B' || char_in == 'C' || char_in == 'D') {
                checksum[i] = char_in;
                i++;
            }
            if (char_in == 'E' || char_in == 'F' || char_in == '1' || char_in == '2') {
                checksum[i] = char_in;
                i++;
            }
            if (char_in == '3' || char_in == '4' || char_in == '5' || char_in == '6') {
                checksum[i] = char_in;
                i++;
            }
            if (char_in == '7' || char_in == '8' || char_in == '9' || char_in == '0') {
                checksum[i] = char_in;
                i++;
            }
            else{
                return STANDARD_ERROR;
            }
            break;
    }
    if (strlen(checksum) > MESSAGE_CHECKSUM_LEN + 1){
        return STANDARD_ERROR;
    }
    return SUCCESS;
    
}




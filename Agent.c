#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Agent.h"
#include "Ascii.h"
#include "Message.h"
#include "BattleBoats.h"
#include "BOARD.h"
#include "Buttons.h"
#include "CircularBuffer.h"
#include "FieldOled.h"
#include "Oled.h"
#include "OledDriver.h"
#include "Uart1.h"
#include "Negotiation.h"
#include "Field.h"

struct Agent {
    AgentState state;
    NegotiationData secret;
    NegotiationData hash;
    Field home;
    Field opponent;
    Message message;
};

static struct Agent agent;
static int turns = 0;
static FieldOledTurn playerTurn;
static char *error;

void AgentInit(void) {
    agent.state = AGENT_STATE_START;
    turns = 0;
    playerTurn = FIELD_OLED_TURN_NONE;
}

Message AgentRun(BB_Event event) {
    //switch statement for type of event occuring in BB
    switch (event.type) {
            //No event occurs return none
        case BB_EVENT_NO_EVENT:

            agent.message.type = MESSAGE_NONE;
            break;
            //Start event     
        case BB_EVENT_START_BUTTON:
            //set agent state to start 
            if (agent.state == AGENT_STATE_START) {
                //initialize the fields and generate a random hash
                agent.secret = rand() & 0xFFFF;
                agent.message.param0 = NegotiationHash(agent.secret);
                agent.message.type = MESSAGE_CHA;
                FieldInit(&agent.home, &agent.opponent);
                FieldAIPlaceAllBoats(&agent.home);
                //set agent state to challenging
                agent.state = AGENT_STATE_CHALLENGING;

            } else {
                agent.message.type = MESSAGE_NONE;
            }
            break;
            //reset event 
        case BB_EVENT_RESET_BUTTON:
            //reset data that needs to be reset 
            agent.message.type = MESSAGE_NONE;
            OledClear(OLED_COLOR_BLACK);
            //create and display start message
            char *startMessage = "Press BTN4 to start";
            OledDrawString(startMessage);
            OledUpdate();
            AgentInit();
            return agent.message;
            break;
            //challenger received event
        case BB_EVENT_CHA_RECEIVED:
            if (agent.state == AGENT_STATE_START) {
                //generate random number
                agent.secret = rand() & 0xFFFF;
                agent.hash = event.param0;
                //send to opponent 
                agent.message.type = MESSAGE_ACC;
                agent.message.param0 = agent.secret;
                //initialize home field and opponent field
                FieldInit(&agent.home, &agent.opponent);
                FieldAIPlaceAllBoats(&agent.home);
                //set agent to challenge accepting
                agent.state = AGENT_STATE_ACCEPTING;
            } else {
                agent.message.type = MESSAGE_NONE;
            }
            break;
            //acceptor received event
        case BB_EVENT_ACC_RECEIVED:
            if (agent.state == AGENT_STATE_CHALLENGING) {
                //send REV message
                agent.message.type = MESSAGE_REV;
                agent.message.param0 = agent.secret;
                //set up coin flip to decide starting order
                NegotiationOutcome headsTails;
                headsTails = NegotiateCoinFlip(agent.secret, event.param0);
                //based on coinflip outcome set player to attacking or defending
                if (headsTails == HEADS) {
                    playerTurn = FIELD_OLED_TURN_MINE;
                    agent.state = AGENT_STATE_WAITING_TO_SEND;
                } else {
                    playerTurn = FIELD_OLED_TURN_THEIRS;
                    agent.state = AGENT_STATE_DEFENDING;
                }

            } else {
                agent.message.type = MESSAGE_NONE;
            }
            break;
            // REV received event
        case BB_EVENT_REV_RECEIVED:
            if (agent.state == AGENT_STATE_ACCEPTING) {
                NegotiationOutcome headsTails;
                headsTails = NegotiateCoinFlip(agent.secret, event.param0);
                //check for cheating
                if (NegotiationVerify(event.param0, agent.hash) == FALSE) {
                    //if cheating detected prompt restart and end game
                    char *cheatingDetected = "Cheating detected :-( Reset game to start again\n";
                    OledDrawString(cheatingDetected);
                    OledUpdate();
                    agent.state = AGENT_STATE_END_SCREEN;
                    agent.message.type = MESSAGE_NONE;
                    return agent.message;
                }
                //if no cheating detected initialize game based on coin flip result.
                if (headsTails == TAILS) {
                    playerTurn = FIELD_OLED_TURN_MINE;
                    GuessData guess = FieldAIDecideGuess(&agent.opponent);
                    agent.message.type = MESSAGE_SHO;
                    agent.message.param0 = guess.row;
                    agent.message.param1 = guess.col;
                    agent.state = AGENT_STATE_ATTACKING;
                } else {
                    agent.message.type = MESSAGE_NONE;
                    playerTurn = FIELD_OLED_TURN_THEIRS;
                    agent.state = AGENT_STATE_DEFENDING;
                }


            } else {
                agent.message.type = MESSAGE_NONE;
            }
            break;
            //Shot received event 
        case BB_EVENT_SHO_RECEIVED:
            if (agent.state == AGENT_STATE_DEFENDING) {
                //registers opponents shot on home team board
                GuessData opponentShot;
                opponentShot.row = event.param0;
                opponentShot.col = event.param1;
                FieldRegisterEnemyAttack(&agent.home, &opponentShot);
                agent.message.type = MESSAGE_RES;
                agent.message.param0 = event.param0;
                agent.message.param1 = event.param1;
                agent.message.param2 = opponentShot.result;

                //Checks if all home boats sunk to end game
                if (FieldGetBoatStates(&agent.home) == 0b00000000) {
                    //if all boats sunk displays loss message letting player know they have lost
                    agent.message.type = MESSAGE_NONE;
                    char *lossMessage = "You have lost all your ships have been sunk!";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(lossMessage);
                    OledUpdate();
                    //sets agent state to End Screen
                    agent.state = AGENT_STATE_END_SCREEN;
                    return agent.message;
                }                    //change turns if game does not end
                else {
                    playerTurn = FIELD_OLED_TURN_MINE;
                    agent.state = AGENT_STATE_WAITING_TO_SEND;
                }

            } else {
                agent.message.type = MESSAGE_NONE;
            }
            break;
        case BB_EVENT_RES_RECEIVED:
            if (agent.state == AGENT_STATE_ATTACKING) {
                //Register your own shot aka home team on opponent board
                GuessData homeShot;
                homeShot.row = event.param0;
                homeShot.col = event.param1;
                homeShot.result = event.param2;
                FieldUpdateKnowledge(&agent.opponent, &homeShot);
                //If all opponent boats sunk display victory message
                if (FieldGetBoatStates(&agent.opponent) == 0b00000000) {
                    agent.message.type = MESSAGE_NONE;
                    char *winMessage = "You have won all your opponent ships have been sunk!\n";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(winMessage);
                    OledUpdate();
                    //set agent state to end screen State
                    agent.state = AGENT_STATE_END_SCREEN;
                    return agent.message;

                }                    //change turns if game does not end
                else {
                    agent.message.type = MESSAGE_NONE;
                    playerTurn = FIELD_OLED_TURN_THEIRS;
                    agent.state = AGENT_STATE_DEFENDING;
                }
            } else {
                agent.message.type = MESSAGE_NONE;
            }
            break;
            //case for sending message to opponent 
        case BB_EVENT_MESSAGE_SENT:
            if (agent.state == AGENT_STATE_WAITING_TO_SEND) {
                // increment turns counter
                turns++;
                //sends shot data to opponent 
                GuessData shot;
                shot = FieldAIDecideGuess(&agent.opponent);
                agent.message.type = MESSAGE_SHO;
                agent.message.param0 = shot.row;
                agent.message.param1 = shot.col;
                agent.state = AGENT_STATE_ATTACKING;

            } else {
                agent.message.type = MESSAGE_NONE;
            }
            break;
            //event error case
        case BB_EVENT_ERROR:
            //switch statement for different types of errors
            switch (event.param0) {
                    //error is displayed on screen
                case BB_ERROR_BAD_CHECKSUM:
                    error = "Checksum error";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(error);
                    OledUpdate();
                    break;
                    //error is displayed on screen    
                case BB_ERROR_PAYLOAD_LEN_EXCEEDED:
                    error = "Payload error";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(error);
                    OledUpdate();
                    break;
                    //error is displayed on screen     
                case BB_ERROR_CHECKSUM_LEN_EXCEEDED:
                    error = "Checksum length exceeded error";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(error);
                    OledUpdate();
                    break;
                    //error is displayed on screen    
                case BB_ERROR_CHECKSUM_LEN_INSUFFICIENT:
                    error = "Checksum length insufficient error";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(error);
                    OledUpdate();
                    break;
                    //error is displayed on screen
                case BB_ERROR_INVALID_MESSAGE_TYPE:
                    error = "Invalid message type error";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(error);
                    OledUpdate();
                    break;
                    //error is displayed on screen
                case BB_ERROR_MESSAGE_PARSE_FAILURE:
                    error = "Invalid message parse failure";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(error);
                    OledUpdate();
                    break;

                default:
                    error = "Error";
                    OledClear(OLED_COLOR_BLACK);
                    OledDrawString(error);
                    OledUpdate();
                    break;
            }

            agent.state = AGENT_STATE_END_SCREEN;
            agent.message.type = MESSAGE_ERROR;
            return agent.message;
            break;
        case BB_EVENT_SOUTH_BUTTON:
            agent.message.type = MESSAGE_NONE;
            break;
        case BB_EVENT_EAST_BUTTON:
            agent.message.type = MESSAGE_NONE;
            break;






    }
    //after going through the switch statement the game screen is updated with the current information
    OledClear(OLED_COLOR_BLACK);
    FieldOledDrawScreen(&agent.home, &agent.opponent, playerTurn, turns);
    OledUpdate();
    return agent.message;
}

AgentState AgentGetState(void) {
    return agent.state;
}

void AgentSetState(AgentState newState) {
    agent.state = newState;
}

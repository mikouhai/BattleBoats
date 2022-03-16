/*
 * Created on March 8, 2022, 8:41 PM
 */

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
/*
 * 
 */
int main(void) {
    AgentInit();
    if (AgentGetState() == AGENT_STATE_START) {
        printf("Agent passed\n");
    }
    
    BB_Event agentRun;
    agentRun.type = BB_EVENT_START_BUTTON;
    agentRun.param0 = 0;
    agentRun.param1 = 0;
    agentRun.param2 = 0;
    AgentRun(agentRun);
    if (AgentGetState() == AGENT_STATE_CHALLENGING) {
        printf("passed test");
    }
    return (0);
}


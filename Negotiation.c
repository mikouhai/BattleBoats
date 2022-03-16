
#include <stdio.h>
#include <stdlib.h>
#include "BOARD.h"
#include "Negotiation.h"

NegotiationData NegotiationHash(NegotiationData secret) {
    NegotiationData hash;
    uint64_t overflow;
    overflow = ((uint64_t) secret * (uint64_t) secret) % PUBLIC_KEY;
    hash = overflow;
    return hash;
}

int NegotiationVerify(NegotiationData secret, NegotiationData commitment) {
    if (commitment == NegotiationHash(secret)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

NegotiationOutcome NegotiateCoinFlip(NegotiationData A, NegotiationData B) {
    int i;
    int count = 1;
    int parity = 0;

    for (i = 0; i < 32; i++) {
        if ((A ^ B) & (parity << i)) {
            count++;
        }

    }
    if ((count % 2)) {
        return TRUE;
    } else {
        return FALSE;
    }

}
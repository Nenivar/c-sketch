#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*
 *  STRUCTURES
 */

enum OPCODE {
    DX, DY, DT, PEN
};
typedef enum OPCODE OPCODE;

/*
 *  DECODING
 */

// returns the opcode for a given instruction
OPCODE extractOpcode (unsigned char instruction) {
    char opcode = (instruction >> 0x6) & 0x03;

    return opcode;
}

int extractOperand (unsigned char instruction) {
    int operand = instruction & 0x3F;

    return operand;
}

/*
 *  INPUT
 */

/*
 *  OUTPUT
 */

/*
 *  TESTING
 */

void testExtCode () {
    assert (extractOpcode (0x37) == 0);
    assert (extractOpcode (0x03) == 0);
    assert (extractOpcode (0x67) == 1);
    assert (extractOpcode (0x43) == 1);
    assert (extractOpcode (0xAC) == 2);
    assert (extractOpcode (0x91) == 2);
    assert (extractOpcode (0xDE) == 3);
    assert (extractOpcode (0xC6) == 3);
}

void testExtAnd () {

}

void test () {
    testExtCode ();
    testExtAnd ();
}

/*
 *  MAIN
 */

int main (int n, char *varg[n]) {
    test ();

    return 1;
}
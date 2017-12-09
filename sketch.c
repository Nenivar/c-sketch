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
    return (instruction >> 0x6) & 0x03;;
}

// returns the operand (betwen -32 & 31) for a given instruction
char extractOperand (unsigned char instruction) {
    char operand = instruction & 0x1F;
    if ((instruction & 0x20) == 0x20) operand -= 32;

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
    assert (extractOperand (0x00) == 0);
    assert (extractOperand (0x37) == -9);
    assert (extractOperand (0xFF) == -1);
    assert (extractOperand (0x20) == -32);
    assert (extractOperand (0x1F) == 31);
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
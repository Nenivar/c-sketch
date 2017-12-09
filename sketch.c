#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "display.h"

/*
 *  STRUCTURES
 */

const int MAX_INSTRUCTIONS = 100;

enum OPCODE {
    DX, DY, DT, PEN
};
typedef enum OPCODE OPCODE;

struct instructionSet {
    int n;
    unsigned char instructions [];
};
typedef struct instructionSet instructionSet;

struct state {
    bool penDown;
    int x, y, prevX, prevY;
    display *disp;
};
typedef struct state state;

/*
 *  LOGIC
 */

// returns an initalized new state for drawing
state *newState (display *disp) {
    state *new = malloc (sizeof (state));
    new->penDown = false;
    new->x = 0;
    new->y = 0;
    new->disp = disp;

    return new;
}

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

// reads in a sketch file and returns a set of instructions
instructionSet *readFile (char *loc) {
    instructionSet *set = malloc (sizeof (instructionSet));

    FILE *in = fopen (loc, "rb");
    unsigned char b = fgetc (in);
    int i = 0;
    while (!feof (in)) {
        set->instructions [i] = b;
        b = fgetc (in);
        i++;
    }
    fclose (in);

    set->n = i;

    return set;
}

/*
 *  OUTPUT
 */

display *setupDisplay () {
    display *d = newDisplay ("line.sketch", 200, 200);
    //clear (d);

    return d;
}

void interpretInstr (state *s, unsigned char instruction) {
    OPCODE opcode = extractOpcode (instruction);
    char operand = extractOperand (instruction);

    switch (opcode) {
        case DX:
            s->prevX = s->x;
            s->x += operand;

            if (s->penDown) {
                //if (validPrev)
                
            if (s->penDown && operand != 0) line (s->disp, s->prevX, s->prevY,
                                s->x, s->y);
            }

            printf ("Moved pen to x=%d; (%d, %d) -> (%d, %d)\n", s->x, s->prevX, s->prevY, s->x, s->y);

            break;
        case DY:
            s->prevY = s->y;
            s->y += operand;

            if (s->penDown && operand != 0) line (s->disp, s->prevX, s->prevY,
                                s->x, s->y);

            printf ("Moved pen to y=%d; (%d, %d) -> (%d, %d)\n", s->y, s->prevX, s->prevY, s->x, s->y);
            
            break;
        case DT:

            break;
        case PEN:
            
            s->penDown = !s->penDown;
            if (s->penDown) {
s->prevX = s->x;
            s->prevY = s->y;
            }
            
            printf ("Changed pen status to x=%d\n", s->penDown);
            break;
        default:
            printf ("Invalid instruction!\n");
            break;
    }
}

void interpretInstrSet (state *s, instructionSet *set) {
    for (int i = 0; i < set->n; i++){
        interpretInstr (s, set->instructions [i]);
        //pause (s->disp, 1000);
    }
    
    free (set);
}

/*
 *  TESTING
 */

void testExtCode () {
    assert (extractOpcode (0x37) == DX);
    assert (extractOpcode (0x03) == DX);
    assert (extractOpcode (0x67) == DY);
    assert (extractOpcode (0x43) == DY);
    assert (extractOpcode (0xAC) == DT);
    assert (extractOpcode (0x91) == DT);
    assert (extractOpcode (0xDE) == PEN);
    assert (extractOpcode (0xC6) == PEN);
}

void testExtAnd () {
    assert (extractOperand (0x00) == 0);
    assert (extractOperand (0x37) == -9);
    assert (extractOperand (0xFF) == -1);
    assert (extractOperand (0x20) == -32);
    assert (extractOperand (0x1F) == 31);
}

void testReadFile () {
    instructionSet *set = readFile ("line.sketch");
    assert (set->instructions [0] == 0x1e);
    assert (set->instructions [1] == 0x5e);
    assert (set->instructions [2] == 0xc3);
    assert (set->instructions [3] == 0x1e);
    assert (set->instructions [4] == 0x40);
    free (set);
}

void test () {
    testExtCode ();
    testExtAnd ();
    testReadFile ();
}

/*
 *  MAIN
 */

int main (int n, char *varg[n]) {
    test ();

    display *d = setupDisplay ();
    state *s = newState (d);

    // oxo, diag, cross
    instructionSet *set = readFile ("line.sketch");
    interpretInstrSet (s, set);

    //key (d);

    free (d);
    free (s);

    return 1;
}
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "display.h"

/*
 *  STRUCTURES
 */

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
    new->prevX = 0;
    new->prevY = 0;
    new->x = 0;
    new->y = 0;
    new->disp = disp;

    return new;
}

// returns the opcode for a given instruction
OPCODE extractOpcode (unsigned char instruction) {
    return (instruction >> 0x6) & 0x03;
}

// returns the operand (between -32 & 31) for a given instruction
char extractOperand (unsigned char instruction) {
    return instruction & 0x3F;
}

// returns the operand (between 0 & 63) for a dt instruction
char extractMoveOperand (unsigned char instruction) {
    return (instruction & 0x20) == 0x20 ? (instruction & 0x1F) - 32
                                     : (instruction & 0x1F);
}

/*
 *  INPUT
 */

// reads in a sketch file and returns a set of instructions
instructionSet *readFile (char *loc) {
    int i = 0, tempSize = 10;
    instructionSet *set = malloc (sizeof (instructionSet) + tempSize);

    FILE *in = fopen (loc, "rb");
    unsigned char b = fgetc (in);
    while (!feof (in)) {
        if (i > tempSize) {
            tempSize *= 1.5;
            set = realloc (set, sizeof (instructionSet) + tempSize);
        }
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

// creates a new display
display *setupDisplay (char *name) {
    display *d = newDisplay (name, 200, 200);

    return d;
}

void executeDx (state *s, char instruction) {
    char dx = extractMoveOperand (instruction);
    s->x += dx;
}

void executeDy (state *s, unsigned char instruction) {
    char dy = extractMoveOperand (instruction);
    s->y += dy;

    if (s->penDown) line (s->disp, s->prevX, s->prevY, s->x, s->y);

    s->prevX = s->x;
    s->prevY = s->y;
}

void executeDt (state *s, char instruction) {
    pause (s->disp, extractOperand (instruction) * 10);
}

void executePen (state *s) {
    s->penDown = !s->penDown;
}

void interpretInstr (state *s, unsigned char instruction) {
    OPCODE opcode = extractOpcode (instruction);

    switch (opcode) {
        case DX:
            //printf ("Moved pen to x=%d; (%d, %d) -> (%d, %d)\n", s->x, s->prevX, s->prevY, s->x, s->y);
            executeDx (s, instruction);
            break;
        case DY:
            //printf ("Moved pen to y=%d; (%d, %d) -> (%d, %d)\n", s->y, s->prevX, s->prevY, s->x, s->y);
            executeDy (s, instruction);
            break;
        case DT:
            executeDt (s, instruction);
            break;
        case PEN:
            //printf ("Changed pen status to x=%d\n", s->penDown);
            executePen (s);
            break;
        default:
            printf ("Invalid instruction!\n");
            break;
    }
}

void interpretInstrSet (state *s, instructionSet *set) {
    for (int i = 0; i < set->n; i++){
        interpretInstr (s, set->instructions [i]);
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
    assert (extractOperand (0x37) == 55);
    assert (extractOperand (0xFF) == 63);
    assert (extractOperand (0x1F) == 31);

    assert (extractMoveOperand (0x00) == 0);
    assert (extractMoveOperand (0x37) == -9);
    assert (extractMoveOperand (0xFF) == -1);
    assert (extractMoveOperand (0x20) == -32);
    assert (extractMoveOperand (0x1F) == 31);
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

    if (n > 1) {
        char *file = varg [1];

        display *d = setupDisplay (file);
        state *s = newState (d);
        
        instructionSet *set = readFile (file);
        interpretInstrSet (s, set);

        free (s);
        free (d);
    } else {

    }

    return 1;
}
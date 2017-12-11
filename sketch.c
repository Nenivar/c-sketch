#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "display.h"

/*
 *  STRUCTURES
 */
// drawing operations
enum OPCODE {
    DX, DY, DT, PEN
};
typedef enum OPCODE OPCODE;

// the set of instructions to execute for a drawing
struct instructionSet {
    int n;
    unsigned char instructions [];
};
typedef struct instructionSet instructionSet;

// the current state of a drawing
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
    return (instruction >> 0x6);
}

// returns an unsigned operand (between 0 & 63) for a given instruction
char extractUnsignedOperand (unsigned char instruction) {
    return instruction & 0x3F;
}

// returns a signed operand (between -32 & 31) for a given instruction
char extractSignedOperand (unsigned char instruction) {
    return (instruction & 0x20) == 0x20 ? (instruction & 0x1F) - 32
                                     : (instruction & 0x1F);
}

/*
 *  INPUT
 */

// reads in a sketch file and returns a set of instructions
// includes automatic resizing
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

// initalizes a new display
display *setupDisplay (char *name) {
    display *d = newDisplay (name, 200, 200);

    return d;
}

// method for dx command
void executeDx (state *s, char instruction) {
    char dx = extractSignedOperand (instruction);
    s->x += dx;
}

// method for dy command
void executeDy (state *s, unsigned char instruction) {
    char dy = extractSignedOperand (instruction);
    s->y += dy;

    if (s->penDown) line (s->disp, s->prevX, s->prevY, s->x, s->y);

    s->prevX = s->x;
    s->prevY = s->y;
}

// method for dt command
void executeDt (state *s, char instruction) {
    pause (s->disp, extractUnsignedOperand (instruction) * 10);
}

// method for pen command
void executePen (state *s) {
    s->penDown = !s->penDown;
}

// decides what method to execute based on the given instruction
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

// executes a set of instructions
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
    assert (extractUnsignedOperand (0x00) == 0);
    assert (extractUnsignedOperand (0x37) == 55);
    assert (extractUnsignedOperand (0xFF) == 63);
    assert (extractUnsignedOperand (0x1F) == 31);

    assert (extractSignedOperand (0x00) == 0);
    assert (extractSignedOperand (0x37) == -9);
    assert (extractSignedOperand (0xFF) == -1);
    assert (extractSignedOperand (0x20) == -32);
    assert (extractSignedOperand (0x1F) == 31);
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
    }

    return 1;
}
/*
 *  SKETCH
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "display.h"

/*
 *  STRUCTURES
 */

const int WINDOW_DIM = 200;

// drawing operations
enum OPCODE {
    DX, DY, DT, PEN, CLEAR, KEY, COL
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
// first two bits -> opcode
// if opcode == 3 -> last 4 bits
OPCODE extrOpcode (unsigned char instruction) {
    OPCODE opcode = (instruction >> 0x6) & 0x3;
    return opcode == 3 ? instruction & 0x0F : opcode;
}

// returns an unsigned operand (between 0 & 63)
// for a given *compact* instruction
// last 8 bits = operand
char extrUnOperand (unsigned char instruction) {
    return instruction & 0x3F;  
}

// (conditionally) converts an unsigned operand to signed
// if it is applicable i.e. checks if the most sig. bit == 1
// subtracts 2x itself if so
long convToSigned (long operand, int bits) {
    int c = 1 << (bits - 1);
    return (operand & c) == c ? operand - (c * 2) : operand;
}

// packs multiple bytes into one long
// up to 4 bytes
// n = no. bytes; offset = byte pos. to start from
long packBytes (unsigned char *bytes, int n, int offset) {
    long out = 0;
    for (int i = offset; i < n; i++) out += (bytes [i] << ((n - i - 1) * 8));
    
    return out;
}

// returns whether an instruction is using extended mode
// (if the opcode (compact ver.) == 3)
bool usesExtMode (unsigned char instruction) {
    return ((instruction >> 0x6) & 0x3) == 3;
}

// returns the number of extra operand bytes
// for if the opcode == 3
// last 2 bits of first byte -> no.
char extrExtLen (unsigned char instruction) {
    char length = (instruction >> 0x4) & 0x3;
    return length == 3 ? 4 : length;
}

/*
 *  INPUT
 */

// reads in a sketch file and returns a set of instructions
// includes automatic instruction setresizing
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
    display *d = newDisplay (name, WINDOW_DIM, WINDOW_DIM);

    return d;
}

// adds dx to x pos. of pen
void executeDx (state *s, long dx) {
    s->x += dx;
}

// adds dy to y pos. of pen
// also updates state & draws if needed
void executeDy (state *s, long dy) {
    s->y += dy;

    if (s->penDown) line (s->disp, s->prevX, s->prevY, s->x, s->y);

    s->prevX = s->x;
    s->prevY = s->y;
}

// pauses screen by dt ms
void executeDt (state *s, long dt) {
    pause (s->disp, dt);
}

// changes pen from up->down & down->up
void executePen (state *s) {
    s->penDown = !s->penDown;
}

// clears the display
void executeClear (state *s) {
    clear (s->disp);
}

// waits for a key input from the user
void executeKey (state *s) {
    key (s->disp);
}

// changes the colour of the pen
void executeCol (state *s, int c) {
    colour (s->disp, c);
}

// returns the operand based on the extended
// & dy/dx or not
long getOperand (OPCODE opcode, unsigned char *bytes, int n) {
    long operand = 0;
    if (n == 1) {
        operand = extrUnOperand (bytes [0]);
        if (opcode == DX || opcode == DY)
            operand = convToSigned (operand, 6);
    } else if (n > 1) {
        operand = packBytes (bytes, n, 1);
        operand = convToSigned (operand, 8 * (n - 1));        
    }
    return operand;
}

// executes a singular instruction (w/ support for multiple operands)
// n = no. bytes; (1 == not extended); 0 = ext, but no operand
void interpretBytes (state *s, unsigned char *bytes, int n) {
    unsigned char instruction = bytes [0];
    OPCODE opcode = extrOpcode (instruction);
    long operand = getOperand (opcode, bytes, n);

    switch (opcode) {
        case DX:
            executeDx (s, operand); break;
        case DY:
            executeDy (s, operand); break;
        case DT:
            executeDt (s, operand * 10); break;
        case PEN:
            executePen (s); break;
        case CLEAR:
            executeClear (s); break;
        case KEY:
            executeKey (s); break;
        case COL:
            executeCol (s, operand); break;
        default:
            printf ("Invalid instruction!\n"); break;
    }
}

// executes a set of instructions
// takes each instruction and gets any extra operands
// needed before executing it
void interpretInstrSet (state *s, instructionSet *set) {
    unsigned char bytes [5];

    for (int i = 0; i < set->n; i++){
        unsigned char instruction = set->instructions [i];
        char len = extrExtLen (instruction);
        char noBytes = usesExtMode (instruction)
                    ? (len == 0 ? 0 : len + 1) : 1;

        bytes [0] = instruction;

        if (noBytes > 1) {
            for (int j = 1; j < noBytes && i + j < set->n; j++)
                        bytes [j] = set->instructions [i + j];
            i = i + noBytes - 1;
        }

        interpretBytes (s, bytes, noBytes);
    }
    
    free (set);
}

/*
 *  TESTING
 */

void testExtCode () {
    assert (extrOpcode (0x37) == DX);
    assert (extrOpcode (0x03) == DX);
    assert (extrOpcode (0x67) == DY);
    assert (extrOpcode (0x43) == DY);
    assert (extrOpcode (0xAC) == DT);
    assert (extrOpcode (0x91) == DT);
    assert (extrOpcode (0xC3) == PEN);
    assert (extrOpcode (0xE3) == PEN);

    assert (extrOpcode (0xD0) == DX);
    assert (extrOpcode (0xF1) == DY);
    assert (extrOpcode (0xE2) == DT);
    assert (extrOpcode (0xF4) == CLEAR);
    assert (extrOpcode (0xD5) == KEY);
    assert (extrOpcode (0xE6) == COL);
}

void testExtAnd () {
    assert (extrUnOperand (0x00) == 0);
    assert (extrUnOperand (0x37) == 55);
    assert (extrUnOperand (0xFF) == 63);
    assert (extrUnOperand (0x1F) == 31);

    assert (convToSigned (extrUnOperand (0x00), 6) == 0);
    assert (convToSigned (extrUnOperand (0x37), 6) == -9);
    assert (convToSigned (extrUnOperand (0xFF), 6) == -1);
    assert (convToSigned (extrUnOperand (0x20), 6) == -32);
    assert (convToSigned (extrUnOperand (0x1F), 6) == 31);
}

void testExtLen () {
    assert (extrExtLen (0xC7) == 0);
    assert (extrExtLen (0xD2) == 1);
    assert (extrExtLen (0xA3) == 2);
    assert (extrExtLen (0x71) == 4);
}

void testPack () {
    unsigned char ex1 [] = {0x4C, 0xFF, 0x58};
    assert (packBytes (ex1, 3, 0) == 0x4CFF58);
    unsigned char ex2 [] = {0xFF, 0xFF, 0xFF};
    assert (packBytes (ex2, 3, 0) == 0xFFFFFF);
    unsigned char ex3 [] = {0x3};
    assert (packBytes (ex3, 1, 0) == 0x3);
    unsigned char ex4 [] = {0x3F, 0x41, 0xE7};
    assert (packBytes (ex4, 3, 1) == 0x41E7);
    unsigned char ex5 [] = {0xCE, 0x60};
    assert (packBytes (ex5, 2, 0) == 0xCE60);
    assert (convToSigned (
            packBytes (ex5, 2, 0), 16) == -12704);
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
    testExtLen ();
    testPack ();
    testReadFile ();
}

/*
 *  MAIN
 */

// opens a sketch file and executes it
void testFile (char *filename) {
    display *d = setupDisplay (filename);
    state *s = newState (d);
        
    instructionSet *set = readFile (filename);
    interpretInstrSet (s, set);

    free (s);
    free (d);
}

// >1 arg = test first arg. (loc of sketch file)
// 0 = test all sketch files
int main (int n, char *varg[n]) {
    test ();

    if (n > 1) {
        char *file = varg [1];

        testFile (file);
    } else if (n == 1) {
        testFile ("line.sketch");
        testFile ("square.sketch");
        testFile ("box.sketch");
        testFile ("oxo.sketch");
        testFile ("diag.sketch");
        testFile ("cross.sketch");

        testFile ("clear.sketch");
        testFile ("key.sketch");
        testFile ("lawn.sketch");
        testFile ("field.sketch");
        testFile ("pauses.sketch");
    }

    return 1;
}
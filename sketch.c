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
// if opcode == 3 = last 4 bits
OPCODE extractOpcode (unsigned char instruction) {
    OPCODE opcode = (instruction >> 0x6) & 0x3;
    return opcode == 3 ? instruction & 0x0F : opcode;
}

// returns the number of extra operand bytes
// for if the opcode == 3
// last 2 bits of first byte -> length
char extractExtraLength (unsigned char instruction) {
    char length = (instruction >> 0x4) & 0x3;
    return length == 3 ? 4 : length;
}

// returns an unsigned operand (between 0 & 63) for a given instruction
// last 8 bits = operand
char extractUnsignedOperand (unsigned char instruction) {
    return instruction & 0x3F;  
}

// returns a signed operand (between -32 & 31) for a given instruction
char extractSignedOperand (unsigned char instruction) {
    return (instruction & 0x20) == 0x20 ? (instruction & 0x1F) - 32
                                     : (instruction & 0x1F);
}

bool usesExtendedMode (unsigned char instruction) {
    return ((instruction >> 0x6) & 0x3) == 3;
}

// TODO: if offset > n?
// 0x80 << whatever
long packBytes (unsigned char *bytes, int n, int offset) {
    long out = 0;
    //for (int i = offset; i < n; i++) out += ((bytes [i] << ((i - offset) * 8)));
    for (int i = offset; i < n; i++) out += (bytes [i] << ((n - i - 1) * 8));
    
    //if (out & ((n - offset) * 8) == 1) out -= ((n - offset) * 8);
    return out;
}

// or long?
int condConvertToSignedOperand (int operand, int bits) {
    int c = 1 << (bits - 1);
    return (operand & c) == c ? operand - (c * 2) : operand;
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
void executeDx (state *s, long dx) {
    s->x += dx;
}

// method for dy command
void executeDy (state *s, long dy) {
    s->y += dy;

    if (s->penDown) line (s->disp, s->prevX, s->prevY, s->x, s->y);

    s->prevX = s->x;
    s->prevY = s->y;
}

// method for dt command
void executeDt (state *s, long dt) {
    pause (s->disp, dt);
}

// method for pen command
void executePen (state *s) {
    s->penDown = !s->penDown;
}

// method for clear command
void executeClear (state *s) {
    clear (s->disp);
}

// method for key command
void executeKey (state *s) {
    key (s->disp);
}

// method for col command
void executeCol (state *s, int c) {
    colour (s->disp, c);
}

long getOperand (OPCODE opcode, unsigned char *bytes, int n) {
    long operand = 0;
    if (n == 1) {
        operand = extractUnsignedOperand (bytes [0]);
        if (opcode == DX || opcode == DY)
            operand = condConvertToSignedOperand (operand, 6);
    } else if (n > 1) {
        operand = packBytes (bytes, n, 1);
        operand = condConvertToSignedOperand (operand, 8 * (n - 1));        
    }
    return operand;
}

// n = no. bytes
// (1 == not extended)
// 0 = ext, but no operand
void interpretBytes (state *s, unsigned char *bytes, int n) {
    unsigned char instruction = bytes [0];
    OPCODE opcode = extractOpcode (instruction);
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
            printf ("Invalid instruction!\n");
            break;
    }
}

// executes a set of instructions
void interpretInstrSet (state *s, instructionSet *set) {
    unsigned char bytes [5];

    for (int i = 0; i < set->n; i++){
        unsigned char instruction = set->instructions [i];
        char len = extractExtraLength (instruction);
        char noBytes = usesExtendedMode (instruction)
                    ? (len == 0 ? 0 : len + 1) : 1;
                    //? extractExtraLength (instruction) : 1;

        bytes [0] = instruction;

        if (noBytes > 1) {
            for (int j = 1; j < noBytes && i + j < set->n; j++) bytes [j] = set->instructions [i + j];
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
    assert (extractOpcode (0x37) == DX);
    assert (extractOpcode (0x03) == DX);
    assert (extractOpcode (0x67) == DY);
    assert (extractOpcode (0x43) == DY);
    assert (extractOpcode (0xAC) == DT);
    assert (extractOpcode (0x91) == DT);
    assert (extractOpcode (0xC3) == PEN);
    assert (extractOpcode (0xE3) == PEN);

    assert (extractOpcode (0xD0) == DX);
    assert (extractOpcode (0xF1) == DY);
    assert (extractOpcode (0xE2) == DT);
    assert (extractOpcode (0xF4) == CLEAR);
    assert (extractOpcode (0xD5) == KEY);
    assert (extractOpcode (0xE6) == COL);
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

void testExtLen () {
    assert (extractExtraLength (0xC7) == 0);
    assert (extractExtraLength (0xD2) == 1);
    assert (extractExtraLength (0xA3) == 2);
    assert (extractExtraLength (0x71) == 4);
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
    assert (condConvertToSignedOperand (
            packBytes (ex5, 2, 0), 16) == -12704);
}

void test () {
    testExtCode ();
    testExtAnd ();
    testExtLen ();
    testReadFile ();
    testPack ();
}

/*
 *  MAIN
 */

void testFile (char *filename) {
    printf ("-------------\n%s\n-------------\n", filename);
    display *d = setupDisplay (filename);
    state *s = newState (d);
        
    instructionSet *set = readFile (filename);
    interpretInstrSet (s, set);

    free (s);
    free (d);
}

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
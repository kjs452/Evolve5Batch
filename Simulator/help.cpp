/*
 * Help me!
 *
 * This file produced from help.tmpl and help.tt
 * (processed by render_code.py)
 *
 * This file contains static help strings which were generated
 * from tagged text documentation.
 *
 * kforth instruction documentation is the only help data so far.
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

//
// This dialog is used in several contexts
// The core KFORTH instructions will always be displayed in
// this list (all masks use MASK_K).
//
// MASK_XXXX flags are or'd together and associated
// with each entry in the KFI table.
//
// The contexts are:
//	Showing instructions in view organism dialog
//	showing instructions for the kforth interpreter dialog
//	showing valid instruction for Find expressions (MASK_F)
//
//

#define MASK_K			1	/* kforth core instructions */
#define MASK_C			2	/* cell instruction */
#define MASK_F			4	/* find instructions */

#define MASK_CORE		8
#define MASK_INTERACT	16
#define MASK_VISION		32
#define MASK_COMMS		64
#define MASK_QUERY		128
#define MASK_UNIVERSE	256
#define MASK_MISC		512
#define MASK_FIND		1024

static KFORTH_IHELP Kforth_Instruction_Help_Table[] =
{
	MASK_CORE | MASK_K,
	"call",
	"call",
	"( code-block -- )",
	"Subroutine call to another code block. The "
	"code block number is given by 'code-block' on top "
	"of the data stack. "
	"If code block is invalid, the request is ignored. "
	"In disassembly row123 is code-block 123, and so on. ",


	MASK_CORE | MASK_K,
	"if",
	"if",
	"( expr code-block -- )",
	"remove two items from top of stack. If expr is "
	"non-zero then call code-block 'cb'. Code block numbers are relative "
	"to the current code block being executed. ",


	MASK_CORE | MASK_K,
	"ifelse",
	"ifelse",
	"( expr true-block false-block -- )",
	"remove three items from top of stack. If expr is "
	"non-zero then call 'cb1', else call 'cb2'. Code block number "
	"are relative to the current code block being executed. ",


	MASK_CORE | MASK_K,
	"?loop",
	"loop_if",
	"( n -- )",
	"Remove one item from the stack. If value is non-zero "
	"jump to the start of the current code block. Otherwise "
	"continue with the next instruction after '?loop'. ",


	MASK_CORE | MASK_K,
	"?exit",
	"exit_if",
	"( n -- )",
	"Remove one item from the stack. If non-zero then "
	"exit current code block. ",


	MASK_CORE | MASK_K,
	"pop",
	"pop",
	"( n -- )",
	"Remove item from stack and discard it. ",


	MASK_CORE | MASK_K,
	"dup",
	"dup",
	"( a -- a a )",
	"Duplicate item on top of the stack. ",


	MASK_CORE | MASK_K,
	"swap",
	"swap",
	"( a b -- b a )",
	"Swap top two elements on the stack. ",


	MASK_CORE | MASK_K,
	"over",
	"over",
	"( a b -- a b a )",
	"Copy second item from the stack. ",


	MASK_CORE | MASK_K,
	"rot",
	"rot",
	"( a b c -- b c a )",
	"Rotate third item to top. ",


	MASK_CORE | MASK_K,
	"?dup",
	"dup_if",
	"( n -- n n | 0 )",
	"Duplicate top element if non-zero. ",


	MASK_CORE | MASK_K,
	"-rot",
	"reverse_rot",
	"( a b c -- c a b )",
	"Rotate top to third position. ",


	MASK_CORE | MASK_K,
	"2swap",
	"2swap",
	"( a b c d -- c d a b )",
	"Swap pairs. ",


	MASK_CORE | MASK_K,
	"2over",
	"2over",
	"( a b c d -- a b c d a b)",
	"Leapfrog pair. ",


	MASK_CORE | MASK_K,
	"2dup",
	"2dup",
	"( a b -- a b a b )",
	"Dupicate pair. ",


	MASK_CORE | MASK_K,
	"2pop",
	"2pop",
	"( a b -- )",
	"Remove pair. ",


	MASK_CORE | MASK_K,
	"nip",
	"nip",
	"( a b -- b )",
	"Remove 2nd item from stack. ",


	MASK_CORE | MASK_K,
	"tuck",
	"tuck",
	"( a b -- b a b)",
	"Copy top item to third position. ",


	MASK_CORE | MASK_K,
	"1+",
	"increment",
	"( n -- n+1 )",
	"Add 1 to the item on top of the stack. ",


	MASK_CORE | MASK_K,
	"1-",
	"decrement",
	"( n -- n-1 )",
	"Subtract 1 from item on top of the stack. ",


	MASK_CORE | MASK_K,
	"2+",
	"increment2",
	"( n -- n+2 )",
	"Add 2 to item on top of the stack ",


	MASK_CORE | MASK_K,
	"2-",
	"decrement2",
	"( n -- n-2 )",
	"Subtract 2 from the item on top of the stack. ",


	MASK_CORE | MASK_K,
	"2/",
	"half",
	"( n -- n/2 )",
	"Half value. ",


	MASK_CORE | MASK_K,
	"2*",
	"double",
	"( n -- n*2 )",
	"Double value. ",


	MASK_CORE | MASK_K,
	"abs",
	"abs",
	"( n -- abs(n) )",
	"Absolute value of n. ",


	MASK_CORE | MASK_K,
	"sqrt",
	"sqrt",
	"( n -- sqrt(n) )",
	"Square root. n must be positive. If n isn't positive then leave stack unchanged. ",


	MASK_CORE | MASK_K,
	"+",
	"plus",
	"( a b -- a+b )",
	"Add top two elements on stack. ",


	MASK_CORE | MASK_K,
	"-",
	"minus",
	"( a b -- a-b )",
	"Subtract first item on stack from the second item. ",


	MASK_CORE | MASK_K,
	"*",
	"multiply",
	"( a b -- a*b )",
	"Multiply top two elements on the data stack. ",


	MASK_CORE | MASK_K,
	"/",
	"divide",
	"( a b -- a/b )",
	"Divide. ",


	MASK_CORE | MASK_K,
	"mod",
	"modulos",
	"( a b -- a%b )",
	"Modulos. ",


	MASK_CORE | MASK_K,
	"/mod",
	"divmod",
	"( a b -- a%b a/b )",
	"Divide and modulos. ",


	MASK_CORE | MASK_K,
	"negate",
	"negate",
	"( n -- -n )",
	"Negate item on stack ",


	MASK_CORE | MASK_K,
	"2negate",
	"2negate",
	"( a b -- -a -b )",
	"negate top two items on stack. ",


	MASK_CORE | MASK_K,
	"<<",
	"lshift",
	"( a b -- a << b )",
	"Left shift a by b bits. "
	"Negative b will perform right shift. ",


	MASK_CORE | MASK_K,
	">>",
	"rshift",
	"( a b -- a >> b )",
	"Right shift a by b bits. "
	"Negative b will perform left shift. ",


	MASK_CORE | MASK_K,
	"=",
	"eq",
	"( a b -- EQ(a,b) )",
	"Equal to. 1 means 'a' and 'b' are the same, else 0. ",


	MASK_CORE | MASK_K,
	"<>",
	"ne",
	"( a b -- NE(a,b) )",
	"Not equal to. 1 means 'a' and 'b' are not equal, else 0. ",


	MASK_CORE | MASK_K,
	"<",
	"lt",
	"( a b -- LT(a,b) )",
	"Less than. 1 means 'a' is less than 'b', else 0. ",


	MASK_CORE | MASK_K,
	">",
	"gt",
	"( a b -- GT(a,b) )",
	"Greater than. 1 means 'a' is greater than 'b', else 0. ",


	MASK_CORE | MASK_K,
	"<=",
	"le",
	"( a b -- LE(a,b) )",
	"Less than or equal to. ",


	MASK_CORE | MASK_K,
	">=",
	"ge",
	"( a b -- GE(a,b) )",
	"Greater than or equal to. ",


	MASK_CORE | MASK_K,
	"0=",
	"equal_zero",
	"( n -- EQ(n,0) )",
	"Is element on top of the stack equal to 0? ",


	MASK_CORE | MASK_K,
	"or",
	"or",
	"( a b -- a|b )",
	"Bitwise OR. Can be used as a logical OR operator too, because "
	"KFORTH boolean operators return 1 and 0. ",


	MASK_CORE | MASK_K,
	"and",
	"and",
	"( a b -- a&b )",
	"Bitwise AND. Can be used a a logical AND operator too, because "
	"KFORTH boolean operators return 1 and 0. ",


	MASK_CORE | MASK_K,
	"not",
	"not",
	"( n -- !n )",
	"Logical NOT. Any non-zero value will be converted to 0. 0 will be converted to 1. ",


	MASK_CORE | MASK_K,
	"invert",
	"invert",
	"( n -- ~n )",
	"Invert bits (Bitwise NOT). ",


	MASK_CORE | MASK_K,
	"xor",
	"xor",
	"( a b -- a^b )",
	"bitwise XOR function. ",


	MASK_CORE | MASK_K,
	"min",
	"min",
	"( a b -- min(a,b) )",
	"Minimum value. ",


	MASK_CORE | MASK_K,
	"max",
	"max",
	"( a b -- max(a,b) )",
	"Remove two items from stack and replace with "
	"maximum value. ",


	MASK_CORE | MASK_K,
	"CB",
	"CB",
	"( -- CB )",
	"Pushes the current code block number on the data stack. ",


	MASK_CORE | MASK_K,
	"R0",
	"R0",
	"( -- R0 )",
	"Pushes register R0 on the data stack ",


	MASK_CORE | MASK_K,
	"R1",
	"R1",
	"( -- R1 )",
	"Pushes register R1 on the data stack ",


	MASK_CORE | MASK_K,
	"R2",
	"R2",
	"( -- R2 )",
	"Pushes register R2 on the data stack ",


	MASK_CORE | MASK_K,
	"R3",
	"R3",
	"( -- R3 )",
	"Pushes register R3 on the data stack ",


	MASK_CORE | MASK_K,
	"R4",
	"R4",
	"( -- R4 )",
	"Pushes register R4 on the data stack ",


	MASK_CORE | MASK_K,
	"R5",
	"R5",
	"( -- R5 )",
	"Pushes register R5 on the data stack ",


	MASK_CORE | MASK_K,
	"R6",
	"R6",
	"( -- R6 )",
	"Pushes register R6 on the data stack ",


	MASK_CORE | MASK_K,
	"R7",
	"R7",
	"( -- R7 )",
	"Pushes register R7 on the data stack ",


	MASK_CORE | MASK_K,
	"R8",
	"R8",
	"( -- R8 )",
	"Pushes register R8 on the data stack ",


	MASK_CORE | MASK_K,
	"R9",
	"R9",
	"( -- R9 )",
	"Pushes register R9 on the data stack ",


	MASK_CORE | MASK_K,
	"R0!",
	"set_r0",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R0 ",


	MASK_CORE | MASK_K,
	"R1!",
	"set_r1",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R1 ",


	MASK_CORE | MASK_K,
	"R2!",
	"set_r2",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R2 ",


	MASK_CORE | MASK_K,
	"R3!",
	"set_r3",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R3 ",


	MASK_CORE | MASK_K,
	"R4!",
	"set_r4",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R4 ",


	MASK_CORE | MASK_K,
	"R5!",
	"set_r5",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R5 ",


	MASK_CORE | MASK_K,
	"R6!",
	"set_r6",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R6 ",


	MASK_CORE | MASK_K,
	"R7!",
	"set_r7",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R7 ",


	MASK_CORE | MASK_K,
	"R8!",
	"set_r8",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R8 ",


	MASK_CORE | MASK_K,
	"R9!",
	"set_r9",
	"( val -- )",
	"Removes 1 item 'val' from the data stack "
	"and stores 'val' into register R9 ",


	MASK_CORE | MASK_K,
	"SIGN",
	"SIGN",
	"( n -- SIGN(n) )",
	"return sign of 'n'. If n is negative this instruction "
	"returns -1. If n is positive this instruction returns 1. "
	"If n is 0, returns 0. ",


	MASK_CORE | MASK_K,
	"PACK2",
	"PACK2",
	"( a b -- n )",
	"combines 2 8-bit integers into a single value 'n' ",


	MASK_CORE | MASK_K,
	"UNPACK2",
	"UNPACK2",
	"( n -- a b )",
	"does the opposite of pack2. ",


	MASK_CORE | MASK_K,
	"MAX_INT",
	"MAX_INT",
	"( -- max_int )",
	"minimum signed integer ",


	MASK_CORE | MASK_K,
	"MIN_INT",
	"MIN_INT",
	"( -- min_int )",
	"maximum signed integer ",


	MASK_CORE | MASK_K,
	"HALT",
	"HALT",
	"( -- )",
	"end the current program ",


	MASK_CORE | MASK_K,
	"NOP",
	"NOP",
	"( -- )",
	"No operation. Do nothing. ",


	MASK_CORE | MASK_K,
	"R0++",
	"r0_inc",
	"(-- R0++)",
	"Post Increment the register R0. Returns the value of R0 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R0",
	"r0_dec",
	"(-- --R0)",
	"Decrements R0 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R1++",
	"r1_inc",
	"(-- r1++)",
	"Post Increment the register R1. Returns the value of R1 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R1",
	"r1_dec",
	"(-- --r1)",
	"Decrements R1 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R2++",
	"r2_inc",
	"(-- r2++)",
	"Post Increment the register R2. Returns the value of R2 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R2",
	"r2_dec",
	"(-- --r2)",
	"Decrements R2 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R3++",
	"r3_inc",
	"(-- r3++)",
	"Post Increment the register R3. Returns the value of R3 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R3",
	"r3_dec",
	"(-- --r3)",
	"Decrements R3 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R4++",
	"r4_inc",
	"(-- r4++)",
	"Post Increment the register R4. Returns the value of R4 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R4",
	"r4_dec",
	"(-- --r4)",
	"Decrements R4 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R5++",
	"r5_inc",
	"(-- r5++)",
	"Post Increment the register R5. Returns the value of R5 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R5",
	"r5_dec",
	"(-- --r5)",
	"Decrements R5 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R6++",
	"r6_inc",
	"(-- r6++)",
	"Post Increment the register R6. Returns the value of R6 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R6",
	"r6_dec",
	"(-- --r6)",
	"Decrements R6 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R7++",
	"r7_inc",
	"(-- r7++)",
	"Post Increment the register R7. Returns the value of R7 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R7",
	"r7_dec",
	"(-- --r7)",
	"Decrements R7 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R8++",
	"r8_inc",
	"(-- r8++)",
	"Post Increment the register R8. Returns the value of R8 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R8",
	"r8_dec",
	"(-- --r8)",
	"Decrements R8 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"R9++",
	"r9_inc",
	"(-- r9++)",
	"Post Increment the register R9. Returns the value of R9 "
	"before it has been incremented. ",


	MASK_CORE | MASK_K,
	"--R9",
	"r9_dec",
	"(-- --r9)",
	"Decrements R9 by 1, and returns it. ",


	MASK_CORE | MASK_K,
	"PEEK",
	"peek",
	"(n -- value)",
	"Get the n'th data stack item from back, or -n'th item from front. ",


	MASK_CORE | MASK_K,
	"POKE",
	"poke",
	"(value n --)",
	"Set the n'th data stack item from back, or -n'th item from front. ",


	MASK_CORE | MASK_K,
	"CBLEN",
	"cblen",
	"(cb -- len)",
	"Returns the length of a code block. The code block number to use "
	"is given by 'cb'. -1 is returned for invalid code block numbers. ",


	MASK_CORE | MASK_K,
	"DSLEN",
	"dslen",
	"( -- len)",
	"This instruction returns how many data values are pushed onto the "
	"data stack (excludeding 'len'). ",


	MASK_CORE | MASK_K,
	"CSLEN",
	"cslen",
	"( -- len)",
	"Length of the call stack. ",


	MASK_CORE | MASK_K,
	"TRAP1",
	"trap1",
	"( -- )",
	"Call code block 1. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"TRAP2",
	"trap2",
	"( -- )",
	"Call code block 2. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"TRAP3",
	"trap3",
	"( -- )",
	"Call code block 3. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"TRAP4",
	"trap4",
	"( -- )",
	"Call code block 4. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"TRAP5",
	"trap5",
	"( -- )",
	"Call code block 5. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"TRAP6",
	"trap6",
	"( -- )",
	"Call code block 6. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"TRAP7",
	"trap7",
	"( -- )",
	"Call code block 7. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"TRAP8",
	"trap8",
	"( -- )",
	"Call code block 8. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"TRAP9",
	"trap9",
	"( -- )",
	"Call code block 9. This allows un-protected code "
	"to call into protected code. ",


	MASK_CORE | MASK_K,
	"NUMBER",
	"number",
	"(cb pc -- value)",
	"Fetch a number from program memory and push that value onto the "
	"data stack. 'cb' is a code block number, 'pc' is the offset from "
	"the start of the code block. This retrieves a value from the "
	"program memory. Note: KFORTH program literals are signed "
	"15-bit integers. ",


	MASK_CORE | MASK_K,
	"NUMBER!",
	"set_number",
	"(value cb pc -- )",
	"Write 'value' into program memory at (cb, pc). 'cb' is the code block "
	"number, and 'pc' is the offset into the code block. KFORTH program literals "
	"are signed 15-bit integers, therefore 'value' must be between  -16384 and 16383. ",


	MASK_CORE | MASK_K,
	"?NUMBER!",
	"test_set_number",
	"(value cb pc -- value|0)",
	"Test (and then set) the KFORTH program memory location given by (cb, pc). If "
	"it is zero then update it to contain 'value' and return value. Else return 0 and "
	"leave location (cb,pc) unchanged. 'cb' is the code block number, and 'pc' "
	"is the offset into the code block. KFORTH program literals are "
	"signed 15-bit integers. ",


	MASK_CORE | MASK_K,
	"OPCODE",
	"opcode",
	"(cb pc -- opcode)",
	"Fetch an opcode from program memory and push its numeric code onto the "
	"data stack. This retrieves the opcode from the program memory, "
	"'cb' is a code block number, 'pc' is the offset from the start of the "
	"code block. Opcodes are small integers between 0 and ~250. ",


	MASK_CORE | MASK_K,
	"OPCODE!",
	"set_opcode",
	"(opcode cb pc -- )",
	"Write an opcode to program memory. This instruction writes code. "
	"it writes 'opcode' to program memory, 'cb' is a code block number, 'pc' "
	"is the offset from the start of the code block. Opcodes are small integers "
	"between 0 and ~250. ",


	MASK_CORE | MASK_K,
	"OPCODE'",
	"lit_opcode",
	"( -- opcode )",
	"Quote the next instruction and return its opcode. Don't execute the next instruction. "
	"Execution continues with the instruction after the quoted instruction. "
	"Opcodes are small integers between 0 and ~250. ",


	MASK_INTERACT | MASK_C,
	"CMOVE",
	"CMOVE",
	"( x y -- r )",
	"Move Cell. The cell executing this instruction "
	"will be moved relative to the organism. The coordinates "
	"are normalized to (8, -9) becomes (1, -1). "
	"Cell must ensure that the organism is still connected "
	"after the move. "
	"On success r is 1, else r is 0. ",


	MASK_INTERACT | MASK_C,
	"OMOVE",
	"OMOVE",
	"( x y -- n )",
	"Move organism. Organism will move in the direction "
	"specified by (x, y). These coordinates are normalized, "
	"so that (-20, 15) maps to a direction of (-1, 1). "
	"r is 1 on success, else r is 0. ",


	MASK_INTERACT | MASK_C,
	"ROTATE",
	"ROTATE",
	"( n -- r )",
	"rotate organism. "
	"rotate in 45 degree units. positive values are "
	"clockwise rotations, and negative values are counter-clockwise. "
	"On success r is number of cells rotated. ",


	MASK_INTERACT | MASK_C,
	"EAT",
	"EAT",
	"( x y -- n )",
	"The cell executing this instruction will eat stuff at "
	"the (x, y) offset. The three things that can be eaten "
	"are Organic material, Spores, and Cells from other organisms. "
	"'e' will the amount of energy we gained by eating. ",


	MASK_INTERACT | MASK_C,
	"SEND-ENERGY",
	"SEND_ENERGY",
	"(e x y -- rc)",
	"send the energy to the cell at normalized coordinate (x,y). "
	"negative 'e' values take from that cell. ",


	MASK_INTERACT | MASK_C,
	"MAKE-SPORE",
	"MAKE_SPORE",
	"( x y e -- r )",
	"A spore is created at the normalized coordinates (x, y). The "
	"spore will be given energy 'e'. Normalized coordinates mean "
	"that (-3, 0) becomes (-1, 0). s is set to 0 on failure. "
	"s is set to 1 if we created a spore. s is set to -1 if we "
	"fertilized an existing spore. ",


	MASK_INTERACT | MASK_C,
	"GROW",
	"GROW",
	"( x y -- r )",
	"A new cell is added to the organism. The coordinates (x, y) are "
	"normalized, meaning (123, -999) becomes (1, -1). The new cell "
	"inherits the all the execution context of the cell that executed "
	"this instruction. returns 1=parent cell, -1=new cell, 0=failure. ",


	MASK_VISION | MASK_C,
	"LOOK",
	"LOOK",
	"( x y -- what dist )",
	"Look in direction (x, y) and return 'what' was seen, and how "
	"far away it was 'dist'. ",


	MASK_VISION | MASK_C,
	"NEAREST",
	"NEAREST",
	"( mask -- x y )",
	"Returns the (x,y) coordinates for the vision data with the SMALLEST distance "
	"that matches mask. 'mask' is an OR'ing of 'what' values. "
	"If nothing matches (0,0) is returned. ",


	MASK_VISION | MASK_C,
	"FARTHEST",
	"FARTHEST",
	"( mask -- x y )",
	"Returns the (x,y) coordinates for the vision data with the LARGEST distance "
	"that matches mask. 'mask' is an OR'ing of 'what' values. "
	"If nothing matches (0,0) is returned. ",


	MASK_COMMS | MASK_C,
	"MOOD",
	"MOOD",
	"( x y -- m )",
	"We examine one of our own cells at offset (x, y) and return the MOOD "
	"of that cell. If (x, y) is invalid 0 is returned. ",


	MASK_COMMS | MASK_C,
	"MOOD!",
	"SET_MOOD",
	"( m -- )",
	"The cell executing this instruction will set its mood variable to 'm'. ",


	MASK_COMMS | MASK_C,
	"BROADCAST",
	"BROADCAST",
	"( m -- )",
	"The message 'm' will be placed in the message buffer for every cell "
	"in our organism. ",


	MASK_COMMS | MASK_C,
	"SEND",
	"SEND",
	"( m x y -- )",
	"The message 'm' will be placed in the message buffer for the cell "
	"indicated by the vector (x, y). Any cell offset can be speficied. ",


	MASK_COMMS | MASK_C,
	"RECV",
	"RECV",
	"( -- m )",
	"Our message buffer 'm' is placed on top of the data stack. ",


	MASK_QUERY | MASK_C,
	"ENERGY",
	"ENERGY",
	"( -- e )",
	"Get our organisms overall energy. ",


	MASK_QUERY | MASK_C,
	"AGE",
	"AGE",
	"( -- a )",
	"Fetch the organisms 'age' field (which is the number of elapsed simulation step) "
	"and push this value on our data stack. ",


	MASK_QUERY | MASK_C,
	"NUM-CELLS",
	"NUM_CELLS",
	"( -- n )",
	"Get the number of cells in our organism 'n' on top of the data stack. ",


	MASK_QUERY | MASK_C,
	"HAS-NEIGHBOR",
	"HAS_NEIGHBOR",
	"( x y -- r )",
	"check if we have a neighboring cell at the normalized coordinates (x, y). "
	"s is set to 1 if there is a neighbor, else s is set to 0. ",


	MASK_INTERACT | MASK_C,
	"MAKE-ORGANIC",
	"MAKE_ORGANIC",
	"(x y e -- s)",
	"Create organic block at (x,y) offset with energy 'e'. ",


	MASK_INTERACT | MASK_C,
	"GROW.CB",
	"GROW_CB",
	"(x y cb -- r)",
	"Grow a new cell at (x,y) normalized offset. The new cell inherits the context "
	"of the cell which executed the GROW.CB instruction. However, it will begin execution "
	"at code block 'cb'. ",


	MASK_INTERACT | MASK_C,
	"CSHIFT",
	"CSHIFT",
	"(x y -- r)",
	"Shift a line of cells along the normalized vector (x,y) "
	"starting with the cell executing this instruction. ",


	MASK_INTERACT | MASK_C,
	"SPAWN",
	"SPAWN",
	"(x y e strain cb -- r)",
	"Spawn a new organism. Returns 1 on success, else 0. ",


	MASK_INTERACT | MASK_C,
	"MAKE-BARRIER",
	"MAKE_BARRIER",
	"(x y -- r)",
	"Creates a barrier block at the x y normalized offet. Returns 1 on "
	"success and 0 on failure. ",


	MASK_VISION | MASK_C,
	"SIZE",
	"SIZE",
	"(x y -- size dist)",
	"Report the size (number of cells) of the object seen "
	"along the normalized vector (x,y) from this cell. ",


	MASK_VISION | MASK_C,
	"BIGGEST",
	"BIGGEST",
	"(mask -- x y)",
	"Look in all 8 directions. Return a non-normalized (x,y) vector to "
	"the biggest thing. This is the organism consisting of the most number of cells. "
	"The returned (x,y) vector is not normalized. ",


	MASK_VISION | MASK_C,
	"SMALLEST",
	"SMALLEST",
	"(mask -- x y)",
	"Look in all 8 directions. Return a non-normalized (x,y) vector to "
	"the smallest thing. This is the organism with the least number of cells. "
	"The returned (x,y) vector is not normalized. ",


	MASK_VISION | MASK_C,
	"TEMPERATURE",
	"TEMPERATURE",
	"(x y -- energy dist)",
	"get the energy of cell seen along (x,y) ",


	MASK_VISION | MASK_C,
	"HOTTEST",
	"HOTTEST",
	"(mask -- x y)",
	"Look in all 8 directions. Return a non-normalized (x,y) vector to "
	"the hottest thing. This is the organism with the most energy per cell. "
	"The returned (x,y) vector is not normalized, so it might return (-6,6) instead of (-1,1). ",


	MASK_VISION | MASK_C,
	"COLDEST",
	"COLDEST",
	"(mask -- x y)",
	"Look in all 8 directions. Return a non-normalized (x,y) vector to "
	"the coldest thing. This is the organism with the least energy per cell. "
	"The returned (x,y) vector is not normalized. ",


	MASK_COMMS | MASK_C,
	"SHOUT",
	"SHOUT",
	"(m -- r)",
	"Broadcast a message to another organism. "
	"Send the message 'm' outward in all 8 directions. "
	"If a cell is reached, then set it's MESSAGE register to be 'm'. ",


	MASK_COMMS | MASK_C,
	"SAY",
	"SAY",
	"(m x y -- dist)",
	"The (x,y) normalized direction vector will be used to "
	"send the message 'm'. If a cell is reached, then set it's "
	"MESSAGE register to be 'm'. Returns non-zero on success. ",


	MASK_COMMS | MASK_C,
	"LISTEN",
	"LISTEN",
	"(x y -- mood dist)",
	"Listen along the normalized vector (x,y). If another cell was found "
	"return its MOOD register and distance. ",


	MASK_COMMS | MASK_C,
	"READ",
	"READ",
	"(x y cb cbme -- rc)",
	"Read code block 'cb' from the cell or spore located "
	"at the normalized (x,y) coordinates. Place the result "
	"into 'cbme'. "
	"On success, the length of the code block read is returned. "
	"On failure, a negative error code is returned. ",


	MASK_COMMS | MASK_C,
	"WRITE",
	"WRITE",
	"(x y cb cbme -- rc)",
	"Write code block 'cbme' to another cell or spore. The cell "
	"is located at the normalized (x, y) offset from this cell. "
	"The destination code block number is given by 'cb'. "
	"On success, the length of the code block written is returned. "
	"On failure, a negative error code is returned. ",


	MASK_INTERACT | MASK_C,
	"EXUDE",
	"EXUDE",
	"(value x y -- )",
	"excrete a value onto the grid ",


	MASK_INTERACT | MASK_C,
	"SMELL",
	"SMELL",
	"(x y -- value)",
	"read a value from the grid that was previously EXUDE'd. ",


	MASK_UNIVERSE | MASK_C,
	"G0",
	"G0",
	"( -- value)",
	"Get universe-wide global variable. ",


	MASK_UNIVERSE | MASK_C,
	"G0!",
	"SET_G0",
	"(value -- )",
	"Set universe-wide global variable. ",


	MASK_UNIVERSE | MASK_C,
	"S0",
	"S0",
	"( -- value)",
	"Get strain-wide global variable. ",


	MASK_UNIVERSE | MASK_C,
	"S0!",
	"SET_S0",
	"(value -- )",
	"Set strain-wide global variable. ",


	MASK_QUERY | MASK_C,
	"NEIGHBORS",
	"NEIGHBOR",
	"( -- mask)",
	"Returns a bitmask with a '1' bit set for a neighor in that direction. ",


	MASK_UNIVERSE | MASK_C,
	"POPULATION",
	"POPULATION",
	"( -- pop)",
	"Return the current population. ",


	MASK_UNIVERSE | MASK_C,
	"POPULATION.S",
	"POPULATION_STRAIN",
	"( -- pop)",
	"Return the current population of my strain. ",


	MASK_QUERY | MASK_C,
	"GPS",
	"GPS",
	"( -- x y)",
	"Returns the absolute grid coordinates for the cell executing "
	"this instruction. ",


	MASK_UNIVERSE | MASK_C,
	"KEY-PRESS",
	"KEY_PRESS",
	"( -- key)",
	"Get the keyboard character being pressed. Or 0 if nothing is being "
	"pressed. ",


	MASK_UNIVERSE | MASK_C,
	"MOUSE-POS",
	"MOUSE_POS",
	"( -- x y)",
	"Query the mouse position. Returns (0, 0) if no mouse position set. ",


	MASK_MISC | MASK_C,
	"DIST",
	"DIST",
	"(x y -- dist)",
	"Computes the maximum of ABS(x) and ABS(y). ",


	MASK_MISC | MASK_C,
	"CHOOSE",
	"CHOOSE",
	"(min max -- rnd)",
	"pick random value between two values ",


	MASK_MISC | MASK_C,
	"RND",
	"RND",
	"( -- rnd)",
	"return random value between MIN_INT and MAX_INT. ",


	MASK_FIND | MASK_F,
	"ID",
	"Find_ID",
	"( -- id)",
	"The organism ID (modulos 10,000). ",


	MASK_FIND | MASK_F,
	"PARENT1",
	"Find_PARENT1",
	"( -- parent1)",
	"The parent1 ID (modulos 10,000). ",


	MASK_FIND | MASK_F,
	"PARENT2",
	"Find_PARENT2",
	"( -- parent2)",
	"The parent2 ID (modulos 10,000). ",


	MASK_FIND | MASK_F,
	"STRAIN",
	"Find_STRAIN",
	"( -- strain)",
	"The strain number. ",


	MASK_FIND | MASK_F,
	"ENERGY",
	"Find_ENERGY",
	"( -- e)",
	"The energy of the organism. ",


	MASK_FIND | MASK_F,
	"GENERATION",
	"Find_GENERATION",
	"( -- g)",
	"The generation. ",


	MASK_FIND | MASK_F,
	"NUM-CELLS",
	"Find_NUM_CELLS",
	"( -- n)",
	"The number of cells that the organism consists of. ",


	MASK_FIND | MASK_F,
	"AGE",
	"Find_AGE",
	"( -- n)",
	"The age of the organism (divided by 1,000). ",


	MASK_FIND | MASK_F,
	"NCHILDREN",
	"Find_NCHILDREN",
	"( -- n)",
	"The number of living children descended from this organism. ",


	MASK_FIND | MASK_F,
	"EXECUTING",
	"Find_EXECUTING",
	"(cb -- bool)",
	"Does organism have a cell currently executing inside code block 'cb'? ",


	MASK_FIND | MASK_F,
	"NUM-CB",
	"Find_NUM_CB",
	"( -- n)",
	"The total number of code blocks the organism has. ",


	MASK_FIND | MASK_F,
	"NUM-DEAD",
	"Find_NUM_DEAD",
	"( -- n)",
	"The number of cells that have just dies (colored red). ",


	MASK_FIND | MASK_F,
	"MAX-ENERGY",
	"Find_MAX_ENERGY",
	"( -- e)",
	"Constant: for all organisms return the MAXIMUM energy amount. ",


	MASK_FIND | MASK_F,
	"MIN-ENERGY",
	"Find_MIN_ENERGY",
	"( -- e)",
	"Constant: for all organisms return the MINIMUM energy amount. ",


	MASK_FIND | MASK_F,
	"AVG-ENERGY",
	"Find_AVG_ENERGY",
	"( -- e)",
	"Constant: for all organisms return the AVERAGE energy amount ",


	MASK_FIND | MASK_F,
	"MAX-AGE",
	"Find_MAX_AGE",
	"( -- n)",
	"Constant: for all organisms return the MAXIMUM age (divided by 1,000). ",


	MASK_FIND | MASK_F,
	"MIN-AGE",
	"Find_MIN_AGE",
	"( -- n)",
	"Constant: for all organisms return the MINIMUM age (divided by 1,000). ",


	MASK_FIND | MASK_F,
	"AVG-AGE",
	"Find_AVG_AGE",
	"( -- n)",
	"Constant: for all organisms return the AVERAGE age (divided by 1,000). ",


	MASK_FIND | MASK_F,
	"MAX-NUM-CELLS",
	"Find_MAX_NUM_CELLS",
	"( -- n)",
	"Constant: for all organisms return the MAXIMUM number of cells an organism has. ",



};

KFORTH_IHELP* Kforth_Instruction_Help = &Kforth_Instruction_Help_Table[0];

int Kforth_Instruction_Help_len = sizeof(Kforth_Instruction_Help_Table)/sizeof(KFORTH_IHELP);

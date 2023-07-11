#pragma once

typedef unsigned char byte;
typedef unsigned short word;

//----------------------------------------------
// CPU State
//----------------------------------------------
struct CPU {
	CPU() {
		this->memory = new byte[0xffff];
		for (size_t i = 0; i < 14; i++) {
			regsWide[i] = 0;
		}
	}

	~CPU() {
		delete[] memory;
	}

	// Stored in a union to let short and wide registers overlap
	union {
		word regsWide[14];
		struct {
			word ax, cx, dx, bx;
			word sp, bp, si, di;
			word cs, ds, ss, es;
			word ip, flags;
		};

		byte regsShort[8];
		struct {
			byte al, ah, cl, ch, dl, dh, bl, bh;
		};
	};

	// Memory 
	byte* memory;
};

//----------------------------------------------
// Registers
//----------------------------------------------
enum class Register : int {
	// Short Registers
	AL, CL, DL, BL,
	AH, CH, DH, BH,
	// Wide Registers
	AX, CX, DX, BX,
	SP, BP, SI, DI,
	// Segment Registers
	CS, DS, SS, ES,
	// Instruction Pointer and Flags
	IP, FLAGS
};

//----------------------------------------------
// Opcodes
//----------------------------------------------
enum class OpCode {
	ERROR,
	// Move
	MOVE_TOFROM_REGMEM,
	MOVE_IMMEDIATE_TO_REGMEM,
	MOVE_IMMEDIATE_TO_REG,
	MOVE_MEMORY_TO_ACCUMULATOR,
	MOVE_ACCUMULATOR_TO_MEMORY,
	MOVE_REGMEM_TO_SEGMENT,
	MOVE_SEGMENT_TO_REGMEM,

	// Arithmetic
	ADD_TOFROM_REGMEM,
	ADD_IMMEDIATE_TO_ACCUMULATOR,
	ADD_IMMEDIATE_TO_REGMEM,
	SUB_TOFROM_REGMEM,
	SUB_IMMEDIATE_TO_ACCUMULATOR,
	SUB_IMMEDIATE_TO_REGMEM,

	// Compare
	CMP_REG_WITH_REGMEM,
	CMP_IMMEDIATE_WITH_REGMEM,
	CMP_IMMEDIATE_WITH_ACCUMULATOR,

	// Jumps
	JUMP_ON_EQUAL_OR_ZERO,
	JUMP_ON_LESS,
	JUMP_ON_LESS_OR_EQUAL,
	JUMP_ON_BELOW,
	JUMP_ON_BELOW_OR_EQUAL,
	JUMP_ON_PARITY,
	JUMP_ON_OVERFLOW,
	JUMP_ON_SIGN,
	JUMP_ON_NOT_EQUAL_OR_ZERO,
	JUMP_ON_GREATER_OR_EQUAL,
	JUMP_ON_GREATER,
	JUMP_ON_ABOVE_OR_EQUAL,
	JUMP_ON_ABOVE,
	JUMP_ON_NOT_PARITY,
	JUMP_ON_NOT_OVERFLOW,
	JUMP_ON_NOT_SIGN,
	LOOP_CX_TIMES,
	LOOP_WHILE_ZERO,
	LOOP_WHILE_NOT_ZERO,
	JUMP_ON_CX_ZERO,
};

enum OpFlag : int {
	NONE = 0,
	MOD = 1 << 0,
	REG = 1 << 1,
	REGMEM = 1 << 2,
	W = 1 << 3,
	D = 1 << 4,
	S = 1 << 5,
	V = 1 << 6,
	SEGREG = 1 << 7,
	ONE_BYTE_OP = 1 << 8,
	W_IN_FIRSTBYTE = 1 << 9,
	REG_IN_FIRSTBYTE = 1 << 10,
	HAS_DATA = 1 << 11,
	HAS_ADDR = 1 << 12,
	HAS_IP8 = 1 << 13,
};

//----------------------------------------------
// EffectiveAddress
//----------------------------------------------
enum class EffectiveAddress {
	BX_SI,
	BX_DI,
	BP_SI,
	BP_DI,
	SI,
	DI,
	BP,
	BX,
	DIRECT_ADDRESS
};

//----------------------------------------------
// Operation helpers
//----------------------------------------------
enum class MoveMode {
	Register,
	Memory,
};

enum class ExplicitDataSize {
	Byte,
	Word
};

struct RegMem {
	MoveMode type;
	Register reg;
	EffectiveAddress effectiveAddress;
	int memoryOffset;
};

// One instruction fits all
struct Instruction {
	OpCode inst;
	RegMem source;
	RegMem dest;
	word immediate;
	ExplicitDataSize dataSize;
	int relativeJump;
};

struct Buffer {
	byte* data;
	int size;
};

struct DecodeContext {
	int bp;
	byte* buffer;
};
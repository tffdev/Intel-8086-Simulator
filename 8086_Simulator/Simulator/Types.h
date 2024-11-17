#pragma once
#include "String.h"
#include "List.h"

typedef unsigned char byte;
typedef unsigned short word;

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
	IP, FLAGS,

	INVALID
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
	JUMP_ALWAYS_RELATIVE_WIDE,
	LOOP_CX_TIMES,
	LOOP_WHILE_ZERO,
	LOOP_WHILE_NOT_ZERO,
	JUMP_ON_CX_ZERO,

	// Interrupts
	INTERRUPT,
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
	DIRECT_ADDRESS,
	INVALID
};

//----------------------------------------------
// Operation helpers
//----------------------------------------------
struct Buffer {
	byte* data;
	int size;
};

//----------------------------------------------
// Operations and data
//----------------------------------------------
enum class ExplicitDataSize {
	NONE,
	BYTE,
	WORD
};

struct Address {
	EffectiveAddress effectiveAddress = EffectiveAddress::INVALID;
	word memoryOffset = 0;
};

struct Operand {
	enum class Type {
		NONE,
		REGISTER,
		MEMORY_LOC,
		IMMEDIATE
	};

	Type type = Type::NONE;
	ExplicitDataSize dataSize = ExplicitDataSize::NONE;
	union {
		Register reg = Register::INVALID;
		Address mem;
		word immediate;
	};
};

enum InstructionType {
	NONE,
	MOVE,
	ADD,
	SUB,
	COMPARE,
	JUMP,
	INTERRUPT,
};

struct InstructionMove {
	Operand source;
	Operand dest;
};

struct InstructionAdd {
	Operand source;
	Operand dest;
};

struct InstructionSub {
	Operand source;
	Operand dest;
};

struct InstructionCompare {
	Operand source;
	Operand dest;
};

struct InstructionJump {
	enum Condition {
		JumpAlways = 0b11101001,
		JumpOnEqualOrZero = 0b01110100,
		JumpOnLess = 0b01111100,
		JumpOnLessOrEqual = 0b01111110,
		JumpOnBelow = 0b01110010,
		JumpOnBelowOrEqual = 0b01110110,
		JumpOnParity = 0b01111010,
		JumpOnOverflow = 0b01110000,
		JumpOnSign = 0b01111000,
		JumpOnNotEqualOrZero = 0b01110101,
		JumpOnGreaterOrEqual = 0b01111101,
		JumpOnGreater = 0b01111111,
		JumpOnAboveOrEqual = 0b01110011,
		JumpOnAbove = 0b01110111,
		JumpOnNotParity = 0b01111011,
		JumpOnNotOverflow = 0b01110001,
		JumpOnNotSign = 0b01111001,
		Loop = 0b11100010,
		LoopEqualOrZero = 0b11100001,
		LoopNotEqualOrZero = 0b11100000,
		JumpOnCXZero = 0b11100011,
	};
	Condition condition;
	int byteOffset;
	int byteLocation;
	int instructionIndex;
};

struct InstructionInterrupt {
	byte interruptNumber;
};

struct InstructionGeneric {
	InstructionType type = InstructionType::NONE;
	int index;
	String asString;
	union {
		InstructionMove move{};
		InstructionAdd add;
		InstructionSub sub;
		InstructionCompare compare;
		InstructionJump jump;
		InstructionInterrupt interrupt;
	};
};
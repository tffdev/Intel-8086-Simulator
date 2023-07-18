#include "String.h"
#include "Types.h"
#include <stdio.h>
#include "StringifyTypes.h"

namespace DecoderYanDev {

OpCode ParseOpCode(byte code, byte secondByte) {
	// get "reg" from second byte
	byte reg = (secondByte & 0b00111000) >> 3;
	// Move instructions
	if ((code & 0b11111100) == 0b10001000) return OpCode::MOVE_TOFROM_REGMEM;
	if ((code & 0b11111110) == 0b11000110) return OpCode::MOVE_IMMEDIATE_TO_REGMEM;
	if ((code & 0b11110000) == 0b10110000) return OpCode::MOVE_IMMEDIATE_TO_REG;
	if ((code & 0b11111110) == 0b10100000) return OpCode::MOVE_MEMORY_TO_ACCUMULATOR;
	if ((code & 0b11111110) == 0b10100010) return OpCode::MOVE_ACCUMULATOR_TO_MEMORY;
	if ((code & 0b11111111) == 0b10001110) return OpCode::MOVE_REGMEM_TO_SEGMENT;
	if ((code & 0b11111111) == 0b10001100) return OpCode::MOVE_SEGMENT_TO_REGMEM;
	// Arithmetic instructions
	if ((code & 0b11111100) == 0b00000000) return OpCode::ADD_TOFROM_REGMEM;
	if ((code & 0b11111110) == 0b00000100) return OpCode::ADD_IMMEDIATE_TO_ACCUMULATOR;
	if (((code & 0b11111100) == 0b10000000) && (reg == 0b000)) return OpCode::ADD_IMMEDIATE_TO_REGMEM;
	if ((code & 0b11111100) == 0b00101000) return OpCode::SUB_TOFROM_REGMEM;
	if ((code & 0b11111110) == 0b00101100) return OpCode::SUB_IMMEDIATE_TO_ACCUMULATOR;
	if (((code & 0b11111100) == 0b10000000) && (reg == 0b101)) return OpCode::SUB_IMMEDIATE_TO_REGMEM;
	if ((code & 0b11111100) == 0b00111000) return OpCode::CMP_REG_WITH_REGMEM;
	if ((code & 0b11111110) == 0b00111100) return OpCode::CMP_IMMEDIATE_WITH_ACCUMULATOR;
	if (((code & 0b11111100) == 0b10000000) && (reg == 0b111)) return OpCode::CMP_IMMEDIATE_WITH_REGMEM;
	// Jump instructions
	if ((code & 0b11111111) == 0b01110100) return OpCode::JUMP_ON_EQUAL_OR_ZERO;
	if ((code & 0b11111111) == 0b01111100) return OpCode::JUMP_ON_LESS;
	if ((code & 0b11111111) == 0b01111110) return OpCode::JUMP_ON_LESS_OR_EQUAL;
	if ((code & 0b11111111) == 0b01110010) return OpCode::JUMP_ON_BELOW;
	if ((code & 0b11111111) == 0b01110110) return OpCode::JUMP_ON_BELOW_OR_EQUAL;
	if ((code & 0b11111111) == 0b01111010) return OpCode::JUMP_ON_PARITY;
	if ((code & 0b11111111) == 0b01110000) return OpCode::JUMP_ON_OVERFLOW;
	if ((code & 0b11111111) == 0b01111000) return OpCode::JUMP_ON_SIGN;
	if ((code & 0b11111111) == 0b01110101) return OpCode::JUMP_ON_NOT_EQUAL_OR_ZERO;
	if ((code & 0b11111111) == 0b01111101) return OpCode::JUMP_ON_GREATER_OR_EQUAL;
	if ((code & 0b11111111) == 0b01111111) return OpCode::JUMP_ON_GREATER;
	if ((code & 0b11111111) == 0b01110011) return OpCode::JUMP_ON_ABOVE_OR_EQUAL;
	if ((code & 0b11111111) == 0b01110111) return OpCode::JUMP_ON_ABOVE;
	if ((code & 0b11111111) == 0b01111011) return OpCode::JUMP_ON_NOT_PARITY;
	if ((code & 0b11111111) == 0b01110001) return OpCode::JUMP_ON_NOT_OVERFLOW;
	if ((code & 0b11111111) == 0b01111001) return OpCode::JUMP_ON_NOT_SIGN;
	if ((code & 0b11111111) == 0b11100010) return OpCode::LOOP_CX_TIMES;
	if ((code & 0b11111111) == 0b11100011) return OpCode::LOOP_WHILE_ZERO;
	if ((code & 0b11111111) == 0b11100001) return OpCode::LOOP_WHILE_NOT_ZERO;
	if ((code & 0b11111111) == 0b11100000) return OpCode::JUMP_ON_CX_ZERO;
}

EffectiveAddress RMToEffectiveAddress(char rm) {
	return (EffectiveAddress)rm;
}

Register RegisterParse(char reg, bool w) {
	int index = (int)reg + (w ? 8 : 0);
	return (Register)index;
}

void CalculateReg(char mod, bool isWide, char reg, RegMem& out) {
	out.type = MoveMode::REGISTER;
	out.reg = RegisterParse(reg, isWide);
}

int CalculateRegMem(char mod, byte* bufferPossibleMemStart, bool isWide, char regMem, RegMem& out) {
	// Account for rm being 110, which is a special case with 16-bit displacement
	bool directAddress = (mod == 0b00) && (regMem == 0b110);
	if (directAddress) {
		out.type = MoveMode::MEMORY;
		out.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;
		out.memoryOffset = (unsigned short)((bufferPossibleMemStart[1] << 8) | bufferPossibleMemStart[0]);
		return 2;
	}

	switch (mod) {
	case 0b00: {
		// No displacement, to memory
		out.type = MoveMode::MEMORY;
		out.effectiveAddress = RMToEffectiveAddress(regMem);
		out.memoryOffset = 0;
		return 0;
	}
	case 0b01: {
		// 8-bit displacement, to memory
		out.type = MoveMode::MEMORY;
		out.effectiveAddress = RMToEffectiveAddress(regMem);
		out.memoryOffset = (signed char)bufferPossibleMemStart[0];
		return 1;
	}
	case 0b10: {
		// 16-bit displacement, to memory
		out.type = MoveMode::MEMORY;
		out.effectiveAddress = RMToEffectiveAddress(regMem);
		out.memoryOffset = (signed short)((bufferPossibleMemStart[1] << 8) | bufferPossibleMemStart[0]);
		return 2;
	}
	case 0b11: {
		// No displacement, to register
		out.type = MoveMode::REGISTER;
		out.reg = RegisterParse(regMem, isWide);
		return 0;
	}
	default:
		printf("ERROR: Unhandled mod %d\n", mod);
		return 0;
	}
}

int ParseImmediateData(byte* buffer, bool isWide) {
	if (isWide) {
		return (signed short)((buffer[1] << 8) | buffer[0]);
	}
	else {
		return (signed char)(buffer[0]);
	}
}

// Function that given a mod byte, returns the mode, memory offset (if applicable), and number of bytes to read
int CalculateSourceAndDestFromMode(char mod, byte* bufferPossibleMemStart, bool destinationIsRegister, bool isWide, char reg, char regMem, RegMem& source, RegMem& dest) {
	RegMem& slotReg = destinationIsRegister ? dest : source;
	CalculateReg(mod, isWide, reg, slotReg);

	RegMem& slotMem = destinationIsRegister ? source : dest;
	return CalculateRegMem(mod, bufferPossibleMemStart, isWide, regMem, slotMem);
}

//----------------------------------------------
// MOVE Register/memory to/from register
//----------------------------------------------
struct OperationMoveToFromRegMem {
public:
	RegMem source;
	RegMem dest;
};

int OperationMoveToFromRegMemParse(unsigned char* buffer, OperationMoveToFromRegMem& move) {
	bool destIsReg = buffer[0] & 0b00000010;
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	int bytesToShift = CalculateSourceAndDestFromMode(mod, &buffer[2], destIsReg, isWide, reg, regMem, move.source, move.dest);
	return 2 + bytesToShift;
}

void OperationMoveToFromRegMemPrint(OperationMoveToFromRegMem& move) {
	printf("mov %s, %s\n", RegMemToString(move.dest).c_str(), RegMemToString(move.source).c_str());
}

//----------------------------------------------
// MOVE immediate to register/memory
//----------------------------------------------
struct OperationMoveImmediateToRegMem {
	RegMem dest;
	int immediateData;
	ExplicitDataSize dataSize;
};

int OperationMoveImmediateToRegMemParse(byte* buffer, OperationMoveImmediateToRegMem& move) {
	bool isDataWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char regMem = (buffer[1] & 0b00000111);

	move.dataSize = isDataWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;

	// Calc effective address
	int byteOffset = CalculateRegMem(mod, &buffer[2], isDataWide, regMem, move.dest);
	byte* dataLocation = buffer + 2 + byteOffset;
	move.immediateData = isDataWide ? ((dataLocation[1] << 8) | dataLocation[0]) : dataLocation[0];
	return 2 + byteOffset + (isDataWide ? 2 : 1);
}

void OperationMoveImmediateToRegMemPrint(OperationMoveImmediateToRegMem move) {
	printf("mov %s, %s%d\n", RegMemToString(move.dest).c_str(), DataSizeToString(move.dataSize).c_str(), move.immediateData);
}

//----------------------------------------------
// MOVE immediate to register
//----------------------------------------------
struct OperationMoveImmediateToRegister {
	Register registerDst;
	int immediateData;
};

int OperationMoveImmediateToRegisterParse(unsigned char* buffer, OperationMoveImmediateToRegister& move) {
	bool isWide = buffer[0] & 0b00001000;
	char reg = (buffer[0] & 0b00000111);
	move.registerDst = RegisterParse(reg, isWide);
	move.immediateData = ParseImmediateData(&buffer[1], isWide);
	return isWide ? 3 : 2;
}

void OperationMoveImmediateToRegisterPrint(OperationMoveImmediateToRegister move) {
	printf("mov %s, %d\n", RegisterToString(move.registerDst).c_str(), move.immediateData);
}

//----------------------------------------------
// MOVE memory to accumulator
//----------------------------------------------
struct OperationMoveMemoryToAccumulator {
	int memoryLocation;
	ExplicitDataSize dataSize;
	RegMem dest;
};

int OperationMoveMemoryToAccumulatorParse(unsigned char* buffer, OperationMoveMemoryToAccumulator& move) {
	bool isWide = buffer[0] & 0b00000001;
	move.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;
	move.dest.type = MoveMode::REGISTER;
	move.dest.reg = isWide ? Register::AX : Register::AL;

	if (isWide) {
		move.memoryLocation = (buffer[2] << 8) | buffer[1];
		return 3;
	}
	else {
		move.memoryLocation = buffer[1];
		return 2;
	}
}

void OperationMoveMemoryToAccumulatorPrint(OperationMoveMemoryToAccumulator move) {
	printf("mov %s, [%i]\n", RegMemToString(move.dest).c_str(), move.memoryLocation);
}

//----------------------------------------------
// MOVE accumulator to memory
//----------------------------------------------
struct OperationMoveAccumulatorToMemory {
	int memoryLocation;
};

int OperationMoveAccumulatorToMemoryParse(unsigned char* buffer, OperationMoveAccumulatorToMemory& move) {
	bool isWide = buffer[0] & 0b00000001;
	if (isWide) {
		move.memoryLocation = (buffer[2] << 8) | buffer[1];
		return 3;
	}
	else {
		move.memoryLocation = buffer[1];
		return 2;
	}
}

void OperationMoveAccumulatorToMemoryPrint(OperationMoveAccumulatorToMemory move) {
	printf("mov [%i], ax\n", move.memoryLocation);
}

//----------------------------------------------
// MOVE RegMem to segment register
//----------------------------------------------
struct OperationMoveRegMemToSegmentRegister {
	RegMem source;
	Register dest;
};

Register SRToSegmentRegister(char sr) {
	switch (sr) {
	case 0b00: return Register::ES;
	case 0b01: return Register::CS;
	case 0b10: return Register::SS;
	case 0b11: return Register::DS;
	}
}

int OperationMoveRegMemToSegmentRegisterParse(unsigned char* buffer, OperationMoveRegMemToSegmentRegister& move) {
	bool isWide = true;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char sr = (buffer[1] & 0b00011000) >> 3;
	char regMem = (buffer[1] & 0b00000111);
	move.dest = SRToSegmentRegister(sr);

	// Calc effective address
	int byteOffset = CalculateRegMem(mod, &buffer[2], isWide, regMem, move.source);
	return 2 + byteOffset;
}

void OperationMoveRegMemToSegmentRegisterPrint(OperationMoveRegMemToSegmentRegister move) {
	printf("mov %s, %s\n", RegisterToString(move.dest).c_str(), RegMemToString(move.source).c_str());
}

//----------------------------------------------
// MOVE segment register to RegMem
//----------------------------------------------
struct OperationMoveSegmentRegisterToRegMem {
	Register source;
	RegMem dest;
};

int OperationMoveSegmentRegisterToRegMemParse(unsigned char* buffer, OperationMoveSegmentRegisterToRegMem& move) {
	bool isWide = true;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char sr = (buffer[1] & 0b00011000) >> 3;
	char regMem = (buffer[1] & 0b00000111);
	move.source = SRToSegmentRegister(sr);

	// Calc effective address
	int byteOffset = CalculateRegMem(mod, &buffer[2], isWide, regMem, move.dest);
	return 2 + byteOffset;
}

void OperationMoveSegmentRegisterToRegMemPrint(OperationMoveSegmentRegisterToRegMem move) {
	printf("mov %s, %s\n", RegMemToString(move.dest).c_str(), RegisterToString(move.source).c_str());
}

//----------------------------------------------
// ADD immediate to register/memory
//----------------------------------------------
struct OperationAddImmediateToRegMem {
	RegMem dest;
	int immediateData;
	ExplicitDataSize dataSize;
};

int OperationAddImmediateToRegMemParse(unsigned char* buffer, OperationAddImmediateToRegMem& add) {
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char regMem = (buffer[1] & 0b00000111);
	bool s = (buffer[0] & 0b00000010) >> 1;

	bool isDataWide = (!s) && (isWide);

	add.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;

	// Calc effective address
	int addressOffset = CalculateRegMem(mod, &buffer[2], isWide, regMem, add.dest);
	byte* dataLocation = buffer + 2 + addressOffset;

	add.immediateData = ParseImmediateData(dataLocation, isDataWide);
	return 2 + addressOffset + (isDataWide ? 2 : 1);
}

void OperationAddImmediateToRegMemPrint(OperationAddImmediateToRegMem add) {
	printf("add %s%s, %d\n", DataSizeToString(add.dataSize).c_str(), RegMemToString(add.dest).c_str(), add.immediateData);
}

//----------------------------------------------
// ADD to from register memory
//----------------------------------------------
struct OperationAddToFromRegMem {
	RegMem source;
	RegMem dest;
};

int OperationAddToFromRegMemParse(unsigned char* buffer, OperationAddToFromRegMem& add) {
	bool isWide = buffer[0] & 0b00000001;
	bool destIsReg = buffer[0] & 0b00000010;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	CalculateReg(mod, isWide, reg, destIsReg ? add.dest : add.source);
	int addressOffset = CalculateRegMem(mod, &buffer[2], isWide, regMem, destIsReg ? add.source : add.dest);
	return 2 + addressOffset;
}

void OperationAddToFromRegMemPrint(OperationAddToFromRegMem& add) {
	printf("add %s, %s\n", RegMemToString(add.dest).c_str(), RegMemToString(add.source).c_str());
}

//----------------------------------------------
// ADD immediate to accumulator
//----------------------------------------------
struct OperationAddImmediateToAccumulator {
	int immediateData;
	bool isWide;
	RegMem dest;
};

int OperationAddImmediateToAccumulatorParse(unsigned char* buffer, OperationAddImmediateToAccumulator& add) {
	bool isWide = buffer[0] & 0b00000001; // explicit wide

	add.dest.type = MoveMode::REGISTER;
	add.dest.reg = isWide ? Register::AX : Register::AL;
	add.immediateData = ParseImmediateData(&buffer[1], isWide);

	return isWide ? 3 : 2;
}

void OperationAddImmediateToAccumulatorPrint(OperationAddImmediateToAccumulator add) {
	printf("add %s, %d\n", RegMemToString(add.dest).c_str(), add.immediateData);
}

//----------------------------------------------
// SUB to from register memory
//----------------------------------------------
struct OperationSubToFromRegMem {
	RegMem source;
	RegMem dest;
};

int OperationSubToFromRegMemParse(unsigned char* buffer, OperationSubToFromRegMem& sub) {
	bool isWide = buffer[0] & 0b00000001;
	bool destIsReg = buffer[0] & 0b00000010;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	CalculateReg(mod, isWide, reg, destIsReg ? sub.dest : sub.source);
	int addressOffset = CalculateRegMem(mod, &buffer[2], isWide, regMem, destIsReg ? sub.source : sub.dest);
	return 2 + addressOffset;
}

void OperationSubToFromRegMemPrint(OperationSubToFromRegMem& sub) {
	printf("sub %s, %s\n", RegMemToString(sub.dest).c_str(), RegMemToString(sub.source).c_str());
}

//----------------------------------------------
// SUB immediate from reg/mem
//----------------------------------------------
struct OperationSubImmediateFromRegMem {
	RegMem dest;
	int immediateData;
	ExplicitDataSize dataSize;
};

int OperationSubImmediateFromRegMemParse(unsigned char* buffer, OperationSubImmediateFromRegMem& sub) {
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);
	bool s = (buffer[0] & 0b00000010) >> 1;

	bool isDataWide = (!s) && (isWide);

	sub.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;

	// Calc effective address
	int addressOffset = CalculateRegMem(mod, &buffer[2], isWide, regMem, sub.dest);
	byte* dataLocation = buffer + 2 + addressOffset;

	sub.immediateData = ParseImmediateData(dataLocation, isDataWide);
	return 2 + addressOffset + (isDataWide ? 2 : 1);
}

void OperationSubImmediateFromRegMemPrint(OperationSubImmediateFromRegMem sub) {
	printf("sub %s%s, %d\n", DataSizeToString(sub.dataSize).c_str(), RegMemToString(sub.dest).c_str(), sub.immediateData);
}

//----------------------------------------------
// SUB immediate from accumulator
//----------------------------------------------
struct OperationSubImmediateFromAccumulator {
	int immediateData;
	RegMem dest;
};

int OperationSubImmediateFromAccumulatorParse(unsigned char* buffer, OperationSubImmediateFromAccumulator& sub) {
	bool isWide = buffer[0] & 0b00000001; // explicit wide

	sub.dest.type = MoveMode::REGISTER;
	sub.dest.reg = isWide ? Register::AX : Register::AL;
	sub.immediateData = ParseImmediateData(&buffer[1], isWide);

	return isWide ? 3 : 2;
}

void OperationSubImmediateFromAccumulatorPrint(OperationSubImmediateFromAccumulator sub) {
	printf("sub %s, %d\n", RegMemToString(sub.dest).c_str(), sub.immediateData);
}

//----------------------------------------------
// CMP reg and reg/mem
//----------------------------------------------
struct OperationCompareRegWithRegMem {
	RegMem source;
	RegMem dest;
};

int OperationCompareRegWithRegMemParse(unsigned char* buffer, OperationCompareRegWithRegMem& cmp) {
	bool isWide = buffer[0] & 0b00000001;
	bool destIsReg = buffer[0] & 0b00000010;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	CalculateReg(mod, isWide, reg, destIsReg ? cmp.dest : cmp.source);
	int addressOffset = CalculateRegMem(mod, &buffer[2], isWide, regMem, destIsReg ? cmp.source : cmp.dest);
	return 2 + addressOffset;
}

void OperationCompareRegWithRegMemPrint(OperationCompareRegWithRegMem& cmp) {
	printf("cmp %s, %s\n", RegMemToString(cmp.dest).c_str(), RegMemToString(cmp.source).c_str());
}

//----------------------------------------------
// CMP immediate and reg/mem
//----------------------------------------------
struct OperationCompareImmediateWithRegMem {
	RegMem dest;
	int immediateData;
	ExplicitDataSize dataSize;
};

int OperationCompareImmediateWithRegMemParse(unsigned char* buffer, OperationCompareImmediateWithRegMem& cmp) {
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);
	bool s = (buffer[0] & 0b00000010) >> 1;

	bool isDataWide = (!s) && (isWide);

	cmp.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;

	// Calc effective address
	int addressOffset = CalculateRegMem(mod, &buffer[2], isWide, regMem, cmp.dest);
	byte* dataLocation = buffer + 2 + addressOffset;

	cmp.immediateData = ParseImmediateData(dataLocation, isDataWide);
	return 2 + addressOffset + (isDataWide ? 2 : 1);
}

void OperationCompareImmediateWithRegMemPrint(OperationCompareImmediateWithRegMem cmp) {
	printf("cmp %s%s, %d\n", DataSizeToString(cmp.dataSize).c_str(), RegMemToString(cmp.dest).c_str(), cmp.immediateData);
}

//----------------------------------------------
// CMP immediate and accumulator
//----------------------------------------------
struct OperationCompareImmediateWithAccumulator {
	int immediateData;
	RegMem dest;
};

int OperationCompareImmediateWithAccumulatorParse(unsigned char* buffer, OperationCompareImmediateWithAccumulator& cmp) {
	bool isWide = buffer[0] & 0b00000001; // explicit wide

	cmp.dest.type = MoveMode::REGISTER;
	cmp.dest.reg = isWide ? Register::AX : Register::AL;
	cmp.immediateData = ParseImmediateData(&buffer[1], isWide);

	return isWide ? 3 : 2;
}

void OperationCompareImmediateWithAccumulatorPrint(OperationCompareImmediateWithAccumulator cmp) {
	printf("cmp %s, %d\n", RegMemToString(cmp.dest).c_str(), cmp.immediateData);
}

//----------------------------------------------
// JUMP conditional
//----------------------------------------------
// Smashing all conditional jumps into one operation struct because they're all very similar in encoding
// and I don't want to write 16 different functions for them lol.
struct OperationJumpConditional {
	enum Condition {
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

	int jumpOffset;
	Condition condition;
};

String ConditionToString(OperationJumpConditional::Condition cond) {
	switch (cond) {
	case OperationJumpConditional::JumpOnEqualOrZero: return "je";
	case OperationJumpConditional::JumpOnLess: return "jl";
	case OperationJumpConditional::JumpOnLessOrEqual: return "jle";
	case OperationJumpConditional::JumpOnBelow: return "jb";
	case OperationJumpConditional::JumpOnBelowOrEqual: return "jbe";
	case OperationJumpConditional::JumpOnParity: return "jp";
	case OperationJumpConditional::JumpOnOverflow: return "jo";
	case OperationJumpConditional::JumpOnSign: return "js";
	case OperationJumpConditional::JumpOnNotEqualOrZero: return "jnz";
	case OperationJumpConditional::JumpOnGreaterOrEqual: return "jge";
	case OperationJumpConditional::JumpOnGreater: return "jg";
	case OperationJumpConditional::JumpOnAboveOrEqual: return "jae";
	case OperationJumpConditional::JumpOnAbove: return "ja";
	case OperationJumpConditional::JumpOnNotParity: return "jnp";
	case OperationJumpConditional::JumpOnNotOverflow: return "jno";
	case OperationJumpConditional::JumpOnNotSign: return "jns";
	case OperationJumpConditional::Loop: return "loop";
	case OperationJumpConditional::LoopEqualOrZero: return "loope";
	case OperationJumpConditional::LoopNotEqualOrZero: return "loopne";
	case OperationJumpConditional::JumpOnCXZero: return "jcxz";
	}
	return "";
}

int OperationJumpConditionalParse(unsigned char* buffer, OperationJumpConditional& jump, int myByte) {
	// Jump offsets are relative to the next instruction in bytes
	jump.condition = (OperationJumpConditional::Condition)buffer[0];
	int jumpByteOffset = ParseImmediateData(&buffer[1], false);
	jump.jumpOffset = myByte + jumpByteOffset;
	return 2;
}

void OperationJumpConditionalPrint(OperationJumpConditional jump) {
	printf("%s LABEL_%d\n", ConditionToString(jump.condition).c_str(), jump.jumpOffset);
}

//----------------------------------------------
// Testing stuff
//----------------------------------------------
Buffer LoadBufferFromFile(String filename) {
	FILE* file;
	fopen_s(&file, filename.c_str(), "rb");
	if (file == NULL) {
		printf("Error: Could not open file %s\n", filename.c_str());
		return {};
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Read file into buffer
	int bufferSize = size;
	byte* buffer = new byte[bufferSize];
	fread_s(buffer, bufferSize, sizeof(char), bufferSize, file);
	fclose(file);

	// Return buffer
	return { buffer, bufferSize };
}

struct ParseContext {
	int bp;
	Buffer* buffer;
};;

//----------------------------------------------
// Main
//----------------------------------------------
void Decompile(Buffer& buffer) {
	printf(";Executable read successfully\n");
	printf(";Size: %d\n", buffer.size);
	printf(";Decompiled instructions: \n");
	printf("bits 16\n");

	// Print-decode buffer
	int instructionCount = 0;
	for (int bp = 0; bp < buffer.size;)
	{
		OpCode code = ParseOpCode(buffer.data[bp], buffer.data[bp + 1]);
		int bytes = 0;

		switch (code) {
		case OpCode::MOVE_TOFROM_REGMEM: {
			OperationMoveToFromRegMem move;
			bytes = OperationMoveToFromRegMemParse(&buffer.data[bp], move);
			OperationMoveToFromRegMemPrint(move);
			break;
		}
		case OpCode::MOVE_IMMEDIATE_TO_REGMEM: {
			OperationMoveImmediateToRegMem move;
			bytes = OperationMoveImmediateToRegMemParse(&buffer.data[bp], move);
			OperationMoveImmediateToRegMemPrint(move);
			break;
		}
		case OpCode::MOVE_IMMEDIATE_TO_REG: {
			OperationMoveImmediateToRegister move;
			bytes = OperationMoveImmediateToRegisterParse(&buffer.data[bp], move);
			OperationMoveImmediateToRegisterPrint(move);
			break;
		}
		case OpCode::MOVE_MEMORY_TO_ACCUMULATOR: {
			OperationMoveMemoryToAccumulator move;
			bytes = OperationMoveMemoryToAccumulatorParse(&buffer.data[bp], move);
			OperationMoveMemoryToAccumulatorPrint(move);
			break;
		}
		case OpCode::MOVE_ACCUMULATOR_TO_MEMORY: {
			OperationMoveAccumulatorToMemory move;
			bytes = OperationMoveAccumulatorToMemoryParse(&buffer.data[bp], move);
			OperationMoveAccumulatorToMemoryPrint(move);
			break;
		}
		case OpCode::MOVE_REGMEM_TO_SEGMENT: {
			OperationMoveRegMemToSegmentRegister move;
			bytes = OperationMoveRegMemToSegmentRegisterParse(&buffer.data[bp], move);
			OperationMoveRegMemToSegmentRegisterPrint(move);
			break;
		}
		case OpCode::MOVE_SEGMENT_TO_REGMEM: {
			OperationMoveSegmentRegisterToRegMem move;
			bytes = OperationMoveSegmentRegisterToRegMemParse(&buffer.data[bp], move);
			OperationMoveSegmentRegisterToRegMemPrint(move);
			break;
		}
		case OpCode::ADD_IMMEDIATE_TO_ACCUMULATOR: {
			OperationAddImmediateToAccumulator add;
			bytes = OperationAddImmediateToAccumulatorParse(&buffer.data[bp], add);
			OperationAddImmediateToAccumulatorPrint(add);
			break;
		}
		case OpCode::ADD_TOFROM_REGMEM: {
			OperationAddToFromRegMem add;
			bytes = OperationAddToFromRegMemParse(&buffer.data[bp], add);
			OperationAddToFromRegMemPrint(add);
			break;
		}
		case OpCode::ADD_IMMEDIATE_TO_REGMEM: {
			OperationAddImmediateToRegMem add;
			bytes = OperationAddImmediateToRegMemParse(&buffer.data[bp], add);
			OperationAddImmediateToRegMemPrint(add);
			break;
		}
		case OpCode::SUB_TOFROM_REGMEM: {
			OperationSubToFromRegMem sub;
			bytes = OperationSubToFromRegMemParse(&buffer.data[bp], sub);
			OperationSubToFromRegMemPrint(sub);
			break;
		}
		case OpCode::SUB_IMMEDIATE_TO_REGMEM: {
			OperationSubImmediateFromRegMem sub;
			bytes = OperationSubImmediateFromRegMemParse(&buffer.data[bp], sub);
			OperationSubImmediateFromRegMemPrint(sub);
			break;
		}
		case OpCode::SUB_IMMEDIATE_TO_ACCUMULATOR: {
			OperationSubImmediateFromAccumulator sub;
			bytes = OperationSubImmediateFromAccumulatorParse(&buffer.data[bp], sub);
			OperationSubImmediateFromAccumulatorPrint(sub);
			break;
		}
		case OpCode::CMP_REG_WITH_REGMEM: {
			OperationCompareRegWithRegMem cmp;
			bytes = OperationCompareRegWithRegMemParse(&buffer.data[bp], cmp);
			OperationCompareRegWithRegMemPrint(cmp);
			break;
		}
		case OpCode::CMP_IMMEDIATE_WITH_REGMEM: {
			OperationCompareImmediateWithRegMem cmp;
			bytes = OperationCompareImmediateWithRegMemParse(&buffer.data[bp], cmp);
			OperationCompareImmediateWithRegMemPrint(cmp);
			break;
		}
		case OpCode::CMP_IMMEDIATE_WITH_ACCUMULATOR: {
			OperationCompareImmediateWithAccumulator cmp;
			bytes = OperationCompareImmediateWithAccumulatorParse(&buffer.data[bp], cmp);
			OperationCompareImmediateWithAccumulatorPrint(cmp);
			break;
		}
		case OpCode::JUMP_ON_EQUAL_OR_ZERO:
		case OpCode::JUMP_ON_LESS:
		case OpCode::JUMP_ON_LESS_OR_EQUAL:
		case OpCode::JUMP_ON_BELOW:
		case OpCode::JUMP_ON_BELOW_OR_EQUAL:
		case OpCode::JUMP_ON_PARITY:
		case OpCode::JUMP_ON_OVERFLOW:
		case OpCode::JUMP_ON_SIGN:
		case OpCode::JUMP_ON_NOT_EQUAL_OR_ZERO:
		case OpCode::JUMP_ON_GREATER_OR_EQUAL:
		case OpCode::JUMP_ON_GREATER:
		case OpCode::JUMP_ON_ABOVE_OR_EQUAL:
		case OpCode::JUMP_ON_ABOVE:
		case OpCode::JUMP_ON_NOT_PARITY:
		case OpCode::JUMP_ON_NOT_OVERFLOW:
		case OpCode::JUMP_ON_NOT_SIGN:
		case OpCode::LOOP_CX_TIMES:
		case OpCode::LOOP_WHILE_ZERO:
		case OpCode::LOOP_WHILE_NOT_ZERO:
		case OpCode::JUMP_ON_CX_ZERO:
		{
			OperationJumpConditional jump;
			bytes = OperationJumpConditionalParse(&buffer.data[bp], jump, bp);
			OperationJumpConditionalPrint(jump);
			break;
		}
		default: {
			printf("ERROR: Unhandled opcode 0x%x\n", buffer.data[bp]);
			return;
		}
		}

		bp += bytes;
		instructionCount++;
	}
}

}
#include "Decoder.h"
#include "String.h"
#include "Types.h"
#include <stdio.h>
#include "StringifyTypes.h"

namespace Decoder {
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
		MOVE,
		ADD,
		SUB,
		COMPARE,
		JUMP
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
		int instructionIndex;
	};

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

void CalculateReg(char mod, bool isWide, char reg, Operand& out) {
	out.type = Operand::Type::REGISTER;
	out.reg = RegisterParse(reg, isWide);
}

Register SRToSegmentRegister(char sr) {
	switch (sr) {
	case 0b00: return Register::ES;
	case 0b01: return Register::CS;
	case 0b10: return Register::SS;
	case 0b11: return Register::DS;
	}
}

int CalculateOperandFromRegMem(char mod, byte* bufferPossibleMemStart, bool isWide, char regMem, Operand& out) {
	// Account for rm being 110, which is a special case with 16-bit displacement
	bool directAddress = (mod == 0b00) && (regMem == 0b110);
	if (directAddress) {
		out.type = Operand::Type::MEMORY_LOC;
		out.mem.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;
		out.mem.memoryOffset = (unsigned short)((bufferPossibleMemStart[1] << 8) | bufferPossibleMemStart[0]);
		return 2;
	}

	switch (mod) {
	case 0b00: {
		// No displacement, to memory
		out.type = Operand::Type::MEMORY_LOC;
		out.mem.effectiveAddress = RMToEffectiveAddress(regMem);
		out.mem.memoryOffset = 0;
		return 0;
	}
	case 0b01: {
		// 8-bit displacement, to memory
		out.type = Operand::Type::MEMORY_LOC;
		out.mem.effectiveAddress = RMToEffectiveAddress(regMem);
		out.mem.memoryOffset = (signed char)bufferPossibleMemStart[0];
		return 1;
	}
	case 0b10: {
		// 16-bit displacement, to memory
		out.type = Operand::Type::MEMORY_LOC;
		out.mem.effectiveAddress = RMToEffectiveAddress(regMem);
		out.mem.memoryOffset = (signed short)((bufferPossibleMemStart[1] << 8) | bufferPossibleMemStart[0]);
		return 2;
	}
	case 0b11: {
		// No displacement, to register
		out.type = Operand::Type::REGISTER;
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
int CalculateSourceAndDestFromMode(char mod, byte* bufferPossibleMemStart, bool destinationIsRegister, bool isWide, char reg, char regMem, Operand& source, Operand& dest) {
	Operand& slotReg = destinationIsRegister ? dest : source;
	CalculateReg(mod, isWide, reg, slotReg);

	Operand& slotMem = destinationIsRegister ? source : dest;
	return CalculateOperandFromRegMem(mod, bufferPossibleMemStart, isWide, regMem, slotMem);
}

//----------------------------------------------
// MOVE Register/memory to/from register
//----------------------------------------------
int OperationMoveToFromRegMemParse(unsigned char* buffer, InstructionMove& move) {
	bool destIsReg = buffer[0] & 0b00000010;
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	int bytesToShift = CalculateSourceAndDestFromMode(mod, &buffer[2], destIsReg, isWide, reg, regMem, move.source, move.dest);
	return 2 + bytesToShift;
}

//----------------------------------------------
// MOVE immediate to register/memory
//----------------------------------------------
int OperationMoveImmediateToRegMemParse(byte* buffer, InstructionMove& move) {
	bool isDataWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char regMem = (buffer[1] & 0b00000111);

	move.source.dataSize = isDataWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;

	// Calc effective address
	int byteOffset = CalculateOperandFromRegMem(mod, &buffer[2], isDataWide, regMem, move.dest);
	byte* dataLocation = buffer + 2 + byteOffset;
	move.source.type = Operand::Type::IMMEDIATE;
	move.source.immediate = isDataWide ? ((dataLocation[1] << 8) | dataLocation[0]) : dataLocation[0];
	return 2 + byteOffset + (isDataWide ? 2 : 1);
}

//----------------------------------------------
// MOVE immediate to register
//----------------------------------------------
int OperationMoveImmediateToRegisterParse(unsigned char* buffer, InstructionMove& move) {
	bool isWide = buffer[0] & 0b00001000;
	char reg = (buffer[0] & 0b00000111);
	move.dest.type = Operand::Type::REGISTER;
	move.dest.reg = RegisterParse(reg, isWide);
	move.source.type = Operand::Type::IMMEDIATE;
	move.source.immediate = ParseImmediateData(&buffer[1], isWide);
	return isWide ? 3 : 2;
}

//----------------------------------------------
// MOVE memory to accumulator
//----------------------------------------------
int OperationMoveMemoryToAccumulatorParse(unsigned char* buffer, InstructionMove& move) {
	bool isWide = buffer[0] & 0b00000001;
	move.dest.type = Operand::Type::REGISTER;
	move.dest.reg = isWide ? Register::AX : Register::AL;

	move.source.type = Operand::Type::MEMORY_LOC;
	move.source.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;
	move.source.mem.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;

	if (isWide) {
		move.source.mem.memoryOffset = (buffer[2] << 8) | buffer[1];
		return 3;
	}
	else {
		move.source.mem.memoryOffset = buffer[1];
		return 2;
	}
}

//----------------------------------------------
// MOVE accumulator to memory
//----------------------------------------------
int OperationMoveAccumulatorToMemoryParse(unsigned char* buffer, InstructionMove& move) {
	bool isWide = buffer[0] & 0b00000001;
	
	move.source.type = Operand::Type::REGISTER;
	move.source.reg = Register::AX;

	move.dest.type = Operand::Type::MEMORY_LOC;
	move.dest.mem.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;

	if (isWide) {
		move.dest.mem.memoryOffset = (buffer[2] << 8) | buffer[1];
		return 3;
	}
	else {
		move.dest.mem.memoryOffset = buffer[1];
		return 2;
	}
}

//----------------------------------------------
// MOVE RegMem to segment register
//----------------------------------------------
int OperationMoveRegMemToSegmentRegisterParse(unsigned char* buffer, InstructionMove& move) {
	bool isWide = true;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char sr = (buffer[1] & 0b00011000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	move.dest.type = Operand::Type::REGISTER;
	move.dest.reg = SRToSegmentRegister(sr);

	// Calc effective address
	int byteOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, move.source);
	return 2 + byteOffset;
}

//----------------------------------------------
// MOVE segment register to RegMem
//----------------------------------------------
int OperationMoveSegmentRegisterToRegMemParse(unsigned char* buffer, InstructionMove& move) {
	bool isWide = true;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char sr = (buffer[1] & 0b00011000) >> 3;
	char regMem = (buffer[1] & 0b00000111);
	move.source.type = Operand::Type::REGISTER;
	move.source.reg = SRToSegmentRegister(sr);

	// Calc effective address
	int byteOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, move.dest);
	return 2 + byteOffset;
}

//----------------------------------------------
// ADD immediate to register/memory
//----------------------------------------------
int OperationAddImmediateToRegMemParse(unsigned char* buffer, InstructionAdd& add) {
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char regMem = (buffer[1] & 0b00000111);
	bool s = (buffer[0] & 0b00000010) >> 1;

	bool isDataWide = (!s) && (isWide);

	add.source.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;

	// Calc effective address
	int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, add.dest);
	byte* dataLocation = buffer + 2 + addressOffset;

	add.source.type = Operand::Type::IMMEDIATE;
	add.source.immediate = ParseImmediateData(dataLocation, isDataWide);
	return 2 + addressOffset + (isDataWide ? 2 : 1);
}

//----------------------------------------------
// ADD to from register memory
//----------------------------------------------
int OperationAddToFromRegMemParse(unsigned char* buffer, InstructionAdd& add) {
	bool isWide = buffer[0] & 0b00000001;
	bool destIsReg = buffer[0] & 0b00000010;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	CalculateReg(mod, isWide, reg, destIsReg ? add.dest : add.source);
	int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, destIsReg ? add.source : add.dest);
	return 2 + addressOffset;
}

//----------------------------------------------
// ADD immediate to accumulator
//----------------------------------------------
int OperationAddImmediateToAccumulatorParse(unsigned char* buffer, InstructionAdd& add) {
	bool isWide = buffer[0] & 0b00000001; // explicit wide

	add.dest.type = Operand::Type::REGISTER;
	add.dest.reg = isWide ? Register::AX : Register::AL;

	add.source.type = Operand::Type::IMMEDIATE;
	add.source.immediate = ParseImmediateData(&buffer[1], isWide);

	return isWide ? 3 : 2;
}

//----------------------------------------------
// SUB to from register memory
//----------------------------------------------
int OperationSubToFromRegMemParse(unsigned char* buffer, InstructionSub& sub) {
	bool isWide = buffer[0] & 0b00000001;
	bool destIsReg = buffer[0] & 0b00000010;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	CalculateReg(mod, isWide, reg, destIsReg ? sub.dest : sub.source);
	int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, destIsReg ? sub.source : sub.dest);
	return 2 + addressOffset;
}

//----------------------------------------------
// SUB immediate from reg/mem
//----------------------------------------------
int OperationSubImmediateFromRegMemParse(unsigned char* buffer, InstructionSub& sub) {
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);
	bool s = (buffer[0] & 0b00000010) >> 1;

	bool isDataWide = (!s) && (isWide);

	sub.source.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;

	// Calc effective address
	int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, sub.dest);
	byte* dataLocation = buffer + 2 + addressOffset;

	sub.source.type = Operand::Type::IMMEDIATE;
	sub.source.immediate = ParseImmediateData(dataLocation, isDataWide);
	return 2 + addressOffset + (isDataWide ? 2 : 1);
}

//----------------------------------------------
// SUB immediate from accumulator
//----------------------------------------------
int OperationSubImmediateFromAccumulatorParse(unsigned char* buffer, InstructionSub& sub) {
	bool isWide = buffer[0] & 0b00000001; // explicit wide

	sub.dest.type = Operand::Type::REGISTER;
	sub.dest.reg = isWide ? Register::AX : Register::AL;

	sub.source.type = Operand::Type::IMMEDIATE;
	sub.source.immediate = ParseImmediateData(&buffer[1], isWide);

	return isWide ? 3 : 2;
}

//----------------------------------------------
// CMP reg and reg/mem
//----------------------------------------------
int OperationCompareRegWithRegMemParse(unsigned char* buffer, InstructionCompare& cmp) {
	bool isWide = buffer[0] & 0b00000001;
	bool destIsReg = buffer[0] & 0b00000010;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	CalculateReg(mod, isWide, reg, destIsReg ? cmp.dest : cmp.source);
	int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, destIsReg ? cmp.source : cmp.dest);
	return 2 + addressOffset;
}

//----------------------------------------------
// CMP immediate and reg/mem
//----------------------------------------------

int OperationCompareImmediateWithRegMemParse(unsigned char* buffer, InstructionCompare& cmp) {
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);
	bool s = (buffer[0] & 0b00000010) >> 1;

	bool isDataWide = (!s) && (isWide);

	cmp.source.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;

	// Calc effective address
	int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, cmp.dest);
	byte* dataLocation = buffer + 2 + addressOffset;

	cmp.source.type = Operand::Type::IMMEDIATE;
	cmp.source.immediate = ParseImmediateData(dataLocation, isDataWide);
	return 2 + addressOffset + (isDataWide ? 2 : 1);
}

//----------------------------------------------
// CMP immediate and accumulator
//----------------------------------------------
int OperationCompareImmediateWithAccumulatorParse(unsigned char* buffer, InstructionCompare& cmp) {
	bool isWide = buffer[0] & 0b00000001; // explicit wide

	cmp.dest.type = Operand::Type::REGISTER;
	cmp.dest.reg = isWide ? Register::AX : Register::AL;
	cmp.source.type = Operand::Type::IMMEDIATE;
	cmp.source.immediate = ParseImmediateData(&buffer[1], isWide);

	return isWide ? 3 : 2;
}

//----------------------------------------------
// JUMP conditional
//----------------------------------------------
// Smashing all conditional jumps into one operation struct because they're all very similar in encoding
// and I don't want to write 16 different functions for them lol.

String ConditionToString(InstructionJump::Condition cond) {
	switch (cond) {
	case InstructionJump::JumpOnEqualOrZero: return "je";
	case InstructionJump::JumpOnLess: return "jl";
	case InstructionJump::JumpOnLessOrEqual: return "jle";
	case InstructionJump::JumpOnBelow: return "jb";
	case InstructionJump::JumpOnBelowOrEqual: return "jbe";
	case InstructionJump::JumpOnParity: return "jp";
	case InstructionJump::JumpOnOverflow: return "jo";
	case InstructionJump::JumpOnSign: return "js";
	case InstructionJump::JumpOnNotEqualOrZero: return "jnz";
	case InstructionJump::JumpOnGreaterOrEqual: return "jge";
	case InstructionJump::JumpOnGreater: return "jg";
	case InstructionJump::JumpOnAboveOrEqual: return "jae";
	case InstructionJump::JumpOnAbove: return "ja";
	case InstructionJump::JumpOnNotParity: return "jnp";
	case InstructionJump::JumpOnNotOverflow: return "jno";
	case InstructionJump::JumpOnNotSign: return "jns";
	case InstructionJump::Loop: return "loop";
	case InstructionJump::LoopEqualOrZero: return "loope";
	case InstructionJump::LoopNotEqualOrZero: return "loopne";
	case InstructionJump::JumpOnCXZero: return "jcxz";
	}
	return "";
}

int OperationJumpConditionalParse(unsigned char* buffer, InstructionJump& jump, int myByte) {
	// Jump offsets are relative to the next instruction in bytes
	jump.condition = (InstructionJump::Condition)buffer[0];
	int jumpByteOffset = ParseImmediateData(&buffer[1], false);

	// This gives the byte, not the instruction index
	jump.instructionIndex = myByte + jumpByteOffset;
	return 2;
}

//----------------------------------------------
// Operation printing
//----------------------------------------------
String OperandToString(Operand const& o) {
	switch (o.type) {
	case Operand::Type::REGISTER: return RegisterToString(o.reg);
	case Operand::Type::MEMORY_LOC: return EffectiveAddressWithOffsetToString(o.mem.effectiveAddress, o.mem.memoryOffset);
	case Operand::Type::IMMEDIATE: {
		switch (o.dataSize) {
		case ExplicitDataSize::WORD: return String::Format("word %i", o.immediate);
		case ExplicitDataSize::BYTE: return String::Format("byte %i", o.immediate);
		default: return String::Format("%i", o.immediate);
		}
	}
	case Operand::Type::NONE: 
		printf("OPERATION HAS NO TYPE\n");
		return "";
	}
}

void PrintMove(InstructionMove const& move) {
	printf("mov %s, %s\n", OperandToString(move.dest).c_str(), OperandToString(move.source).c_str());
}

void PrintAdd(InstructionAdd const& add) {
	printf("add %s, %s\n", OperandToString(add.dest).c_str(), OperandToString(add.source).c_str());
}

void PrintSub(InstructionSub const& sub) {
	printf("sub %s, %s\n", OperandToString(sub.dest).c_str(), OperandToString(sub.source).c_str());
}

void PrintCompare(InstructionCompare const& cmp) {
	printf("cmp %s, %s\n", OperandToString(cmp.dest).c_str(), OperandToString(cmp.source).c_str());
}

void PrintJump(InstructionJump const& jump) {
	printf("%s LABEL_%i\n", ConditionToString(jump.condition).c_str(), jump.instructionIndex);
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

struct DecodedBuffer {
	void* data;
	int* instructionIndexToByte;
	InstructionType* instructionIndexToType;
	int size;
};

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
			InstructionMove move;
			bytes = OperationMoveToFromRegMemParse(&buffer.data[bp], move);
			PrintMove(move);
			break;
		}
		case OpCode::MOVE_IMMEDIATE_TO_REGMEM: {
			InstructionMove move;
			bytes = OperationMoveImmediateToRegMemParse(&buffer.data[bp], move);
			PrintMove(move);
			break;
		}
		case OpCode::MOVE_IMMEDIATE_TO_REG: {
			InstructionMove move;
			bytes = OperationMoveImmediateToRegisterParse(&buffer.data[bp], move);
			PrintMove(move);
			break;
		}
		case OpCode::MOVE_MEMORY_TO_ACCUMULATOR: {
			InstructionMove move;
			bytes = OperationMoveMemoryToAccumulatorParse(&buffer.data[bp], move);
			PrintMove(move);
			break;
		}
		case OpCode::MOVE_ACCUMULATOR_TO_MEMORY: {
			InstructionMove move;
			bytes = OperationMoveAccumulatorToMemoryParse(&buffer.data[bp], move);
			PrintMove(move);
			break;
		}
		case OpCode::MOVE_REGMEM_TO_SEGMENT: {
			InstructionMove move;
			bytes = OperationMoveRegMemToSegmentRegisterParse(&buffer.data[bp], move);
			PrintMove(move);
			break;
		}
		case OpCode::MOVE_SEGMENT_TO_REGMEM: {
			InstructionMove move;
			bytes = OperationMoveSegmentRegisterToRegMemParse(&buffer.data[bp], move);
			PrintMove(move);
			break;
		}
		case OpCode::ADD_IMMEDIATE_TO_ACCUMULATOR: {
			InstructionAdd add;
			bytes = OperationAddImmediateToAccumulatorParse(&buffer.data[bp], add);
			PrintAdd(add);
			break;
		}
		case OpCode::ADD_TOFROM_REGMEM: {
			InstructionAdd add;
			bytes = OperationAddToFromRegMemParse(&buffer.data[bp], add);
			PrintAdd(add);
			break;
		}
		case OpCode::ADD_IMMEDIATE_TO_REGMEM: {
			InstructionAdd add;
			bytes = OperationAddImmediateToRegMemParse(&buffer.data[bp], add);
			PrintAdd(add);
			break;
		}
		case OpCode::SUB_TOFROM_REGMEM: {
			InstructionSub sub;
			bytes = OperationSubToFromRegMemParse(&buffer.data[bp], sub);
			PrintSub(sub);
			break;
		}
		case OpCode::SUB_IMMEDIATE_TO_REGMEM: {
			InstructionSub sub;
			bytes = OperationSubImmediateFromRegMemParse(&buffer.data[bp], sub);
			PrintSub(sub);
			break;
		}
		case OpCode::SUB_IMMEDIATE_TO_ACCUMULATOR: {
			InstructionSub sub;
			bytes = OperationSubImmediateFromAccumulatorParse(&buffer.data[bp], sub);
			PrintSub(sub);
			break;
		}
		case OpCode::CMP_REG_WITH_REGMEM: {
			InstructionCompare cmp;
			bytes = OperationCompareRegWithRegMemParse(&buffer.data[bp], cmp);
			PrintCompare(cmp);
			break;
		}
		case OpCode::CMP_IMMEDIATE_WITH_REGMEM: {
			InstructionCompare cmp;
			bytes = OperationCompareImmediateWithRegMemParse(&buffer.data[bp], cmp);
			PrintCompare(cmp);
			break;
		}
		case OpCode::CMP_IMMEDIATE_WITH_ACCUMULATOR: {
			InstructionCompare cmp;
			bytes = OperationCompareImmediateWithAccumulatorParse(&buffer.data[bp], cmp);
			PrintCompare(cmp);
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
			InstructionJump jump;
			bytes = OperationJumpConditionalParse(&buffer.data[bp], jump, bp);
			PrintJump(jump);
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
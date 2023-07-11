// Creator: Daniel B.
// Date: 2023-07-06
// This file was made for Casey Muratori's 8086 homework segment for the Computer Enhance course
// on performance programming. For the sake of keeping things straight forward and easy to debug and extend, 
// this file is written in a bare-bones and verbose C style.
// All magic numbers used are from the Intel 8086 manual
// which can be found here: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf


/*
--------------------------------------------------
TODO:
--------------------------------------------------
[x] Fix weird register address bug
[ ] Implement all required opcodes
[ ] Convert all operations to classes
[ ] Use a "parsing context" rather than rough return logic
[ ] Figure out way to rid of huge switch statement at bottom
*/

#include <stdio.h>
#include "String.h"

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

Register RegisterParse(char reg, bool w) {
	int index = (int)reg + (w ? 8 : 0);
	return (Register)index;
}


Register SRToSegmentRegister(char sr) {
	switch (sr) {
	case 0b00: return Register::ES;
	case 0b01: return Register::CS;
	case 0b10: return Register::SS;
	case 0b11: return Register::DS;
	}
}

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

EffectiveAddress RMToEffectiveAddress(char rm) {
	return (EffectiveAddress)rm;
}

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

struct ParseContext {
	int bp;
	byte* buffer;
};

struct InstructionDef {
	OpCode inst;
	int decodeFlags;
	byte opCodeMask;
	byte opCodeValue;
	int optionalRequiredReg;

	void BuildMask(int numBitsForMask) {
		opCodeMask = 0;
		for (int i = 0; i <= numBitsForMask; i++) {
			opCodeMask |= (1 << (8 - i));
		}
	}

	InstructionDef(OpCode inst, int numBitsForMask, byte opCodeValue, int decodeFlags = 0, int optionalRequiredReg = -1)
		: inst(inst), opCodeValue(opCodeValue), optionalRequiredReg(optionalRequiredReg), decodeFlags(decodeFlags)
	{
		BuildMask(numBitsForMask);
	}
};

InstructionDef instructionDefs[] = {
	InstructionDef(OpCode::MOVE_TOFROM_REGMEM, 6, 0b10001000, OpFlag::MOD | OpFlag::REG | OpFlag::REGMEM | OpFlag::W | OpFlag::D),
	InstructionDef(OpCode::MOVE_IMMEDIATE_TO_REGMEM, 7, 0b11000110, OpFlag::MOD | OpFlag::REGMEM | OpFlag::W | OpFlag::HAS_DATA),
	InstructionDef(OpCode::MOVE_IMMEDIATE_TO_REG, 4, 0b10110000, OpFlag::MOD | OpFlag::ONE_BYTE_OP | OpFlag::REG_IN_FIRSTBYTE | OpFlag::W_IN_FIRSTBYTE | OpFlag::HAS_DATA),
	InstructionDef(OpCode::MOVE_MEMORY_TO_ACCUMULATOR, 7, 0b10100000, OpFlag::MOD | OpFlag::W | OpFlag::ONE_BYTE_OP | OpFlag::HAS_ADDR),
	InstructionDef(OpCode::MOVE_ACCUMULATOR_TO_MEMORY, 7, 0b10100010, OpFlag::MOD | OpFlag::W | OpFlag::ONE_BYTE_OP | OpFlag::HAS_ADDR),
	//InstructionDef(OpCode::MOVE_REGMEM_TO_SEGMENT, 8, 0b10001110, OpFlag::MOD),
	//InstructionDef(OpCode::MOVE_SEGMENT_TO_REGMEM, 8, 0b10001100, OpFlag::MOD),
	//// Add
	//InstructionDef(OpCode::ADD_TOFROM_REGMEM, 6, 0b00000000, OpFlag::MOD),
	//InstructionDef(OpCode::ADD_IMMEDIATE_TO_REGMEM, 6, 0b10000000, 0b000, OpFlag::MOD),
	//InstructionDef(OpCode::ADD_IMMEDIATE_TO_ACCUMULATOR, 7, 0b00000100, OpFlag::MOD),
	//// Sub
	//InstructionDef(OpCode::SUB_TOFROM_REGMEM, 6, 0b00101000, OpFlag::MOD),
	//InstructionDef(OpCode::SUB_IMMEDIATE_TO_REGMEM, 6, 0b10000000, 0b101, OpFlag::MOD),
	//InstructionDef(OpCode::SUB_IMMEDIATE_TO_ACCUMULATOR, 7, 0b00101100, OpFlag::MOD),
	//// Compare
	//InstructionDef(OpCode::CMP_REG_WITH_REGMEM, 6, 0b00111000, OpFlag::MOD),
	//InstructionDef(OpCode::CMP_IMMEDIATE_WITH_REGMEM, 6, 0b10000000, 0b111, OpFlag::MOD),
	//InstructionDef(OpCode::CMP_IMMEDIATE_WITH_ACCUMULATOR, 7, 0b00111100, OpFlag::MOD),
	//// Jump
	//InstructionDef(OpCode::JUMP_ON_EQUAL_OR_ZERO,		8, 0b01110100, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_LESS,				8, 0b01111100, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_LESS_OR_EQUAL,		8, 0b01111110, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_BELOW,				8, 0b01110010, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_BELOW_OR_EQUAL,		8, 0b01110110, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_PARITY,				8, 0b01111010, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_OVERFLOW,			8, 0b01110000, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_SIGN,				8, 0b01111000, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_NOT_EQUAL_OR_ZERO,	8, 0b01110101, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_GREATER_OR_EQUAL,	8, 0b01111101, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_GREATER,				8, 0b01111111, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_ABOVE_OR_EQUAL,		8, 0b01110011, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_ABOVE,				8, 0b01110111, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_NOT_PARITY,			8, 0b01111011, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_NOT_OVERFLOW,		8, 0b01110001, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_NOT_SIGN,			8, 0b01111001, OpFlag::MOD),
	//InstructionDef(OpCode::LOOP_CX_TIMES,				8, 0b11100010, OpFlag::MOD),
	//InstructionDef(OpCode::LOOP_WHILE_ZERO,				8, 0b11100011, OpFlag::MOD),
	//InstructionDef(OpCode::LOOP_WHILE_NOT_ZERO,			8, 0b11100001, OpFlag::MOD),
	//InstructionDef(OpCode::JUMP_ON_CX_ZERO,				8, 0b11100000, OpFlag::MOD),
};

bool ParseInstruction(ParseContext& ctx, Instruction& instruction) {
	byte const* buffer = &ctx.buffer[ctx.bp];
	byte opCode = buffer[0];
	int flags = 0;

	// Scan through definitions to find a match for opcode
	bool defFound = false;
	int mappingSize = sizeof(instructionDefs) / sizeof(InstructionDef);
	for (int i = 0; i < mappingSize; i++) {
		InstructionDef mapping = instructionDefs[i];
		if ((opCode & mapping.opCodeMask) == mapping.opCodeValue) {
			// Check that the optional reg is correct
			if ((mapping.optionalRequiredReg != -1)) {
				byte parsedReg = (buffer[1] & 0b00111000) >> 3;
				if (parsedReg != mapping.optionalRequiredReg) {
					continue;
				}
			}
			instruction.inst = instructionDefs[i].inst;
			flags = instructionDefs[i].decodeFlags;
			defFound = true;
		}
	}

	if (!defFound) {
		printf("Couldn't find InstructionDef for this instruction: 0x%x\n", opCode);
		return false;
	}

	// Parse full instruction using flags
	int sizeOfInstruction = 1;

	int dataSign = 0;
	int disp = 0;
	bool destIsReg = false;
	bool isWide = false;
	int displacementSize = 0;
	MoveMode moveMode = MoveMode::Register;

	if (flags & OpFlag::ONE_BYTE_OP) {
		sizeOfInstruction = 1;
	}
	else {
		sizeOfInstruction = 2;
	}

	if (flags & OpFlag::MOD) {
		int mod = (buffer[1] & 0b11000000) >> 6;
		// everything with mod has a displacement
		switch (mod) {
		case 0b00: displacementSize = 0; moveMode = MoveMode::Memory; break;
		case 0b01: displacementSize = 1; moveMode = MoveMode::Memory; break;
		case 0b10: displacementSize = 2; moveMode = MoveMode::Memory; break;
		case 0b11: displacementSize = 0; moveMode = MoveMode::Register; break;
		}
		// Calculate mem displacement
		if (displacementSize == 1) {
			disp = buffer[2];
		}
		else if (displacementSize == 2) {
			disp = buffer[3] << 8 | buffer[2];
		}
		sizeOfInstruction += displacementSize;
	}

	if (flags & OpFlag::D) {
		destIsReg = (buffer[0] & 0b00000010) != 0;
	}

	if (flags & OpFlag::W) {
		isWide = (buffer[0] & 0b00000001) != 0;
	}

	if (flags & OpFlag::W_IN_FIRSTBYTE) {
		isWide = (buffer[0] & 0b00001000) != 0;
	}

	if (flags & OpFlag::REG) {
		int regInt = (buffer[1] & 0b00111000) >> 3;
		RegMem& reg = destIsReg ? instruction.dest : instruction.source;
		reg.reg = RegisterParse(regInt, isWide);
		reg.type = MoveMode::Register;
	}

	if (flags & OpFlag::REGMEM) {
		int regMemInt = (buffer[1] & 0b00000111);
		RegMem& regMem = destIsReg ? instruction.source : instruction.dest;
		if (moveMode == MoveMode::Register) {
			regMem.type = MoveMode::Register;
			regMem.reg = RegisterParse(regMemInt, isWide);
		}
		else {
			regMem.memoryOffset = disp;
			regMem.type = MoveMode::Memory;
			regMem.effectiveAddress = RMToEffectiveAddress(regMemInt);
		}
	}

	if (flags & OpFlag::S) {
		dataSign = (buffer[0] & 0b00000010);
	}

	if (flags & OpFlag::REG_IN_FIRSTBYTE) {
		int regInt = (buffer[0] & 0b00000111);
		RegMem& reg = destIsReg ? instruction.dest : instruction.source;
		reg.reg = RegisterParse(regInt, isWide);
		reg.type = MoveMode::Register;
	}

	if (flags & OpFlag::HAS_IP8) {
		instruction.relativeJump = buffer[1];
	}

	if (flags & OpFlag::HAS_DATA) {
		if (isWide) {
			instruction.dataSize = ExplicitDataSize::Word;
			instruction.immediate = (word)((buffer[2 + displacementSize] << 8) | buffer[1 + displacementSize]);
		}
		else {
			instruction.dataSize = ExplicitDataSize::Byte;
			instruction.immediate = (word)(buffer[1 + displacementSize]);
		}
	}

	if (flags & OpFlag::HAS_ADDR) {
		// This is an exact 16-bit address
		instruction.immediate = (word)((buffer[2 + displacementSize] << 8) | buffer[1 + displacementSize]);
		sizeOfInstruction += 2;
	}

	// Return the number of bytes read
	ctx.bp += sizeOfInstruction;

	return true;
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

//----------------------------------------------
// Stringify functions
//----------------------------------------------

String RegisterToString(Register reg) {
	static String names[] = {
		"al", "cl", "dl", "bl",
		"ah", "ch", "dh", "bh",
		"ax", "cx", "dx", "bx",
		"sp", "bp", "si", "di",
		"cs", "ds", "ss", "es",
	};
	String strReg = names[(int)reg];
	return strReg;
}

String EffectiveAddressToString(EffectiveAddress addr) {
	static String effectiveAddresses[] = {
		"[bx+si]",
		"[bx+di]",
		"[bp+si]",
		"[bp+di]",
		"[si]",
		"[di]",
		"[bp]",
		"[bx]",
		"[DIRECT ADDRESS]",
	};
	return effectiveAddresses[(int)addr];
}

String EffectiveAddressWithOffsetToString(EffectiveAddress addr, int offset) {
	if (offset == 0) {
		return EffectiveAddressToString(addr);
	}

	static String effectiveAddresses[] = {
		"[bx+si%s%i]",
		"[bx+di%s%i]",
		"[bp+si%s%i]",
		"[bp+di%s%i]",
		"[si%s%i]",
		"[di%s%i]",
		"[bp]",
		"[bx%s%i]",
		"[%i]" // Direct address
	};

	if (offset < 0) {
		return String::Format(effectiveAddresses[(int)addr], "-", -offset);
	}

	if (addr == EffectiveAddress::DIRECT_ADDRESS) {
		return String::Format(effectiveAddresses[(int)addr], offset);
	}

	return String::Format(effectiveAddresses[(int)addr], "+", offset);
}

String DataSizeToString(ExplicitDataSize s) {
	if (s == ExplicitDataSize::Byte) {
		return "byte ";
	}
	else {
		return "word ";
	}
}

String RegMemToString(RegMem& r) {
	if (r.type == MoveMode::Register) {
		return RegisterToString(r.reg);
	}
	else {
		return EffectiveAddressWithOffsetToString(r.effectiveAddress, r.memoryOffset);
	}
}

//----------------------------------------------
// Main
//----------------------------------------------
void Decompile(Buffer& buffer) {
	printf(";Executable read successfully\n");
	printf(";Size: %d\n", buffer.size);
	printf(";Decompiled instructions: \n");
	printf("bits 16\n");

	// Print-decode buffer
	ParseContext ctx;
	ctx.bp = 0;
	ctx.buffer = buffer.data;
	while(ctx.bp < buffer.size)
	{
		Instruction inst{};
		bool success = ParseInstruction(ctx, inst);
		if (!success) {
			return;
		}
		else {
			printf("successfully parsed instruction: mov %s, %s\n", RegMemToString(inst.dest).c_str(), RegMemToString(inst.source).c_str());
		}
	}
}

int main(int argc, char* argv[]) {
	// Parse command line arguments
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	Buffer buffer = LoadBufferFromFile(argv[1]);
	Decompile(buffer);
	return 0;
}






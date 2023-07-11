#include "DecoderAlternate.h"

#include <stdio.h>
#include "Types.h"
#include "String.h"

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

EffectiveAddress RMToEffectiveAddress(char rm) {
	return (EffectiveAddress)rm;
}

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

String ByteToBinaryString(byte b) {
	char binaryStr[9];
	for (int i = 0; i < 8; i++) {
		binaryStr[7 - i] = (b & (1 << i)) ? '1' : '0';
	}
	binaryStr[8] = '\0';
	return String(binaryStr);
}

bool DecodeInstruction(DecodeContext& ctx, Instruction& instruction) {
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
		printf("Couldn't find InstructionDef for this instruction: 0b%s\n", ByteToBinaryString(opCode).c_str());
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

// 140 line func
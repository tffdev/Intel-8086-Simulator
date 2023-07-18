#include "Decoder.h"

#include <stdio.h>
#include "Types.h"
#include "String.h"
#include "StringifyTypes.h"

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
	String name;

	void BuildMask(int numBitsForMask) {
		opCodeMask = 0;
		for (int i = 0; i <= numBitsForMask; i++) {
			opCodeMask |= (1 << (8 - i));
		}
	}

	InstructionDef(String name, OpCode inst, int numBitsForMask, byte opCodeValue, int decodeFlags = 0, int optionalRequiredReg = -1)
		: name(name), inst(inst), opCodeValue(opCodeValue), optionalRequiredReg(optionalRequiredReg), decodeFlags(decodeFlags)
	{
		BuildMask(numBitsForMask);
	}
};

enum OpFlag : int {
	NONE = 0,
	MOD_AND_DISP = 1 << 0,
	REG = 1 << 1,
	REGMEM = 1 << 2,
	W = 1 << 3,
	D = 1 << 4,
	S = 1 << 5,
	V = 1 << 6,
	SEGREG = 1 << 7,
	ONE_BYTE_OP = 1 << 8,
	W_5THBIT = 1 << 9,
	REG_IN_FIRSTBYTE = 1 << 10,
	HAS_DATA = 1 << 11,
	HAS_ADDR = 1 << 12,
	REG_IS_DEST = 1 << 13,
	HAS_IP8 = 1 << 14,
};

InstructionDef instructionDefs[] = {
	InstructionDef("mov", OpCode::MOVE_TOFROM_REGMEM, 6, 0b10001000, OpFlag::MOD_AND_DISP|OpFlag::REG|OpFlag::REGMEM|OpFlag::W|OpFlag::D),
	InstructionDef("mov", OpCode::MOVE_IMMEDIATE_TO_REGMEM, 7, 0b11000110, OpFlag::MOD_AND_DISP|OpFlag::REGMEM|OpFlag::W|OpFlag::HAS_DATA),
	InstructionDef("mov", OpCode::MOVE_IMMEDIATE_TO_REG, 4, 0b10110000, OpFlag::ONE_BYTE_OP|OpFlag::REG_IN_FIRSTBYTE|OpFlag::W_5THBIT|OpFlag::HAS_DATA),
	InstructionDef("mov", OpCode::MOVE_MEMORY_TO_ACCUMULATOR, 7, 0b10100000, OpFlag::W|OpFlag::ONE_BYTE_OP|OpFlag::HAS_ADDR|OpFlag::REG_IS_DEST),
	InstructionDef("mov", OpCode::MOVE_ACCUMULATOR_TO_MEMORY, 7, 0b10100010, OpFlag::W|OpFlag::ONE_BYTE_OP|OpFlag::HAS_ADDR),
	InstructionDef("mov", OpCode::MOVE_REGMEM_TO_SEGMENT, 8, 0b10001110, OpFlag::MOD_AND_DISP|OpFlag::REGMEM|OpFlag::SEGREG|OpFlag::REG_IS_DEST),
	InstructionDef("mov", OpCode::MOVE_SEGMENT_TO_REGMEM, 8, 0b10001100, OpFlag::MOD_AND_DISP|OpFlag::REGMEM|OpFlag::SEGREG),
	//// Add
	InstructionDef("add", OpCode::ADD_TOFROM_REGMEM, 6, 0b00000000, OpFlag::D|OpFlag::W|OpFlag::MOD_AND_DISP | OpFlag::REG | OpFlag::REGMEM),
	InstructionDef("add", OpCode::ADD_IMMEDIATE_TO_REGMEM, 6, 0b10000000, OpFlag::S|OpFlag::W|OpFlag::MOD_AND_DISP|OpFlag::REGMEM|OpFlag::HAS_DATA, 0b000),
	InstructionDef("add", OpCode::ADD_IMMEDIATE_TO_ACCUMULATOR, 7, 0b00000100, OpFlag::W|OpFlag::HAS_DATA),
	//// Sub
	//InstructionDef(OpCode::SUB_TOFROM_REGMEM, 6, 0b00101000, OpFlag::MOD),
	//InstructionDef(OpCode::SUB_IMMEDIATE_TO_REGMEM, 6, 0b10000000, 0b101, OpFlag::MOD),
	//InstructionDef(OpCode::SUB_IMMEDIATE_TO_ACCUMULATOR, 7, 0b00101100, OpFlag::MOD),
	//// Compare
	//InstructionDef(OpCode::CMP_REG_WITH_REGMEM, 6, 0b00111000, OpFlag::MOD),
	//InstructionDef(OpCode::CMP_IMMEDIATE_WITH_REGMEM, 6, 0b10000000, 0b111, OpFlag::MOD),
	//InstructionDef(OpCode::CMP_IMMEDIATE_WITH_ACCUMULATOR, 7, 0b00111100, OpFlag::MOD),
	//// Jump
	//InstructionDef(OpCode::JUMP_ON_EQUAL_OR_ZERO, 8, 0b01110100, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_LESS, 8, 0b01111100, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_LESS_OR_EQUAL, 8, 0b01111110, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_BELOW, 8, 0b01110010, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_BELOW_OR_EQUAL, 8, 0b01110110, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_PARITY, 8, 0b01111010, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_OVERFLOW, 8, 0b01110000, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_SIGN, 8, 0b01111000, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_NOT_EQUAL_OR_ZERO, 8, 0b01110101, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_GREATER_OR_EQUAL, 8, 0b01111101, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_GREATER, 8, 0b01111111, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_ABOVE_OR_EQUAL, 8, 0b01110011, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_ABOVE, 8, 0b01110111, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_NOT_PARITY, 8, 0b01111011, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_NOT_OVERFLOW, 8, 0b01110001, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_NOT_SIGN, 8, 0b01111001, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::LOOP_CX_TIMES, 8, 0b11100010, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::LOOP_WHILE_ZERO, 8, 0b11100011, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::LOOP_WHILE_NOT_ZERO, 8, 0b11100001, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
	//InstructionDef(OpCode::JUMP_ON_CX_ZERO, 8, 0b11100000, OpFlag::ONE_BYTE_OP | OpFlag::HAS_IP8),
};

String ByteToBinaryString(byte b) {
	char binaryStr[9];
	for (int i = 0; i < 8; i++) {
		binaryStr[7 - i] = (b & (1 << i)) ? '1' : '0';
	}
	binaryStr[8] = '\0';
	return String(binaryStr);
}

bool DecodeInstruction(DecodeContext& ctx, NewInstruction& instruction) {
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
			instruction.opCode = instructionDefs[i].inst;
			instruction.nameStr = instructionDefs[i].name;
			flags = instructionDefs[i].decodeFlags;
			defFound = true;
		}
	}

	if (!defFound) {
		printf("Couldn't find InstructionDef for this instruction: 0b%s\n", ByteToBinaryString(opCode).c_str());
		return false;
	}

	// Parse full instruction using flags
	int sizeOfInstruction = 0;

	int dataSign = 0;
	bool isWide = false;

	int memDisplacement = 0;
	int memDisplacementSize = 0;

	bool destIsReg = false;
	int mod = 0;

	MoveMode moveMode = MoveMode::REGISTER;

	if (flags & OpFlag::ONE_BYTE_OP) {
		sizeOfInstruction = 1;
	}
	else {
		sizeOfInstruction = 2;
	}

	if (flags & OpFlag::W) {
		isWide = (buffer[0] & 0b00000001) != 0;
	}

	if (flags & OpFlag::W_5THBIT) {
		isWide = (buffer[0] & 0b00001000) != 0;
	}

	if (flags & OpFlag::REG_IS_DEST) {
		destIsReg = true;
	}

	if (flags & OpFlag::S) {
		dataSign = (buffer[0] & 0b00000010);
		isWide = isWide && !dataSign;
	}

	if (flags & OpFlag::D) {
		destIsReg = (buffer[0] & 0b00000010) != 0;
	}

	if (flags & OpFlag::HAS_IP8) {
		instruction.relativeJump = buffer[1];
	}

	// Flags with side effects
	if (flags & OpFlag::SEGREG) {
		isWide = true; // assume wide
		int sr = (buffer[1] & 0b00011000) >> 3;
		Operand& reg = destIsReg ? instruction.dest : instruction.source;
		reg.opType = Operand::Type::REGISTER;
		reg.reg = SRToSegmentRegister(sr);
	}

	if (flags & OpFlag::MOD_AND_DISP) {
		mod = (buffer[1] & 0b11000000) >> 6;
		// everything with mod has a displacement
		switch (mod) {
		case 0b00: memDisplacementSize = 0; moveMode = MoveMode::MEMORY; break;
		case 0b01: memDisplacementSize = 1; moveMode = MoveMode::MEMORY; break;
		case 0b10: memDisplacementSize = 2; moveMode = MoveMode::MEMORY; break;
		case 0b11: memDisplacementSize = 0; moveMode = MoveMode::REGISTER; break;
		}
		// Calculate mem displacement
		if (memDisplacementSize == 1) {
			memDisplacement = buffer[2];
		}
		else if (memDisplacementSize == 2) {
			memDisplacement = buffer[3] << 8 | buffer[2];
		}
		sizeOfInstruction += memDisplacementSize;
	}

	if (flags & OpFlag::REG) {
		int regInt = (buffer[1] & 0b00111000) >> 3;
		Operand& reg = destIsReg ? instruction.dest : instruction.source;
		reg.opType = Operand::Type::REGISTER;
		reg.reg = RegisterParse(regInt, isWide);
	}

	if (flags & OpFlag::REGMEM) {
		int regMemInt = (buffer[1] & 0b00000111);
		Operand& regMem = destIsReg ? instruction.source : instruction.dest;
		if (moveMode == MoveMode::REGISTER) {
			regMem.opType = Operand::Type::REGISTER;
			regMem.reg = RegisterParse(regMemInt, isWide);
		}
		else {
			// Check for direct address mode, in which 16 bit displacement follows
			if (regMemInt == 0b110 && mod == 0b00) {
				memDisplacementSize = 2;
				sizeOfInstruction += 2;
				regMem.opType = Operand::Type::MEMORY_LOC;
				regMem.mem.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;
				regMem.mem.memoryOffset = buffer[3] << 8 | buffer[2];
			}
			else {
				regMem.opType = Operand::Type::MEMORY_LOC;
				regMem.mem.effectiveAddress = RMToEffectiveAddress(regMemInt);
				regMem.mem.memoryOffset = memDisplacement;
			}
		}
	}

	if (flags & OpFlag::REG_IN_FIRSTBYTE) {
		int regInt = (buffer[0] & 0b00000111);
		Operand& reg = instruction.dest;
		reg.reg = RegisterParse(regInt, isWide);
		reg.opType = Operand::Type::REGISTER;
	}

	if (flags & OpFlag::HAS_DATA) {
		if (isWide) {
			// only explicit sizes if moving immediate to memory
			if(instruction.dest.opType == Operand::Type::MEMORY_LOC) 
				instruction.source.dataSize = ExplicitDataSize::WORD;

			instruction.source.opType = Operand::Type::IMMEDIATE;
			instruction.source.immediate = (word)((buffer[1 + sizeOfInstruction] << 8) | buffer[sizeOfInstruction]);
		}
		else {
			if (instruction.dest.opType == Operand::Type::MEMORY_LOC)
				instruction.source.dataSize = ExplicitDataSize::BYTE;

			instruction.source.opType = Operand::Type::IMMEDIATE;
			instruction.source.immediate = (word)(buffer[sizeOfInstruction]);
		}
		sizeOfInstruction += isWide ? 2 : 1;
	}

	if (flags & OpFlag::HAS_ADDR) {
		Operand& accOperand = (destIsReg) ? instruction.source : instruction.dest;
		Operand& addrOperand = (destIsReg) ? instruction.dest : instruction.source;

		accOperand.opType = Operand::Type::REGISTER;
		accOperand.reg = Register::AX;

		// This is an exact 16-bit address
		addrOperand.opType = Operand::Type::MEMORY_LOC;
		addrOperand.mem.memoryOffset = (word)((buffer[2 + memDisplacementSize] << 8) | buffer[1 + memDisplacementSize]);
		addrOperand.mem.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;
		sizeOfInstruction += 2;
	}

	// Return the number of bytes read
	ctx.bp += sizeOfInstruction;

	return true;
}

String OperandToString(Operand& o) {
	switch (o.opType) {
	case Operand::Type::REGISTER: return RegisterToString(o.reg);
	case Operand::Type::MEMORY_LOC: return EffectiveAddressWithOffsetToString(o.mem.effectiveAddress, o.mem.memoryOffset);
	case Operand::Type::IMMEDIATE: {
		switch (o.dataSize) {
		case ExplicitDataSize::WORD: return String::Format("word %i", o.immediate);
		case ExplicitDataSize::BYTE: return String::Format("byte %i", o.immediate);
		default: return String::Format("%i", o.immediate);
		}
	}
	}
}

void Decompile(Buffer& buffer) {
	printf(";Executable read successfully\n");
	printf(";Size: %d\n", buffer.size);
	printf(";Decompiled instructions: \n");
	printf("bits 16\n");

	// Print-decode buffer
	DecodeContext ctx;
	ctx.bp = 0;
	ctx.buffer = buffer.data;
	ctx.instructionCount = 0;
	while (ctx.bp < buffer.size)
	{
		NewInstruction inst{};
		bool success = DecodeInstruction(ctx, inst);
		if (!success) return;

		String op = String::Format("%s %s, %s", inst.nameStr.c_str(), OperandToString(inst.dest).c_str(), OperandToString(inst.source).c_str());
		printf("%s\n", op.c_str());
		ctx.instructionCount++;
	}
}
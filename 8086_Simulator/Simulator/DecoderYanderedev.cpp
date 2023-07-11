#include "String.h"
#include "Types.h"
#include <stdio.h>

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

struct Mapping {
	OpCode opCode;
	byte mask;
	byte value;

	bool useOptionalReg;
	byte optionalReg;

	Mapping(OpCode opCode, int numBitsForMask, byte value, bool useOptionalReg = false, byte optionalReg = 0)
		: opCode(opCode), value(value), useOptionalReg(useOptionalReg), optionalReg(optionalReg)
	{
		mask = 0;
		for (int i = 0; i < numBitsForMask; i++) {
			mask |= (1 << i);
		}
		mask <<= (8 - numBitsForMask);
	}
};

Mapping mappings[] = {
	// Move
	Mapping(OpCode::MOVE_TOFROM_REGMEM, 6, 0b10001000),
	Mapping(OpCode::MOVE_IMMEDIATE_TO_REGMEM, 7, 0b11000110),
	Mapping(OpCode::MOVE_IMMEDIATE_TO_REG, 4, 0b10110000),
	Mapping(OpCode::MOVE_MEMORY_TO_ACCUMULATOR, 7, 0b10100000),
	Mapping(OpCode::MOVE_ACCUMULATOR_TO_MEMORY, 7, 0b10100010),
	Mapping(OpCode::MOVE_REGMEM_TO_SEGMENT, 8, 0b10001110),
	Mapping(OpCode::MOVE_SEGMENT_TO_REGMEM, 8, 0b10001100),
	// Add
	Mapping(OpCode::ADD_TOFROM_REGMEM, 6, 0b00000000),
	Mapping(OpCode::ADD_IMMEDIATE_TO_REGMEM, 6, 0b10000000, true, 0b000),
	Mapping(OpCode::ADD_IMMEDIATE_TO_ACCUMULATOR, 7, 0b00000100),
	// Sub
	Mapping(OpCode::SUB_TOFROM_REGMEM, 6, 0b00101000),
	Mapping(OpCode::SUB_IMMEDIATE_TO_REGMEM, 6, 0b10000000, true, 0b101),
	Mapping(OpCode::SUB_IMMEDIATE_TO_ACCUMULATOR, 7, 0b00101100),
	// Compare
	Mapping(OpCode::CMP_REG_WITH_REGMEM, 6, 0b00111000),
	Mapping(OpCode::CMP_IMMEDIATE_WITH_REGMEM, 6, 0b10000000, true, 0b111),
	Mapping(OpCode::CMP_IMMEDIATE_WITH_ACCUMULATOR, 7, 0b00111100),

	Mapping(OpCode::JUMP_ON_EQUAL_OR_ZERO,		8, 0b01110100),
	Mapping(OpCode::JUMP_ON_LESS,				8, 0b01111100),
	Mapping(OpCode::JUMP_ON_LESS_OR_EQUAL,		8, 0b01111110),
	Mapping(OpCode::JUMP_ON_BELOW,				8, 0b01110010),
	Mapping(OpCode::JUMP_ON_BELOW_OR_EQUAL,		8, 0b01110110),
	Mapping(OpCode::JUMP_ON_PARITY,				8, 0b01111010),
	Mapping(OpCode::JUMP_ON_OVERFLOW,			8, 0b01110000),
	Mapping(OpCode::JUMP_ON_SIGN,				8, 0b01111000),
	Mapping(OpCode::JUMP_ON_NOT_EQUAL_OR_ZERO,	8, 0b01110101),
	Mapping(OpCode::JUMP_ON_GREATER_OR_EQUAL,	8, 0b01111101),
	Mapping(OpCode::JUMP_ON_GREATER,			8, 0b01111111),
	Mapping(OpCode::JUMP_ON_ABOVE_OR_EQUAL,		8, 0b01110011),
	Mapping(OpCode::JUMP_ON_ABOVE,				8, 0b01110111),
	Mapping(OpCode::JUMP_ON_NOT_PARITY,			8, 0b01111011),
	Mapping(OpCode::JUMP_ON_NOT_OVERFLOW,		8, 0b01110001),
	Mapping(OpCode::JUMP_ON_NOT_SIGN,			8, 0b01111001),
	Mapping(OpCode::LOOP_CX_TIMES,				8, 0b11100010),
	Mapping(OpCode::LOOP_WHILE_ZERO,			8, 0b11100011),
	Mapping(OpCode::LOOP_WHILE_NOT_ZERO,		8, 0b11100001),
	Mapping(OpCode::JUMP_ON_CX_ZERO,			8, 0b11100000),
};

OpCode ParseOpCode(byte opCode, byte secondByte) {
	// get "reg" from second byte
	byte reg = (secondByte & 0b00111000) >> 3;

	for (int i = 0; i < sizeof(mappings) / sizeof(Mapping); i++) {
		Mapping mapping = mappings[i];
		if ((opCode & mapping.mask) == mapping.value) {
			// Double check that the optional reg is correct
			if (mapping.useOptionalReg && (reg != mapping.optionalReg)) {
				continue;
			}
			return mapping.opCode;
		}
	}
	return OpCode::ERROR;
}

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

String DataSizeToString(ExplicitDataSize s) {
	if (s == ExplicitDataSize::Byte) {
		return "byte ";
	}
	else {
		return "word ";
	}
}

Register RegisterParse(char reg, bool w) {
	int index = (int)reg + (w ? 8 : 0);
	return (Register)index;
}

void CalculateReg(char mod, bool isWide, char reg, RegMem& out) {
	out.type = MoveMode::Register;
	out.reg = RegisterParse(reg, isWide);
}

int CalculateRegMem(char mod, byte* bufferPossibleMemStart, bool isWide, char regMem, RegMem& out) {
	// Account for rm being 110, which is a special case with 16-bit displacement
	bool directAddress = (mod == 0b00) && (regMem == 0b110);
	if (directAddress) {
		out.type = MoveMode::Memory;
		out.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;
		out.memoryOffset = (unsigned short)((bufferPossibleMemStart[1] << 8) | bufferPossibleMemStart[0]);
		return 2;
	}

	switch (mod) {
	case 0b00: {
		// No displacement, to memory
		out.type = MoveMode::Memory;
		out.effectiveAddress = RMToEffectiveAddress(regMem);
		out.memoryOffset = 0;
		return 0;
	}
	case 0b01: {
		// 8-bit displacement, to memory
		out.type = MoveMode::Memory;
		out.effectiveAddress = RMToEffectiveAddress(regMem);
		out.memoryOffset = (signed char)bufferPossibleMemStart[0];
		return 1;
	}
	case 0b10: {
		// 16-bit displacement, to memory
		out.type = MoveMode::Memory;
		out.effectiveAddress = RMToEffectiveAddress(regMem);
		out.memoryOffset = (signed short)((bufferPossibleMemStart[1] << 8) | bufferPossibleMemStart[0]);
		return 2;
	}
	case 0b11: {
		// No displacement, to register
		out.type = MoveMode::Register;
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

String RegMemToString(RegMem& r) {
	if (r.type == MoveMode::Register) {
		return RegisterToString(r.reg);
	}
	else {
		return EffectiveAddressWithOffsetToString(r.effectiveAddress, r.memoryOffset);
	}
}


//----------------------------------------------
// CPU helpers
//----------------------------------------------

bool RegisterIsShort(Register r) {
	return r < Register::AX;
}

bool RegisterIsLow(Register r) {
	return r == Register::AL || r == Register::BL || r == Register::CL || r == Register::DL;
}

bool RegisterIsHigh(Register r) {
	return r == Register::AH || r == Register::BH || r == Register::CH || r == Register::DH;
}

bool RegisterIsSegment(Register r) {
	return r >= Register::CS && r <= Register::ES;
}

Register GetWideRegFromShortReg(Register reg) {
	// Given a short register, return the corresponding wide register
	// Fuck it!!
	switch (reg) {
	case Register::AL: return Register::AX;
	case Register::AH: return Register::AX;
	case Register::BL: return Register::BX;
	case Register::BH: return Register::BX;
	case Register::CL: return Register::CX;
	case Register::CH: return Register::CX;
	case Register::DL: return Register::DX;
	case Register::DH: return Register::DX;
	}
	return reg;
}

int RegisterRead(CPU& cpu, Register source) {
	switch (source) {
	case Register::AL: return (int)cpu.al; break;
	case Register::AH: return (int)cpu.ah; break;
	case Register::BL: return (int)cpu.bl; break;
	case Register::BH: return (int)cpu.bh; break;
	case Register::CL: return (int)cpu.cl; break;
	case Register::CH: return (int)cpu.ch; break;
	case Register::DL: return (int)cpu.dl; break;
	case Register::DH: return (int)cpu.dh; break;

	case Register::AX: return (int)cpu.ax; break;
	case Register::BX: return (int)cpu.bx; break;
	case Register::CX: return (int)cpu.cx; break;
	case Register::DX: return (int)cpu.dx; break;

	case Register::SP: return (int)cpu.sp; break;
	case Register::BP: return (int)cpu.bp; break;
	case Register::SI: return (int)cpu.si; break;
	case Register::DI: return (int)cpu.di; break;

	case Register::CS: return (int)cpu.cs; break;
	case Register::DS: return (int)cpu.ds; break;
	case Register::ES: return (int)cpu.es; break;
	case Register::SS: return (int)cpu.ss; break;
	}
}

void RegisterWrite(CPU& cpu, Register reg, int value) {
	Register wideReg = GetWideRegFromShortReg(reg);

	Register changedRegister = wideReg;
	int before = RegisterRead(cpu, changedRegister);

	// No clean conversion from register as int to actual bytes in cpu!! annoying ough
	switch (reg) {
	case Register::AL: cpu.al = (byte)value; break;
	case Register::AH: cpu.ah = (byte)value; break;
	case Register::BL: cpu.bl = (byte)value; break;
	case Register::BH: cpu.bh = (byte)value; break;
	case Register::CL: cpu.cl = (byte)value; break;
	case Register::CH: cpu.ch = (byte)value; break;
	case Register::DL: cpu.dl = (byte)value; break;
	case Register::DH: cpu.dh = (byte)value; break;

	case Register::AX: cpu.ax = (word)value; break;
	case Register::BX: cpu.bx = (word)value; break;
	case Register::CX: cpu.cx = (word)value; break;
	case Register::DX: cpu.dx = (word)value; break;

	case Register::SP: cpu.sp = (word)value; break;
	case Register::BP: cpu.bp = (word)value; break;
	case Register::SI: cpu.si = (word)value; break;
	case Register::DI: cpu.di = (word)value; break;

	case Register::CS: cpu.cs = (word)value; break;
	case Register::DS: cpu.ds = (word)value; break;
	case Register::ES: cpu.es = (word)value; break;
	case Register::SS: cpu.ss = (word)value; break;
	}

	int after = RegisterRead(cpu, changedRegister);
	printf("%s: 0x%04x -> 0x%04x\n", RegisterToString(changedRegister).c_str(), before, after);
}

int EffectiveAddressMemoryLocation(CPU& cpu, EffectiveAddress addr) {
	switch (addr) {
	case EffectiveAddress::BX_SI: return cpu.bx + cpu.si; break;
	case EffectiveAddress::BX_DI: return cpu.bx + cpu.di; break;
	case EffectiveAddress::BP_SI: return cpu.bp + cpu.si; break;
	case EffectiveAddress::BP_DI: return cpu.bp + cpu.di; break;
	case EffectiveAddress::SI: return cpu.si; break;
	case EffectiveAddress::DI: return cpu.di; break;
	case EffectiveAddress::BP: return cpu.bp; break;
	case EffectiveAddress::BX: return cpu.bx; break;
	case EffectiveAddress::DIRECT_ADDRESS: return 0; break;
	}
}

void EffectiveAddressWrite(CPU& cpu, EffectiveAddress addr, int memoryOffset, int value) {
	int memoryLocation = EffectiveAddressMemoryLocation(cpu, addr);
	memoryLocation += memoryOffset;

	// Write to memory
}

int EffectiveAddressRead(CPU& cpu, EffectiveAddress addr, int memoryOffset) {
	int memoryLocation = EffectiveAddressMemoryLocation(cpu, addr);
	memoryLocation += memoryOffset;

	// Read from memory
	return 0;
}

void RegMemWrite(CPU& cpu, RegMem dest, int value) {
	if (dest.type == MoveMode::Register) {
		RegisterWrite(cpu, dest.reg, value);
	}
	else {
		EffectiveAddressWrite(cpu, dest.effectiveAddress, dest.memoryOffset, value);
	}
}

int RegMemRead(CPU& cpu, RegMem source) {
	if (source.type == MoveMode::Register) {
		return RegisterRead(cpu, source.reg);
	}
	else {
		return EffectiveAddressRead(cpu, source.effectiveAddress, source.memoryOffset);
	}
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

void OperationMoveToFromRegMemProcess(CPU& cpu, OperationMoveToFromRegMem& move) {
	int value = RegMemRead(cpu, move.source);
	RegMemWrite(cpu, move.dest, value);
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

	move.dataSize = isDataWide ? ExplicitDataSize::Word : ExplicitDataSize::Byte;

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

void OperationMoveImmediateToRegisterProcesses(CPU& cpu, OperationMoveImmediateToRegister move) {
	RegisterWrite(cpu, move.registerDst, move.immediateData);
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
	move.dataSize = isWide ? ExplicitDataSize::Word : ExplicitDataSize::Byte;
	move.dest.type = MoveMode::Register;
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

void OperationMoveRegMemToSegmentRegisterProcesses(CPU& cpu, OperationMoveRegMemToSegmentRegister move) {
	int data = RegMemRead(cpu, move.source);
	RegisterWrite(cpu, move.dest, data);
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

void OperationMoveSegmentRegisterToRegMemProcesses(CPU& cpu, OperationMoveSegmentRegisterToRegMem move) {
	int data = RegisterRead(cpu, move.source);
	RegMemWrite(cpu, move.dest, data);
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

	add.dataSize = isWide ? ExplicitDataSize::Word : ExplicitDataSize::Byte;

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

	add.dest.type = MoveMode::Register;
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

	sub.dataSize = isWide ? ExplicitDataSize::Word : ExplicitDataSize::Byte;

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

	sub.dest.type = MoveMode::Register;
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

	cmp.dataSize = isWide ? ExplicitDataSize::Word : ExplicitDataSize::Byte;

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

	cmp.dest.type = MoveMode::Register;
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
	case OperationJumpConditional::JumpOnNotEqualOrZero: return "jne";
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

int OperationJumpConditionalParse(unsigned char* buffer, OperationJumpConditional& jump) {
	// Jump offsets are relative to the next instruction in bytes
	jump.condition = (OperationJumpConditional::Condition)buffer[0];
	jump.jumpOffset = ParseImmediateData(&buffer[1], false);
	return 2;
}

void OperationJumpConditionalPrint(OperationJumpConditional jump) {
	if (jump.jumpOffset > 0) {
		printf("%s label_%d\n", ConditionToString(jump.condition).c_str(), jump.jumpOffset);
	}
	else {
		printf("%s label_N%d\n", ConditionToString(jump.condition).c_str(), -jump.jumpOffset);
	}
}

//----------------------------------------------
// Testing stuff
//----------------------------------------------
struct Buffer {
	byte* data;
	int size;
};

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
			bytes = OperationJumpConditionalParse(&buffer.data[bp], jump);
			OperationJumpConditionalPrint(jump);
			break;
		}
		default: {
			printf("ERROR: Unhandled opcode 0x%x\n", buffer.data[bp]);
			return;
		}
		}

		bp += bytes;
	}
}
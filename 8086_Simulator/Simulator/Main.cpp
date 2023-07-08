// Creator: Daniel B.
// Date: 2023-07-06
// This file was made for Casey Muratori's 8086 homework segment for the Computer Enhance course
// on performance programming. For the sake of keeping things straight forward and easy to debug and extend, 
// this file is written in a bare-bones and verbose C style.
// All "magic numbers" used are from the Intel 8086 manual
// which can be found here: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf

#include <stdio.h>
#include "String.h"

typedef unsigned char byte;
typedef unsigned short word;

//----------------------------------------------
// CPU Data
//----------------------------------------------
enum class Register : int {
	AL, CL, DL, BL, AH, CH, DH, BH,
	AX, CX, DX, BX, SP, BP, SI, DI
};

struct CPU {
	// All registers
	byte AL, CL, DL, BL, AH, CH, DH, BH;
	word AX, CX, DX, BX, SP, BP, SI, DI;
};

Register RegisterParse(char reg, bool w) {
	int index = (int)reg + (w ? 8 : 0);
	return (Register)index;
}

String RegisterToString(Register reg) {
	static String names[] = {
		"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
		"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
	};
	return names[(int)reg];
}

//----------------------------------------------
// CPU Instructions
//----------------------------------------------
enum class OpCode {
	ERROR,
	MOVE_TOFROM_REGMEM,
	MOVE_IMMEDIATE_TO_REGMEM,
	MOVE_IMMEDIATE_TO_REG,
	MOVE_MEMORY_TO_ACCUMULATOR,
	MOVE_ACCUMULATOR_TO_MEMORY,

	ADD_IMMEDIATE_TO_ACCUMULATOR,
};

struct Mapping {
	OpCode opCode;
	byte mask;
	byte value;

	Mapping(OpCode opCode, byte mask, byte value)
		: opCode(opCode), mask(mask), value(value)
	{}
};

Mapping mappings[] = {
	Mapping(OpCode::MOVE_TOFROM_REGMEM, 0b11111100, 0b10001000),
	Mapping(OpCode::MOVE_IMMEDIATE_TO_REGMEM, 0b11111110, 0b11000110),
	Mapping(OpCode::MOVE_IMMEDIATE_TO_REG, 0b11110000, 0b10110000),
	Mapping(OpCode::MOVE_MEMORY_TO_ACCUMULATOR, 0b11111110, 0b10100000),
	Mapping(OpCode::MOVE_ACCUMULATOR_TO_MEMORY, 0b11111110, 0b10100010),
	Mapping(OpCode::ADD_IMMEDIATE_TO_ACCUMULATOR, 0b11111110, 0b00000100),
};

OpCode ParseOpCode(byte opCode) {
	for (int i = 0; i < sizeof(mappings) / sizeof(Mapping); i++) {
		Mapping mapping = mappings[i];
		if ((opCode & mapping.mask) == mapping.value) {
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
	static String effectiveAddresses[] = {
		"[bx+si+%i]",
		"[bx+di+%i]",
		"[bp+si+%i]",
		"[bp+di+%i]",
		"[si+%i]",
		"[di+%i]",
		"[bp]", 
		"[bx+%i]",
		"[%i]" // Direct address
	};

	static String effectiveAddressesNegativeOffset[] = {
		"[bx+si-%i]",
		"[bx+di-%i]",
		"[bp+si-%i]",
		"[bp+di-%i]",
		"[si-%i]",
		"[di-%i]",
		"[bp]",
		"[bx-%i]",
		"[%i]" // Direct address
	};

	if (offset < 0) {
		return String::Format(effectiveAddressesNegativeOffset[(int)addr], -offset);
	}

	return String::Format(effectiveAddresses[(int)addr], offset);
}

//----------------------------------------------
// MOVE Register/memory to/from register
//----------------------------------------------
struct OperationMoveToFromRegMem {
	enum class Mode {
		RegisterToRegister,
		MemoryToRegister,
		MemoryToRegisterWithOffset,
		RegisterToMemory,
		RegisterToMemoryWithOffset
	};

	Mode mode;
	Register registerSrc;
	Register registerDst;
	EffectiveAddress effectiveAddress;
	int memoryOffset;
};

int ParseMoveToFromRegMem(unsigned char* buffer, OperationMoveToFromRegMem& move) {
	bool destIsReg = buffer[0] & 0b00000010;
	bool isWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char reg = (buffer[1] & 0b00111000) >> 3;
	char regMem = (buffer[1] & 0b00000111);

	// Account for direct address edge case
	bool directAddress = (mod == 0b00) && (regMem == 0b110);
	if (directAddress) {
		if (destIsReg) {
			move.mode = OperationMoveToFromRegMem::Mode::MemoryToRegisterWithOffset;
			move.registerDst = RegisterParse(reg, isWide);
			move.memoryOffset = (buffer[3] << 8) | buffer[2];
			move.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;
		}
		else {
			move.mode = OperationMoveToFromRegMem::Mode::RegisterToMemoryWithOffset;
			move.registerSrc = RegisterParse(reg, isWide);
			move.memoryOffset = (buffer[3] << 8) | buffer[2];
			move.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;
		}
		return 4;
	}

	switch (mod) {
	case 0b00: {
		if (destIsReg) {
			move.mode = OperationMoveToFromRegMem::Mode::MemoryToRegister;
			move.registerDst = RegisterParse(reg, isWide);
			move.effectiveAddress = RMToEffectiveAddress(regMem);
		}
		else {
			move.mode = OperationMoveToFromRegMem::Mode::RegisterToMemory;
			move.registerSrc = RegisterParse(reg, isWide);
			move.effectiveAddress = RMToEffectiveAddress(regMem);
		}
		return 2;
		break;
	}
	case 0b01: {
		// 8-bit displacement
		if (destIsReg) {
			move.mode = OperationMoveToFromRegMem::Mode::MemoryToRegisterWithOffset;
			move.registerDst = RegisterParse(reg, isWide);
			move.effectiveAddress = RMToEffectiveAddress(regMem);
			move.memoryOffset = (signed char)buffer[2];
		} else {
			move.mode = OperationMoveToFromRegMem::Mode::RegisterToMemoryWithOffset;
			move.registerSrc = RegisterParse(reg, isWide);
			move.effectiveAddress = RMToEffectiveAddress(regMem);
			move.memoryOffset = (signed char)buffer[2];
		}
		return 3;
		break;
	}
	case 0b10: {
		// 16-bit displacement
		if (destIsReg) {
			move.mode = OperationMoveToFromRegMem::Mode::MemoryToRegisterWithOffset;
			move.registerDst = RegisterParse(reg, isWide);
			move.memoryOffset = (signed short)((buffer[3] << 8) | buffer[2]);
			move.effectiveAddress = RMToEffectiveAddress(regMem);
		}
		else {
			move.mode = OperationMoveToFromRegMem::Mode::RegisterToMemoryWithOffset;
			move.registerSrc = RegisterParse(reg, isWide);
			move.memoryOffset = (signed short)((buffer[3] << 8) | buffer[2]);
			move.effectiveAddress = RMToEffectiveAddress(regMem);
		}
		return 4;
	}
	case 0b11: {
		move.mode = OperationMoveToFromRegMem::Mode::RegisterToRegister;
		move.registerSrc = RegisterParse(reg, isWide);
		move.registerDst = RegisterParse(regMem, isWide);
		return 2;
		break;
	}
	default:
		printf("ERROR: Unhandled mod %d\n", mod);
		return 0;
	}
}

void PrintMove(OperationMoveToFromRegMem move) {
	switch (move.mode) {
	case OperationMoveToFromRegMem::Mode::RegisterToRegister:
		printf("mov %s, %s\n", RegisterToString(move.registerDst).c_str(), RegisterToString(move.registerSrc).c_str());
		break;
	case OperationMoveToFromRegMem::Mode::MemoryToRegister:
		printf("mov %s, %s\n", RegisterToString(move.registerDst).c_str(), EffectiveAddressToString(move.effectiveAddress).c_str());
		break;
	case OperationMoveToFromRegMem::Mode::MemoryToRegisterWithOffset:
		printf("mov %s, %s\n", RegisterToString(move.registerDst).c_str(), EffectiveAddressWithOffsetToString(move.effectiveAddress, move.memoryOffset).c_str());
		break;
	case OperationMoveToFromRegMem::Mode::RegisterToMemory:
		printf("mov %s, %s\n", EffectiveAddressToString(move.effectiveAddress).c_str(), RegisterToString(move.registerSrc).c_str());
		break;
	case OperationMoveToFromRegMem::Mode::RegisterToMemoryWithOffset:
		printf("mov %s, %s\n", EffectiveAddressWithOffsetToString(move.effectiveAddress, move.memoryOffset).c_str(), RegisterToString(move.registerSrc).c_str());
		break;
	default:
		printf("Error: Unknown move mode\n");
		break;
	}
}

//----------------------------------------------
// MOVE immediate to register/memory
//----------------------------------------------
struct OperationMoveImmediateToRegMem {
	enum class Mode {
		ImmediateToRegister,
		ImmediateToMemory,
		ImmediateToMemoryWithOffset
	};

	enum class DataSize {
		Byte,
		Word
	};

	Mode mode;
	Register registerDst;
	EffectiveAddress effectiveAddress;
	int memoryOffset;
	int immediateData;
	DataSize dataSize;
};

int ParseMoveImmediateToRegMem(unsigned char* buffer, OperationMoveImmediateToRegMem& move) {
	bool isDataWide = buffer[0] & 0b00000001;
	char mod = (buffer[1] & 0b11000000) >> 6;
	char regMem = (buffer[1] & 0b00000111);
	
	move.dataSize = (OperationMoveImmediateToRegMem::DataSize)isDataWide;

	switch (mod) {
	case 0b00: {
		// No displacement, to memory
		move.mode = OperationMoveImmediateToRegMem::Mode::ImmediateToMemory;
		move.effectiveAddress = RMToEffectiveAddress(regMem);
		move.immediateData = isDataWide ? ((buffer[3] << 8) | buffer[2]) : buffer[2];
		return isDataWide ? 4 : 3;
	}
	case 0b01: {
		// 8-bit displacement, to memory
		move.mode = OperationMoveImmediateToRegMem::Mode::ImmediateToMemoryWithOffset;
		move.effectiveAddress = RMToEffectiveAddress(regMem);
		move.memoryOffset = buffer[2];
		move.immediateData = isDataWide ? ((buffer[4] << 8) | buffer[3]) : buffer[3];
		return isDataWide ? 5 : 4;
	}
	case 0b10: {
		// 16-bit displacement, to memory
		move.mode = OperationMoveImmediateToRegMem::Mode::ImmediateToMemoryWithOffset;
		move.effectiveAddress = RMToEffectiveAddress(regMem);
		move.memoryOffset = (buffer[3] << 8) | buffer[2];
		move.immediateData = isDataWide ? ((buffer[5] << 8) | buffer[4]) : buffer[4];
		return isDataWide ? 6 : 5;
	}
	case 0b11: {
		// No displacement, to register
		move.mode = OperationMoveImmediateToRegMem::Mode::ImmediateToRegister;
		move.registerDst = RegisterParse(regMem, isDataWide);
		move.immediateData = isDataWide ? ((buffer[3] << 8) | buffer[2]) : buffer[2];
		return isDataWide ? 4 : 3;
	}
	default:
		printf("ERROR: Unhandled mod %d\n", mod);
		return 0;
	}
}

void PrintMoveImmediateToRegMem(OperationMoveImmediateToRegMem move) {
	// If immediate data is inconsistent with data size, add prefix
	String prefix("");
	if (move.immediateData) {
		if (move.dataSize == OperationMoveImmediateToRegMem::DataSize::Byte) {
			prefix.Set("byte ");
		}
		else {
			prefix.Set("word ");
		}
	}

	switch (move.mode) {
	case OperationMoveImmediateToRegMem::Mode::ImmediateToRegister:
		printf("mov %s, %s%d\n", RegisterToString(move.registerDst).c_str(), prefix.c_str(), move.immediateData);
		break;
	case OperationMoveImmediateToRegMem::Mode::ImmediateToMemory:
		printf("mov %s, %s%d\n", EffectiveAddressToString(move.effectiveAddress).c_str(), prefix.c_str(), move.immediateData);
		break;
	case OperationMoveImmediateToRegMem::Mode::ImmediateToMemoryWithOffset:
		printf("mov %s, %s%d\n", EffectiveAddressWithOffsetToString(move.effectiveAddress, move.memoryOffset).c_str(), prefix.c_str(), move.immediateData);
		break;
	default:
		printf("Error: Unknown move mode\n");
		break;
	}
}

//----------------------------------------------
// MOVE immediate to register
//----------------------------------------------
struct OperationMoveImmediateToRegister {
	Register registerDst;
	int immediateData;
};

int ParseMoveImmediateToRegister(unsigned char* buffer, OperationMoveImmediateToRegister& move) {
	bool isWide = buffer[0] & 0b00001000;
	char reg = (buffer[0] & 0b00000111);
	move.registerDst = RegisterParse(reg, isWide);
	if (isWide) {
		move.immediateData = (buffer[2] << 8) | buffer[1];
		return 3;
	} else {
		move.immediateData = buffer[1];
		return 2;
	}
}

void PrintMoveImmediateToRegister(OperationMoveImmediateToRegister move) {
	printf("mov %s, %d\n", RegisterToString(move.registerDst).c_str(), move.immediateData);
}

//----------------------------------------------
// MOVE memory to accumulator
//----------------------------------------------
struct OperationMoveMemoryToAccumulator {
	int memoryLocation;
};

int ParseMoveMemoryToAccumulator(unsigned char* buffer, OperationMoveMemoryToAccumulator& move) {
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

void PrintMoveMemoryToAccumulator(OperationMoveMemoryToAccumulator move) {
	printf("mov ax, [%i]\n", move.memoryLocation);
}

//----------------------------------------------
// MOVE accumulator to memory
//----------------------------------------------
struct OperationMoveAccumulatorToMemory {
	int memoryLocation;
};

int ParseMoveAccumulatorToMemory(unsigned char* buffer, OperationMoveAccumulatorToMemory& move) {
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

void PrintMoveAccumulatorToMemory(OperationMoveAccumulatorToMemory move) {
	printf("mov [%i], ax\n", move.memoryLocation);
}

//----------------------------------------------
// ADD immediate to accumulator
//----------------------------------------------
struct OperationAddImmediateToAccumulator {
	int immediateData;
};

int ParseAddImmediateToAccumulator(unsigned char* buffer, OperationAddImmediateToAccumulator& add) {
	bool isWide = buffer[0] & 0b00000001;
	if (isWide) {
		add.immediateData = (buffer[2] << 8) | buffer[1];
		return 3;
	}
	else {
		add.immediateData = buffer[1];
		return 2;
	}
}

void PrintAddImmediateToAccumulator(OperationAddImmediateToAccumulator add) {
	printf("add ax, %d\n", add.immediateData);
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

//----------------------------------------------
// Main
//----------------------------------------------
int main(int argc, char* argv[]) {
	// Parse command line arguments
	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	Buffer buffer = LoadBufferFromFile(argv[1]);

	printf(";Executable read successfully\n");
	printf(";Size: %d\n", buffer.size);
	printf(";Decompiled instructions: \n");
	printf("bits 16\n");

	// Print-decode buffer
	for (int bp = 0; bp < buffer.size;)
	{
		OpCode code = ParseOpCode(buffer.data[bp]);
		int bytes = 0;

		switch (code) {
		case OpCode::MOVE_TOFROM_REGMEM: {
			OperationMoveToFromRegMem move;
			bytes = ParseMoveToFromRegMem(&buffer.data[bp], move);
			PrintMove(move);
			break;
		}
		case OpCode::MOVE_IMMEDIATE_TO_REGMEM: {
			OperationMoveImmediateToRegMem move;
			bytes = ParseMoveImmediateToRegMem(&buffer.data[bp], move);
			PrintMoveImmediateToRegMem(move);
			break;
		}
		case OpCode::MOVE_IMMEDIATE_TO_REG: {
			OperationMoveImmediateToRegister move;
			bytes = ParseMoveImmediateToRegister(&buffer.data[bp], move);
			PrintMoveImmediateToRegister(move);
			break;
		}
		case OpCode::MOVE_MEMORY_TO_ACCUMULATOR: {
			OperationMoveMemoryToAccumulator move;
			bytes = ParseMoveMemoryToAccumulator(&buffer.data[bp], move);
			PrintMoveMemoryToAccumulator(move);
			break;
		}
		case OpCode::MOVE_ACCUMULATOR_TO_MEMORY: {
			OperationMoveAccumulatorToMemory move;
			bytes = ParseMoveAccumulatorToMemory(&buffer.data[bp], move);
			PrintMoveAccumulatorToMemory(move);
			break;
		}
		default: {
			printf("ERROR: Unhandled opcode 0x%x\n", buffer.data[bp]);
			return 1;
		}
		}

		bp += bytes;
	}

	return 0;
}
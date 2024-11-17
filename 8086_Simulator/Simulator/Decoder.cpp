#include "Decoder.h"

#include <stdio.h>

#include "String.h"
#include "Types.h"
#include "StringifyTypes.h"
#include "List.h"

namespace Decoder {
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
		if ((code & 0b11111111) == 0b11101001) return OpCode::JUMP_ALWAYS_RELATIVE_WIDE;
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
		if ((code & 0b11111111) == 0b11001101) return OpCode::INTERRUPT;

		return OpCode::ERROR;
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
		return Register::INVALID;
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
		move.source.immediate = ParseImmediateData(dataLocation, isDataWide);
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
		move.source.mem.effectiveAddress = EffectiveAddress::DIRECT_ADDRESS;

		if (isWide) {
			move.source.mem.memoryOffset = (signed short)((buffer[2] << 8) | buffer[1]);
			return 3;
		}
		else {
			move.source.mem.memoryOffset = (signed char)buffer[1];
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
			move.dest.mem.memoryOffset = (signed short)((buffer[2] << 8) | buffer[1]);
			return 3;
		}
		else {
			move.dest.mem.memoryOffset = (signed char)(buffer[1]);
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

		// Calc effective address
		int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, add.dest);
		byte* dataLocation = buffer + 2 + addressOffset;

		if (add.dest.type == Operand::Type::MEMORY_LOC) {
			add.dest.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;
		}

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

		// Calc effective address
		int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, sub.dest);
		byte* dataLocation = buffer + 2 + addressOffset;

		if (sub.dest.type == Operand::Type::MEMORY_LOC) {
			sub.dest.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;
		}

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

		// Calc effective address
		int addressOffset = CalculateOperandFromRegMem(mod, &buffer[2], isWide, regMem, cmp.dest);
		byte* dataLocation = buffer + 2 + addressOffset;

		if (cmp.dest.type == Operand::Type::MEMORY_LOC) {
			cmp.dest.dataSize = isWide ? ExplicitDataSize::WORD : ExplicitDataSize::BYTE;
		}

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
	int OperationJumpConditionalParse(unsigned char* buffer, InstructionJump& jump, int myByte, bool isWide = false) {
		// Jump offsets are relative to the next instruction in bytes
		jump.condition = (InstructionJump::Condition)buffer[0];
		int jumpByteOffset = ParseImmediateData(&buffer[1], isWide);

		// This gives the byte, not the instruction index. We resolve this later in the decode function
		jump.byteOffset = jumpByteOffset;
		jump.byteLocation = myByte + jumpByteOffset + (isWide ? 3 : 2);
		jump.instructionIndex = -1;
		return (isWide ? 3 : 2);
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
	// Main
	//----------------------------------------------
	List<InstructionGeneric> Decode(Buffer& buffer) {
		List<InstructionGeneric> instructions;

		struct InstructionIndexToByte {
			int instructionIndex;
			int byte;
		};

		// Used for jump instructions
		List<InstructionIndexToByte> instructionIndexToByte;

		// Print-decode buffer
		for (int bp = 0; bp < buffer.size;)
		{
			OpCode code = ParseOpCode(buffer.data[bp], buffer.data[bp + 1]);
			int bytes = 0;
			InstructionGeneric instruction;
			switch (code) {
			case OpCode::MOVE_TOFROM_REGMEM:
				instruction.type = InstructionType::MOVE;
				bytes = OperationMoveToFromRegMemParse(&buffer.data[bp], instruction.move);
				break;
			case OpCode::MOVE_IMMEDIATE_TO_REGMEM:
				instruction.type = InstructionType::MOVE;
				bytes = OperationMoveImmediateToRegMemParse(&buffer.data[bp], instruction.move);
				break;
			case OpCode::MOVE_IMMEDIATE_TO_REG:
				instruction.type = InstructionType::MOVE;
				bytes = OperationMoveImmediateToRegisterParse(&buffer.data[bp], instruction.move);
				break;
			case OpCode::MOVE_MEMORY_TO_ACCUMULATOR:
				instruction.type = InstructionType::MOVE;
				bytes = OperationMoveMemoryToAccumulatorParse(&buffer.data[bp], instruction.move);
				break;
			case OpCode::MOVE_ACCUMULATOR_TO_MEMORY:
				instruction.type = InstructionType::MOVE;
				bytes = OperationMoveAccumulatorToMemoryParse(&buffer.data[bp], instruction.move);
				break;
			case OpCode::MOVE_REGMEM_TO_SEGMENT:
				instruction.type = InstructionType::MOVE;
				bytes = OperationMoveRegMemToSegmentRegisterParse(&buffer.data[bp], instruction.move);
				break;
			case OpCode::MOVE_SEGMENT_TO_REGMEM:
				instruction.type = InstructionType::MOVE;
				bytes = OperationMoveSegmentRegisterToRegMemParse(&buffer.data[bp], instruction.move);
				break;
			case OpCode::ADD_IMMEDIATE_TO_ACCUMULATOR:
				instruction.type = InstructionType::ADD;
				bytes = OperationAddImmediateToAccumulatorParse(&buffer.data[bp], instruction.add);
				break;
			case OpCode::ADD_TOFROM_REGMEM:
				instruction.type = InstructionType::ADD;
				bytes = OperationAddToFromRegMemParse(&buffer.data[bp], instruction.add);
				break;
			case OpCode::ADD_IMMEDIATE_TO_REGMEM:
				instruction.type = InstructionType::ADD;
				bytes = OperationAddImmediateToRegMemParse(&buffer.data[bp], instruction.add);
				break;
			case OpCode::SUB_TOFROM_REGMEM:
				instruction.type = InstructionType::SUB;
				bytes = OperationSubToFromRegMemParse(&buffer.data[bp], instruction.sub);
				break;
			case OpCode::SUB_IMMEDIATE_TO_REGMEM:
				instruction.type = InstructionType::SUB;
				bytes = OperationSubImmediateFromRegMemParse(&buffer.data[bp], instruction.sub);
				break;
			case OpCode::SUB_IMMEDIATE_TO_ACCUMULATOR:
				instruction.type = InstructionType::SUB;
				bytes = OperationSubImmediateFromAccumulatorParse(&buffer.data[bp], instruction.sub);
				break;
			case OpCode::CMP_REG_WITH_REGMEM:
				instruction.type = InstructionType::COMPARE;
				bytes = OperationCompareRegWithRegMemParse(&buffer.data[bp], instruction.compare);
				break;
			case OpCode::CMP_IMMEDIATE_WITH_REGMEM:
				instruction.type = InstructionType::COMPARE;
				bytes = OperationCompareImmediateWithRegMemParse(&buffer.data[bp], instruction.compare);
				break;
			case OpCode::CMP_IMMEDIATE_WITH_ACCUMULATOR:
				instruction.type = InstructionType::COMPARE;
				bytes = OperationCompareImmediateWithAccumulatorParse(&buffer.data[bp], instruction.compare);
				break;
			case OpCode::JUMP_ALWAYS_RELATIVE_WIDE:
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
					instruction.type = InstructionType::JUMP;
					bool isWide = (code == OpCode::JUMP_ALWAYS_RELATIVE_WIDE);
					bytes = OperationJumpConditionalParse(&buffer.data[bp], instruction.jump, bp, isWide);
				}
				break;
			case OpCode::INTERRUPT:
				instruction.type = InstructionType::INTERRUPT;
				instruction.interrupt.interruptNumber = buffer.data[bp + 1];
				bytes = 2;
				break;
			default:
				printf("ERROR WHILE DECODING: Unhandled opcode 0x%x\n", buffer.data[bp]);
				return {};
			}

			// instruction as string
			instruction.index = instructions.Size();
			instructionIndexToByte.Add({ instruction.index, bp });
			instructions.Add(instruction);
			bp += bytes;
		}

		// Resolve jump targets
		// Also idk why but the jump targets are off by one
		for (int i = 0; i < instructions.Size(); i++)
		{
			InstructionGeneric& instruction = instructions[i];
			if (instruction.type == InstructionType::JUMP) {
				bool resolved = false;
				for (int i = 0; i < instructionIndexToByte.Size(); i++)
				{
					if (instructionIndexToByte[i].byte == instruction.jump.byteLocation) {
						instruction.jump.instructionIndex = instructionIndexToByte[i].instructionIndex;
						resolved = true;
						break;
					}
				}
				if (!resolved) {
					printf("ERROR WHILE DECODING: Could not resolve jump target\n");
					return {};
				}
			}
		}

		// Stringify
		for (int i = 0; i < instructions.Size(); i++)
		{
			InstructionGeneric& instruction = instructions[i];
			instruction.asString = InstructionToString(instruction);
		}

		return instructions;
	}

}
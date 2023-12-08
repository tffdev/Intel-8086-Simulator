#include "StringifyTypes.h"
#include <stdio.h>
#include "Executor.h"

String RegisterToString(Register reg) {
	static String names[] = {
		"al", "cl", "dl", "bl",
		"ah", "ch", "dh", "bh",
		"ax", "cx", "dx", "bx",
		"sp", "bp", "si", "di",
		"cs", "ds", "ss", "es",
		"IP", "FLAGS", "INVALID"
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
		"INVALID"
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
		"[bp%s%i]",
		"[bx%s%i]",
		"[%i]", // Direct address
		"INVALID"
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
	if (s == ExplicitDataSize::BYTE) {
		return "byte ";
	}
	else {
		return "word ";
	}
}

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

String OperandToString(Operand const& o) {
	String prefix = "";
	switch (o.dataSize) {
	case ExplicitDataSize::WORD: prefix = "word "; break;
	case ExplicitDataSize::BYTE: prefix = "byte "; break;
	}

	String operation = "";
	switch (o.type) {
	case Operand::Type::REGISTER: operation = RegisterToString(o.reg); break;
	case Operand::Type::MEMORY_LOC: operation = EffectiveAddressWithOffsetToString(o.mem.effectiveAddress, o.mem.memoryOffset); break;
	case Operand::Type::IMMEDIATE: operation = String::Format("%i", o.immediate); break;
	case Operand::Type::NONE:
		printf("OPERATION HAS NO TYPE\n");
		return "";
	}

	return String::Format("%s%s", prefix.c_str(), operation.c_str());
}

void PrintMove(InstructionMove const& move) {
	printf("mov %s, %s", OperandToString(move.dest).c_str(), OperandToString(move.source).c_str());
}

void PrintAdd(InstructionAdd const& add) {
	printf("add %s, %s", OperandToString(add.dest).c_str(), OperandToString(add.source).c_str());
}

void PrintSub(InstructionSub const& sub) {
	printf("sub %s, %s", OperandToString(sub.dest).c_str(), OperandToString(sub.source).c_str());
}

void PrintCompare(InstructionCompare const& cmp) {
	printf("cmp %s, %s", OperandToString(cmp.dest).c_str(), OperandToString(cmp.source).c_str());
}

void PrintJump(InstructionJump const& jump) {
	printf("%s $%i", ConditionToString(jump.condition).c_str(), jump.byteOffset + 2);
}

String InstructionToString(InstructionGeneric const& inst) {
	switch (inst.type) {
	case InstructionType::MOVE: return String::Format("mov %s, %s", OperandToString(inst.move.dest).c_str(), OperandToString(inst.move.source).c_str());
	case InstructionType::ADD: return String::Format("add %s, %s", OperandToString(inst.add.dest).c_str(), OperandToString(inst.add.source).c_str());
	case InstructionType::SUB: return String::Format("sub %s, %s", OperandToString(inst.sub.dest).c_str(), OperandToString(inst.sub.source).c_str());
	case InstructionType::COMPARE: return String::Format("cmp %s, %s", OperandToString(inst.compare.dest).c_str(), OperandToString(inst.compare.source).c_str());
	case InstructionType::JUMP: return String::Format("%s %i", ConditionToString(inst.jump.condition).c_str(), inst.jump.instructionIndex);
	case InstructionType::INTERRUPT: return String::Format("int %i", inst.interrupt.interruptNumber);
	}
	return "INVALID INSTRUCTION STRING";
}

void PrintInstruction(InstructionGeneric& instruction) {
	printf(instruction.asString.c_str());
}

String FlagsToString(word flags) {
	String result = "_________";
	if (flags & CPU::Flags::CARRY) result.data[0] = 'C';
	if (flags & CPU::Flags::PARITY) result.data[1] = 'P';
	if (flags & CPU::Flags::AUX_CARRY) result.data[2] = 'A';
	if (flags & CPU::Flags::ZERO) result.data[3] = 'Z';
	if (flags & CPU::Flags::SIGN) result.data[4] = 'S';
	if (flags & CPU::Flags::OVERFLOW) result.data[5] = 'O';
	if (flags & CPU::Flags::DIRECTION) result.data[6] = 'D';
	if (flags & CPU::Flags::INTERRUPT) result.data[7] = 'I';
	if (flags & CPU::Flags::TRAP) result.data[8] = 'T';
	return result;
}
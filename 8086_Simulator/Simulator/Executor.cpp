#include "Executor.h"
#include <stdio.h>
#include "StringifyTypes.h"

CPU::CPU()
	: ax(0), cx(0), dx(0), bx(0), sp(0), bp(0), si(0), di(0), cs(0), ds(0), ss(0), es(0), ip(0), flags(0),
	loadedInstructions(), memory(nullptr), halted(false)
{
	this->memory = new byte[0xffff];

	// Initialize memory to 0
	for (int i = 0; i < 0xffff; ++i) {
		memory[i] = 0;
	}
}

void CPU::Reset() {
	ax = 0, cx = 0, dx = 0, bx = 0, sp = 0, bp = 0, si = 0, di = 0, cs = 0, ds = 0, ss = 0, es = 0, ip = 0, flags = 0;
	halted = false;
	loadedInstructions = List<InstructionGeneric>();
	for (int i = 0; i < 0xffff; ++i) {
		memory[i] = 0;
	}
}

void CPU::SetRegister(Register reg, word value) {
	switch (reg) {
		// We print the wide version of these registers
	case Register::AL: { al = (byte)value; break; }
	case Register::AH: { ah = (byte)value; break; }
	case Register::CL: { cl = (byte)value; break; }
	case Register::CH: { ch = (byte)value; break; }
	case Register::DL: { dl = (byte)value; break; }
	case Register::DH: { dh = (byte)value; break; }
	case Register::BL: { bl = (byte)value; break; }
	case Register::BH: { bh = (byte)value; break; }

	case Register::AX: { ax = value; break; }
	case Register::BX: { bx = value; break; }
	case Register::CX: { cx = value; break; }
	case Register::DX: { dx = value; break; }
	case Register::SP: { sp = value; break; }
	case Register::BP: { bp = value; break; }
	case Register::SI: { si = value; break; }
	case Register::DI: { di = value; break; }
	case Register::CS: { cs = value; break; }
	case Register::DS: { ds = value; break; }
	case Register::SS: { ss = value; break; }
	case Register::ES: { es = value; break; }
	case Register::IP: { ip = value; break; }
	}
}

word CPU::GetRegister(Register reg) {
	switch (reg) {
	case Register::AL: return al;
	case Register::AH: return ah;
	case Register::CL: return cl;
	case Register::CH: return ch;
	case Register::DL: return dl;
	case Register::DH: return dh;
	case Register::BL: return bl;
	case Register::BH: return bh;
	case Register::AX: return ax;
	case Register::BX: return bx;
	case Register::CX: return cx;
	case Register::DX: return dx;
	case Register::SP: return sp;
	case Register::BP: return bp;
	case Register::SI: return si;
	case Register::DI: return di;
	case Register::CS: return cs;
	case Register::DS: return ds;
	case Register::SS: return ss;
	case Register::ES: return es;
	case Register::IP: return ip;
	}
	return 0;
}

void CPU::SetMemory(EffectiveAddress addr, word offset, byte value) {
	int effectiveAddress = GetEffectiveAddress(addr);
	byte oldValue = memory[effectiveAddress + offset];
	memory[effectiveAddress + offset] = value;
}

void CPU::SetMemoryWide(EffectiveAddress addr, word offset, word value) {
	int effectiveAddress = GetEffectiveAddress(addr);
	memory[effectiveAddress + offset] = (byte)(value >> 8);
	memory[effectiveAddress + offset + 1] = (byte)value;
}

int CPU::GetEffectiveAddress(EffectiveAddress addr) {
	switch (addr) {
	case EffectiveAddress::BX_SI: return bx + si; break;
	case EffectiveAddress::BX_DI: return bx + di; break;
	case EffectiveAddress::BP_SI: return bp + si; break;
	case EffectiveAddress::BP_DI: return bp + di; break;
	case EffectiveAddress::SI: return si; break;
	case EffectiveAddress::DI: return di; break;
	case EffectiveAddress::BP: return bp; break;
	case EffectiveAddress::BX: return bx; break;
	}
	return 0;
}

byte CPU::GetMemory(EffectiveAddress addr, word offset) {
	// calculate effective address
	int effectiveAddress = GetEffectiveAddress(addr);
	return memory[effectiveAddress + offset];
}

word CPU::GetMemoryWide(EffectiveAddress addr, word offset) {
	int effectiveAddress = GetEffectiveAddress(addr);
	return memory[effectiveAddress + offset] << 8 | memory[effectiveAddress + offset + 1];
}

word CPU::GetData(Operand const& op) {
	switch (op.type) {
	case Operand::Type::IMMEDIATE:
		return op.immediate;
	case Operand::Type::MEMORY_LOC:
		return GetMemory(op.mem.effectiveAddress, (word)op.mem.memoryOffset);
	case Operand::Type::REGISTER:
		return GetRegister(op.reg);
	}
}

void CPU::SetData(Operand const& op, word value) {
	switch (op.type) {
	case Operand::Type::MEMORY_LOC:
		SetMemory(op.mem.effectiveAddress, (word)op.mem.memoryOffset, (byte)value);
		break;
	case Operand::Type::REGISTER:
		SetRegister(op.reg, (word)value);
		break;
	default:
		printf("Cannot set data for immediate value\n");
	}
}

CPU::~CPU() {
	delete[] memory;
}

void CPU::PrintFlags() {
	if (flags == 0) printf("_");
	if (flags & Flags::CARRY) printf("C");
	if (flags & Flags::PARITY) printf("P");
	if (flags & Flags::AUX_CARRY) printf("A");
	if (flags & Flags::ZERO) printf("Z");
	if (flags & Flags::SIGN) printf("S");
	if (flags & Flags::OVERFLOW) printf("O");
	if (flags & Flags::DIRECTION) printf("D");
	if (flags & Flags::INTERRUPT) printf("I");
	if (flags & Flags::TRAP) printf("T");
}

void CPU::SetFlags(word flags) {
	this->flags = flags;
}

bool CPU::GetFlag(Flags flag) {
	return (flags & flag) != 0;
}

bool CheckParity(int e) {
	int parity = 0;
	for (int i = 0; i < 8; i++) {
		if (e & 0x1) parity++;
		e >>= 1;
	}
	return (parity % 2) == 0;
}

bool CheckCarry(word a, word b) {
	unsigned int sum = (unsigned int)a + b;
	return (sum & 0xFFFF0000) != 0;
}

bool CheckCarryNegative(word a, word b) {
	unsigned int sum = (unsigned int)b - a;
	return (sum & 0xFFFF0000) != 0;
}

bool CheckAuxillery(word a, word b) {
	word carry_out = (a & 0xF) + (b & 0xF);
	return (carry_out > 0xF);
}

bool CheckAuxilleryNegative(word a, word b) {
	word carry_out = (b & 0xF) - (a & 0xF);
	return (carry_out > 0xF);
}

bool CheckOverflow(signed short a, signed short b) {
	int sum = (int)a + b;
	int maxWord = 32767;
	int minWord = -32768;
	return (sum > maxWord || sum < minWord);
}

bool CheckOverflowNegative(signed short a, signed short b) {
	int sum = (int)b - a;
	int maxWord = 32767;
	int minWord = -32768;
	return (sum > maxWord || sum < minWord);
}

bool CPU::ShouldJump(InstructionJump::Condition condition) {
	switch (condition) {
	case InstructionJump::Condition::JumpOnEqualOrZero: return GetFlag(Flags::ZERO);
	case InstructionJump::Condition::JumpOnLess: return GetFlag(Flags::SIGN);
	case InstructionJump::Condition::JumpOnLessOrEqual: return GetFlag(Flags::SIGN) || GetFlag(Flags::ZERO);
	case InstructionJump::Condition::JumpOnBelow: return GetFlag(Flags::SIGN);
	case InstructionJump::Condition::JumpOnBelowOrEqual: return GetFlag(Flags::SIGN) || GetFlag(Flags::ZERO);
	case InstructionJump::Condition::JumpOnParity: return GetFlag(Flags::PARITY);
	case InstructionJump::Condition::JumpOnOverflow: return GetFlag(Flags::OVERFLOW);
	case InstructionJump::Condition::JumpOnSign: return GetFlag(Flags::SIGN);
	case InstructionJump::Condition::JumpOnNotEqualOrZero: return !GetFlag(Flags::ZERO);
	case InstructionJump::Condition::JumpOnGreaterOrEqual: return GetFlag(Flags::ZERO) || !GetFlag(Flags::SIGN);
	case InstructionJump::Condition::JumpOnGreater: return !GetFlag(Flags::ZERO) && !GetFlag(Flags::SIGN);
	case InstructionJump::Condition::JumpOnAboveOrEqual: return GetFlag(Flags::ZERO) || !GetFlag(Flags::SIGN);
	case InstructionJump::Condition::JumpOnAbove: return !GetFlag(Flags::ZERO) && !GetFlag(Flags::SIGN);
	case InstructionJump::Condition::JumpOnNotParity: return !GetFlag(Flags::PARITY);
	case InstructionJump::Condition::JumpOnNotOverflow: return !GetFlag(Flags::OVERFLOW);
	case InstructionJump::Condition::JumpOnNotSign: return !GetFlag(Flags::SIGN);
	case InstructionJump::Condition::JumpOnCXZero: return (cx == 0);
	case InstructionJump::Condition::Loop: {
		cx -= 1;
		return !(cx == 0);
	}
	case InstructionJump::Condition::LoopEqualOrZero: {
		cx -= 1;
		return !(cx == 0) && GetFlag(Flags::ZERO);
	}
	case InstructionJump::Condition::LoopNotEqualOrZero: {
		cx -= 1;
		return !(cx == 0) && !GetFlag(Flags::ZERO);
	}
	}
	return false;
}

void CPU::Step() {
	if (halted) return;

	InstructionGeneric& instruction = loadedInstructions[ip];

	// TODO: Implement
	switch (instruction.type) {
	case InstructionType::MOVE: {
		int dat = GetData(instruction.move.source);
		SetData(instruction.move.dest, dat);
		break;
	}
	case InstructionType::ADD: {
		word sourceData = GetData(instruction.add.source);
		word destData = GetData(instruction.add.dest);
		word finalData = destData + sourceData;
		SetData(instruction.add.dest, finalData);
		word flags = 0;
		flags |= (finalData == 0) ? Flags::ZERO : 0;
		flags |= (finalData & 0x8000) ? Flags::SIGN : 0;
		flags |= CheckParity(finalData) ? Flags::PARITY : 0;
		flags |= CheckCarry(sourceData, destData) ? Flags::CARRY : 0;
		flags |= CheckOverflow(sourceData, destData) ? Flags::OVERFLOW : 0;
		flags |= CheckAuxillery(sourceData, destData) ? Flags::AUX_CARRY : 0;
		SetFlags(flags);
		break;
	}
	case InstructionType::SUB: {
		word sourceData = GetData(instruction.add.source);
		word destData = GetData(instruction.add.dest);
		word finalData = destData - sourceData;
		SetData(instruction.add.dest, finalData);
		word flags = 0;
		flags |= (finalData == 0) ? Flags::ZERO : 0;
		flags |= (finalData & 0x8000) ? Flags::SIGN : 0;
		flags |= CheckParity(finalData) ? Flags::PARITY : 0;
		flags |= CheckCarryNegative(sourceData, destData) ? Flags::CARRY : 0;
		flags |= CheckOverflowNegative(sourceData, destData) ? Flags::OVERFLOW : 0;
		flags |= CheckAuxilleryNegative(sourceData, destData) ? Flags::AUX_CARRY : 0;
		SetFlags(flags);
		break;
	}
	case InstructionType::COMPARE: {
		word sourceData = GetData(instruction.add.source);
		word destData = GetData(instruction.add.dest);
		word finalData = destData - sourceData;
		word flags = 0;
		flags |= (finalData == 0) ? Flags::ZERO : 0;
		flags |= (finalData & 0x8000) ? Flags::SIGN : 0;
		flags |= CheckParity(finalData) ? Flags::PARITY : 0;
		flags |= CheckCarryNegative(sourceData, destData) ? Flags::CARRY : 0;
		flags |= CheckOverflowNegative(sourceData, destData) ? Flags::OVERFLOW : 0;
		flags |= CheckAuxilleryNegative(sourceData, destData) ? Flags::AUX_CARRY : 0;
		SetFlags(flags);
		break;
	}
	case InstructionType::JUMP: {
		// If the condition is met, we set the instruction pointer to the address
		if (ShouldJump(instruction.jump.condition)) {
			ip = instruction.jump.instructionIndex;
			return;
		}
		break;
	}
	case InstructionType::INTERRUPT: {
		break;
	}
	}
	ip++;

	if (ip >= loadedInstructions.Size()) {
		halted = true;
	}
}

void CPU::PrintState() {
	printf("CPU register states:\n");
	printf("AX: 0x%04x (%i)\n", ax, ax);
	printf("BX: 0x%04x (%i)\n", bx, bx);
	printf("CX: 0x%04x (%i)\n", cx, cx);
	printf("DX: 0x%04x (%i)\n", dx, dx);
	printf("SP: 0x%04x (%i)\n", sp, sp);
	printf("BP: 0x%04x (%i)\n", bp, bp);
	printf("SI: 0x%04x (%i)\n", si, si);
	printf("DI: 0x%04x (%i)\n", di, di);
	printf("~\n");
	printf("ES: 0x%04x (%i)\n", es, es);
	printf("SS: 0x%04x (%i)\n", ss, ss);
	printf("DS: 0x%04x (%i)\n", ds, ds);
	printf("CS: 0x%04x (%i)\n", cs, cs);
	printf("IP: 0x%04x (%i)\n", ip, ip);

	printf("Flags: ");
	PrintFlags();
	printf("\n");
}
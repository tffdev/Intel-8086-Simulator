#include "Types.h"
#include "String.h"

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
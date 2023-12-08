#pragma once
#include "Types.h"
#include "List.h"

class CPU {
public:
	enum Flags : word {
		NONE = 0,
		SIGN = 1,
		ZERO = 2,
		AUX_CARRY = 4,
		PARITY = 8,
		CARRY = 16,
		OVERFLOW = 32,
		DIRECTION = 64,
		INTERRUPT = 128,
		TRAP = 256,
	};

	CPU();
	~CPU();

	void Reset();

	void Step();
	inline void LoadInstructions(List<InstructionGeneric>& instructions) { loadedInstructions = instructions; }
	inline bool IsHalted() { return halted; }

	// Data access
	word GetData(Operand const& op);
	void SetData(Operand const& op, word value);

	void SetRegister(Register reg, word value);
	word GetRegister(Register reg);

	void SetMemory(EffectiveAddress addr, word offset, byte value);
	void SetMemoryWide(EffectiveAddress addr, word offset, word value);
	byte GetMemory(EffectiveAddress addr, word offset);
	word GetMemoryWide(EffectiveAddress addr, word offset);

	void PrintState();
	void PrintFlags();
	int GetEffectiveAddress(EffectiveAddress addr);

	void SetFlags(word flags);
	bool GetFlag(Flags f);

	bool ShouldJump(InstructionJump::Condition condition);

	List<InstructionGeneric> loadedInstructions;

	// Stored in a union to let short and wide registers overlap
	union {
		struct { word ax, cx, dx, bx; };
		struct { byte al, ah, cl, ch, dl, dh, bl, bh; };
	};

	word sp, bp, si, di;
	word cs, ds, ss, es;
	word ip;

	// Flags
	word flags;

	// Memory 
	byte* memory;
	bool halted;
};
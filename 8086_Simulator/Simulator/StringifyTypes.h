#pragma once
#include "String.h"
#include "Types.h"

String RegisterToString(Register reg);
String EffectiveAddressToString(EffectiveAddress addr);
String EffectiveAddressWithOffsetToString(EffectiveAddress addr, int offset);
String DataSizeToString(ExplicitDataSize s);
String ConditionToString(InstructionJump::Condition cond);

String OperandToString(Operand const& o);
void PrintMove(InstructionMove const& move);
void PrintAdd(InstructionAdd const& add);
void PrintSub(InstructionSub const& sub);
void PrintCompare(InstructionCompare const& cmp);
void PrintJump(InstructionJump const& jump);

void PrintInstruction(InstructionGeneric& instruction);
String InstructionToString(InstructionGeneric const& inst);
String FlagsToString(word flags);
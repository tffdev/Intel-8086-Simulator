#pragma once
#include "Types.h"

void Decompile(Buffer& buffer);
bool DecodeInstruction(DecodeContext& ctx, NewInstruction& instruction);
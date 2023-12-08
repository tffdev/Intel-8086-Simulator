#pragma once
#include "Types.h"
#include "List.h"

namespace Decoder {
	List<InstructionGeneric> Decode(Buffer& buffer);
}
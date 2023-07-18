#pragma once
#include "String.h"
#include "Types.h"

String RegisterToString(Register reg);
String EffectiveAddressToString(EffectiveAddress addr);
String EffectiveAddressWithOffsetToString(EffectiveAddress addr, int offset);
String DataSizeToString(ExplicitDataSize s);
String RegMemToString(RegMem& r);
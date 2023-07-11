// Creator: Daniel B.
// Date: 2023-07-06
// This file was made for Casey Muratori's 8086 homework segment for the Computer Enhance course
// on performance programming. For the sake of keeping things straight forward and easy to debug and extend, 
// this file is written in a bare-bones and verbose C style.
// All magic numbers used are from the Intel 8086 manual
// which can be found here: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf


/*
--------------------------------------------------
TODO:
--------------------------------------------------
[x] Fix weird register address bug
[ ] Implement all required opcodes
[ ] Convert all operations to classes
[ ] Use a "parsing context" rather than rough return logic
[ ] Figure out way to rid of huge switch statement at bottom
*/

#include <stdio.h>
#include "String.h"
#include "Types.h"
#include "DecoderAlternate.h"

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
void Decompile(Buffer& buffer) {
	printf(";Executable read successfully\n");
	printf(";Size: %d\n", buffer.size);
	printf(";Decompiled instructions: \n");
	printf("bits 16\n");

	// Print-decode buffer
	DecodeContext ctx;
	ctx.bp = 0;
	ctx.buffer = buffer.data;
	while(ctx.bp < buffer.size)
	{
		Instruction inst{};
		bool success = DecodeInstruction(ctx, inst);
		if (!success) {
			return;
		}
		else {
			printf("successfully decoded instruction: mov %s, %s\n", RegMemToString(inst.dest).c_str(), RegMemToString(inst.source).c_str());
		}
	}
}

int main(int argc, char* argv[]) {
	// Parse command line arguments
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	Buffer buffer = LoadBufferFromFile(argv[1]);
	Decompile(buffer);
	return 0;
}






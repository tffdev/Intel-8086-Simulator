// Creator: Daniel B.
// Date: 2023-07-06
// This file was made for Casey Muratori's 8086 homework segment for the Computer Enhance course
// on performance programming. For the sake of keeping things straight forward and easy to debug and extend, 
// this file is written in a bare-bones and verbose C style.
// All magic numbers used are from the Intel 8086 manual
// which can be found here: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf
#include <stdio.h>
#include <raylib.h>
#include "rlImgui/rlImGui.h"

#include "Types.h"
#include "StringifyTypes.h"
#include "Decoder.h"
#include "Executor.h"

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

int main(int argc, char* argv[]) {
	// Parse command line arguments
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	printf("Executing program and printing trace\n");
	printf("--------------------\n");

	CPU executor;
	{
		Buffer buffer = LoadBufferFromFile(argv[1]);
		List<InstructionGeneric> instructions = Decoder::Decode(buffer);
		executor.LoadInstructions(instructions);
	}

	// init raylib
	InitWindow(800, 600, "8086 Simulator");
	rlImGuiSetup(true);
	bool running = false;
	int executionsPerFrame = 100;

	// Make new empty texture
	Image renderImg = GenImageColor(64, 64, RED);
	Texture2D renderTexture = LoadTextureFromImage(renderImg);

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);

		// Draw cpu memory as an image
		UpdateTexture(renderTexture, &executor.memory[0x00f0]);
		DrawRectangle(420-1, 20-1, 4.7 * 64 + 3, 4.7 * 64 + 3, DARKGRAY);
		DrawTextureEx(renderTexture, { 420, 20 }, 0.0f, 4.7f, WHITE);

		rlImGuiBegin();

		if (ImGui::Begin("8086 Simulator")) {
			// Control
			if (ImGui::Button("Run")) running = true; ImGui::SameLine();
			if (ImGui::Button("Stop")) running = false; ImGui::SameLine();
			if (ImGui::Button("Step") && !executor.IsHalted()) executor.Step();
			if (ImGui::Button("Reload")) {
				executor.Reset();
				running = false;
				Buffer buffer = LoadBufferFromFile(argv[1]);
				List<InstructionGeneric> instructions = Decoder::Decode(buffer);
				executor.LoadInstructions(instructions);
			}

			ImGui::InputInt("Steps", &executionsPerFrame);

			ImGui::Separator();

			// Show instructions
			for (int i = 0; i < executor.loadedInstructions.Size(); i++) {
				InstructionGeneric& instruction = executor.loadedInstructions[i];
				ImGui::Text("%2i:", i); ImGui::SameLine();
				ImGui::Selectable(instruction.asString.c_str(), i == executor.ip);
			}

			ImGui::End();
		}

		// CPU state
		if (ImGui::Begin("CPU State")) {
			// Table containing all registers
			ImGui::Columns(2, "Registers", true);
			ImGui::Separator();
			ImGui::Text("Register"); ImGui::NextColumn();
			ImGui::Text("Value"); ImGui::NextColumn();
			ImGui::Separator();
			ImGui::Text("AX"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.ax); ImGui::NextColumn();
			ImGui::Text("BX"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.bx); ImGui::NextColumn();
			ImGui::Text("CX"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.cx); ImGui::NextColumn();
			ImGui::Text("DX"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.dx); ImGui::NextColumn();
			ImGui::Text("SI"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.si); ImGui::NextColumn();
			ImGui::Text("DI"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.di); ImGui::NextColumn();
			ImGui::Text("BP"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.bp); ImGui::NextColumn();
			ImGui::Text("SP"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.sp); ImGui::NextColumn();

			ImGui::Text("IP"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.ip); ImGui::NextColumn();
			ImGui::Text("CS"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.cs); ImGui::NextColumn();
			ImGui::Text("DS"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.ds); ImGui::NextColumn();
			ImGui::Text("ES"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.es); ImGui::NextColumn();
			ImGui::Text("SS"); ImGui::NextColumn(); ImGui::Text("0x%04X", executor.ss); ImGui::NextColumn();
			ImGui::Columns(1);

			ImGui::Text("Flags: %s", FlagsToString(executor.flags).c_str());

			if (executor.halted) ImGui::Text("Halted");

			ImGui::End();
		}

		// Memory
		if (ImGui::Begin("Memory")) {
			// pack row of memory into one string
			for (size_t i = 0; i < 100; i++)
			{
				int offset = i * 24;
				ImGui::Text("%4i: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", offset,
					executor.memory[offset + 0], executor.memory[offset + 1], executor.memory[offset + 2], executor.memory[offset + 3],
					executor.memory[offset + 4], executor.memory[offset + 5], executor.memory[offset + 6], executor.memory[offset + 7],
					executor.memory[offset + 8], executor.memory[offset + 9], executor.memory[offset + 10], executor.memory[offset + 11],
					executor.memory[offset + 12], executor.memory[offset + 13], executor.memory[offset + 14], executor.memory[offset + 15],
					executor.memory[offset + 16], executor.memory[offset + 17], executor.memory[offset + 18], executor.memory[offset + 19],
					executor.memory[offset + 20], executor.memory[offset + 21], executor.memory[offset + 22], executor.memory[offset + 23]
				);
			}
			ImGui::End();
		}

		if (running) {
			for (size_t i = 0; i < executionsPerFrame; i++)
			{
				executor.Step();
			}
		}

		rlImGuiEnd();
		EndDrawing();
	}

	rlImGuiShutdown();

	printf("--------------------\n");
	executor.PrintState();
	
	return 0;
}






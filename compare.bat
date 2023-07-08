nasm.exe test.asm -o test_source_bin
"8086_Simulator/x64/Debug/Simulator.exe" test_source_bin > test_decompiled.asm
nasm.exe test_decompiled.asm -o test_target_bin
fc test_source_bin test_target_bin
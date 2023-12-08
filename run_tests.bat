rem 1. Compile the test suite to binary, then compile the decompiled test suite to binary, then compare the two binaries

nasm.exe Testing/full_test_suite.asm -o Testing/test_suite_binary_real
"8086_Simulator/x64/Debug/Simulator.exe" Testing/test_suite_binary_real > Testing/test_suite_decompiled.asm
nasm.exe Testing/test_suite_decompiled.asm -o Testing/test_suite_binary_recreation
fc Testing/test_suite_binary_real Testing/test_suite_binary_recreation
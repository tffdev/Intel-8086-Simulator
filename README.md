# Intel 8086 Simulator

![](https://github.com/tffdev/Intel-8086-Simulator/blob/master/Assets/lowquality.gif?raw=true)

As a practical exercise to do alongside Casey Muratori's series on performance programming, I developed a CPU simulator that supports a reduced instruction set.
This simulator is capable of running a compiled executable or decompiling the executable back to x86 assembly.
The goal I set for myself was to make it render something to a small framebuffer.

# Features
This simulator supports a reduced instruction x86 set. Here are the following instructions this simulator supports:

**Arithmetic:** `mov`, `add`, `sub`, `cmp`

**Branching:** `jmp`, `jnz`, `je`, `jl`, `jle`, `jb`, `jbe`, `jp`, `jo`, `js`, `jge`, `jg`, `jae`, `ja`, `jnp`, `jno`, `jns`, `loop`, `loope`, `loopne`, `jcxz`

You can `mov` to 16 bit registers (`AX`, `BX`, `CX`, `DX`) or use them as multiple 8 bit registers (`AL`, `AH`, `BL`, `BH`, etc.) 
It supports moving to/from registers, memory, the accumulator, and segment registers.

***Please see `Testing/full_test_suite.asm` for a full example of what this simulator and decompiler supports***

**Visualiser:** The UI interprets memory location `0x00f0` onwards as a 64x64 framebuffer and will display on screen.
It will interpret every set of 3 bytes as RGB values.

# Usage
This program currently runs exclusively as a UI. To run a program, you should compile your x86 assembly with `nasm` making
sure you've only used the reduced instruction set, and run the program in the simulator using this arg syntax:

```
8086_Simulator.exe program.asm
```

# Testing
This simulator is tested using an `.asm` file which contains all supported instructions. 
`run_tests.bat` compiles `Testing/full_test_suite.asm` using nasm, loads the binary into the simulator, and saves out the decompilation.
It then re-compiles this decompilation using `nasm` and then compares the original binary to the new binary created from the decompilation.

We know if this simulator's decompiler works well if the two compilations produce completely identical binary.

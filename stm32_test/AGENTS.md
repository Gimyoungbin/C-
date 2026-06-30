# AGENTS.md

- This is a minimal STM32CubeIDE-for-VS-Code CMake project for STM32F103RBT6 / Cortex-M3, not a normal host C/C++ app.
- Use `cube-cmake`, not plain `cmake`; `.vscode/settings.json` wires CMake Tools to `cube-cmake` and this environment may not have `cmake` on PATH.
- Configure/build Debug with: `cube-cmake --preset Debug` then `cube-cmake --build --preset Debug`.
- Release uses the matching preset pair: `cube-cmake --preset Release` then `cube-cmake --build --preset Release`.
- Ninja output goes under `build/<preset>/`; `build` is ignored and should not be edited or committed.
- The firmware target is `stm32_test`; post-build emits `.elf`, `.hex`, `.bin`, `.map`, and memory usage in `build/<preset>/`.
- `CMakeLists.txt` is the user-editable build file; `cmake/vscode_generated.cmake` is generated and explicitly says not to edit it.
- Add application sources in `CMakeLists.txt` `sources_SRCS`; generated startup/syscall/sysmem sources are added from `cmake/vscode_generated.cmake`.
- `Src/startup_stm32f103xx.S` calls `SystemInit` before C runtime init and `main`; keep a `SystemInit` definition available unless replacing the startup flow.
- `stm32f103xb_flash.ld` defines STM32F103xB memory as 128K FLASH and 20K RAM; heap/stack defaults are `_Min_Heap_Size = 0x200`, `_Min_Stack_Size = 0x400`.
- Current `Src/main.c` is bare-metal register code: HSI 8 MHz, PA5 LED output, SysTick polling delay; there is no HAL/CMSIS device header dependency in the app code.
- Toolchain assumptions come from `CMakePresets.json` and `cmake/gnu-tools-for-stm32.cmake`: `arm-none-eabi-*`, Cortex-M3, no FPU, C11, C++20, `-Wall -Wextra -Wpedantic`, Debug `-O0 -g3 -ggdb`, Release `-Os`.

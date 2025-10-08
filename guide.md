---

# Quick Guide — Compile NASM + C (pthreads) and generate a `.exe` on Windows (MSYS2 / MinGW64)

## 1) Install / update MSYS2 (if not already installed)

1. Download and install MSYS2: [https://www.msys2.org/](https://www.msys2.org/)
2. Open **MSYS2 MSYS** terminal and run (to update the system):

```bash
pacman -Syu
# close the terminal and reopen "MSYS2 MSYS" if prompted
pacman -Su
```

---

## 2) Install MinGW64 toolchain, NASM, and winpthreads

Open the **MSYS2 MinGW 64-bit** terminal and run:

```bash
# basic toolchain
pacman -S --needed mingw-w64-x86_64-toolchain

# NASM assembler
pacman -S --needed mingw-w64-x86_64-nasm

# pthreads implementation for MinGW-w64
pacman -S --needed mingw-w64-x86_64-winpthreads
```

> Always use the **MinGW 64-bit terminal** to compile Windows 64-bit applications.

---

## 3) Compile the Assembly (NASM)

In your project folder:

```bash
nasm -f win64 src/disk_assembler.asm -o disk_assembler.o
```

Check the `.o` file:

```bash
ls -l disk_assembler.o
file disk_assembler.o
# should show "x86-64 COFF object file" or similar
```

---

## 4) Compile + link C + Assembly into a `.exe` (MinGW64 GCC)

In the same MinGW64 terminal:

```bash
/mingw64/bin/gcc src/main.c src/scanner.c disk_assembler.o -o diskscout.exe -O3 -lpthread
```

Notes:

* Use `/mingw64/bin/gcc` to make sure you are using the MinGW64 GCC.
* `-lpthread` works after installing `mingw-w64-x86_64-winpthreads`.
* You can omit the full path if `which gcc` points to `/mingw64/bin/gcc`.

Check the `.exe`:

```bash
ls -l diskscout.exe
file diskscout.exe
# should show: "PE32+ executable (console) x86-64, for MS Windows"
```

---

## 5) Run in PowerShell / CMD

In PowerShell (outside MSYS2):

```powershell
cd C:\Users\natan\documents\github\diskscout
.\diskscout.exe <path_to_scan>
```

In bash (MinGW64) use:

```bash
./diskscout.exe
```

---

## 6) Single command (NASM + GCC) alternative

You can compile NASM and C in one line:

```bash
nasm -f win64 src/disk_assembler.asm -o disk_assembler.o && /mingw64/bin/gcc src/main.c src/scanner.c disk_assembler.o -o diskscout.exe -O3 -lpthread
```

---

## 7) Quick debugging (if `.exe` is not generated)

* Check which GCC is being used:

```bash
which gcc
# should point to /mingw64/bin/gcc
```

* Check `disk_assembler.o` format:

```bash
file disk_assembler.o
```

* Try compiling only the C code to verify the toolchain:

```bash
/mingw64/bin/gcc src/main.c src/scanner.c -o test.exe
ls -l test.exe
file test.exe
```

* If `test.exe` works, the problem is with `.o` or unresolved assembly symbols. Check your `extern`/`global` between C and ASM.

---

## 8) Notes & best practices

* Always use `.exe` extension for Windows execution (PowerShell / Explorer).
* If linking fails silently, run GCC with `-v` to see what the linker complains about:

```bash
/mingw64/bin/gcc -v src/main.c src/scanner.c disk_assembler.o -o diskscout.exe -O3 -lpthread
```

* For repeated builds, consider creating a small `Makefile` or `build.sh` script to simplify compilation.

---

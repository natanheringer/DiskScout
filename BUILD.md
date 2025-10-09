## DiskScout — Windows 11 Build Guide (Personal Recommendation)

This is a concise, reliable checklist to set up the toolchain, build DiskScout (CLI and GUI), and run it on Windows 11 using MSYS2 MinGW64. It mirrors the exact commands you used in your screenshots.

---

### TL;DR cheat sheet

1) Install MSYS2 x86_64 from `https://www.msys2.org`.
2) Open “MSYS2 MinGW x64” shell.
3) Update and install toolchain, Qt, and tools:
```bash
pacman -Syu
pacman -Syu
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-qt5 \
                  mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
                  base-devel git nasm
```
4) Backend (CLI) quick build (from repo root):
```bash
nasm -f win64 src/disk_assembler.asm -o disk_assembler.o
gcc src/main.c src/scanner.c src/cache.c disk_assembler.o -o diskscout.exe -O3 -lpthread
```
5) GUI build (qmake route, from `gui/`):
```bash
qmake DiskScoutGUI.pro
make -j$(nproc)   # or: mingw32-make -j$(nproc)
./release/DiskScoutGUI.exe
```

---

### 1) Install prerequisites (Windows 11)

- MSYS2 x86_64: download and install from `https://www.msys2.org`.
- During install, keep default path `C:\msys64`.

Open the Start Menu → MSYS2 64bit → “MSYS2 MinGW x64”. This shell provides the MinGW-w64 GCC, Qt, and tools we need.

Update and install packages:
```bash
pacman -Syu              # let it finish; close/reopen if asked
pacman -Syu              # run again to ensure full sync

# Toolchain + Qt + build tools + assembler
pacman -S --needed \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-qt5 \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  base-devel git nasm
```

Optional (use Qt 6 instead of Qt 5):
```bash
pacman -S --needed mingw-w64-x86_64-qt6
```

---

### 2) Open the MinGW shell (or use it from cmd)

- Recommended: Start Menu → MSYS2 MinGW x64
- From cmd, launching the same shell:
```cmd
"C:\msys64\usr\bin\bash.exe" -lc "echo Using MinGW64 shell"
```
- Use MinGW tools directly from cmd (temporary PATH):
```cmd
set PATH=C:\msys64\mingw64\bin;%PATH%
where g++ && where qmake && where cmake
```

---

### 3) Get the source and verify

```bash
cd /c/Users/Natan/Documents/GitHub/DiskScout
git status   # optional
```

---

### 4) Build the backend (CLI) — mirrors your screenshot

From the repository root (`/c/Users/Natan/Documents/GitHub/DiskScout`):
```bash
nasm -f win64 src/disk_assembler.asm -o disk_assembler.o
gcc src/main.c src/scanner.c src/cache.c disk_assembler.o -o diskscout.exe -O3 -lpthread
```
Run it:
```bash
./diskscout.exe C:/some/path
```

Notes:
- `gcc` is the MinGW-w64 GCC in the MinGW shell PATH.
- If you prefer an explicit path, `which gcc` will show `/mingw64/bin/gcc`.

---

### 5) Build the GUI — Option A: qmake + make (matches your screenshot)

From the `gui/` directory:
```bash
cd /c/Users/Natan/Documents/GitHub/DiskScout/gui
qmake DiskScoutGUI.pro
make -j$(nproc)          # or: mingw32-make -j$(nproc)
./release/DiskScoutGUI.exe
```

Where the executable typically lands: `gui/release/DiskScoutGUI.exe`.

If `make` is not found, either install it (`pacman -S make`) or use `mingw32-make` which is part of the MinGW toolchain package.

Important: Ensure `../src/main.c` is NOT included in the GUI target (to avoid a duplicate `main`). The `.pro` already excludes it.

---

### 6) Build the GUI — Option B: CMake + Ninja (alternative)

This builds the whole project with CMake, auto-detecting Qt in the MinGW environment.
```bash
cd /c/Users/Natan/Documents/GitHub/DiskScout
mkdir -p build && cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -S .. -B .
cmake --build . -j
./gui/DiskScoutGUI.exe
```

If CMake cannot find Qt, confirm the MinGW64 shell is used and `mingw-w64-x86_64-qt5` is installed. As a fallback you can set `CMAKE_PREFIX_PATH="/mingw64"`.

---

### 7) Run the GUI from Windows cmd

Either run inside the MinGW shell as above, or set PATH before launching:
```cmd
set PATH=C:\msys64\mingw64\bin;%PATH%
C:\Users\Natan\Documents\GitHub\DiskScout\gui\release\DiskScoutGUI.exe
```

For CMake builds:
```cmd
set PATH=C:\msys64\mingw64\bin;%PATH%
C:\Users\Natan\Documents\GitHub\DiskScout\build\gui\DiskScoutGUI.exe
```

---

### 8) Troubleshooting (Windows 11 specifics)

- qmake not found:
  - Install Qt 5: `pacman -S mingw-w64-x86_64-qt5`
  - Verify: `qmake --version`

- make/mingw32-make not found:
  - `pacman -S make` or use `mingw32-make` which ships with the MinGW toolchain.

- “QPA platform plugin could not be initialized” when running the GUI:
  - Run from the MinGW shell or ensure `C:\msys64\mingw64\bin` is on PATH at runtime so Qt DLLs are found.

- Duplicate `main` when building the GUI:
  - Ensure `src/main.c` is not compiled into the GUI target (it’s for CLI only). The provided `.pro` already avoids this.

- Qt source build errors (e.g., `qglobal.h` path failures):
  - Do not build Qt from source for this project; use MSYS2 prebuilt Qt (`mingw-w64-x86_64-qt5`).

- Assembly not compiling:
  - Ensure `nasm` is installed. Example manual build is shown in section 4.

---

### 9) One-liners you can paste

Full rebuild + run (qmake):
```bash
cd /c/Users/Natan/Documents/GitHub/DiskScout/gui && \
qmake DiskScoutGUI.pro && \
make -j$(nproc) && \
./release/DiskScoutGUI.exe
```

Full rebuild + run (CMake/Ninja):
```bash
cd /c/Users/Natan/Documents/GitHub/DiskScout && \
rm -rf build && mkdir build && cd build && \
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -S .. -B . && \
cmake --build . -j && \
./gui/DiskScoutGUI.exe
```

Backend-only quick build:
```bash
cd /c/Users/Natan/Documents/GitHub/DiskScout && \
nasm -f win64 src/disk_assembler.asm -o disk_assembler.o && \
gcc src/main.c src/scanner.c src/cache.c disk_assembler.o -o diskscout.exe -O3 -lpthread && \
./diskscout.exe
```

---

### 10) What to remember

- Always use the “MSYS2 MinGW x64” shell for consistent PATH and Qt discovery.
- Prefer Qt 5 via MSYS2 packages for this project.
- For GUI: qmake + make is the shortest path; CMake + Ninja is a good alternative.
- If anything fails, first verify the shell (`echo $MSYSTEM` should be `MINGW64`) and that `qmake`, `gcc`, `nasm` are present.



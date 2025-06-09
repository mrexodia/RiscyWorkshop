# payload

## Build (macos)

Install `lld` and `llvm` via homebrew:

```sh
brew install lld llvm
```

Configure the build:

```sh
cmake -B build --toolchain ~/zig-cross/x86_64-windows-gnu.cmake -DCLANG_EXECUTABLE=$(brew --prefix llvm)/bin/clang -DLLD_EXECUTABLE=$(brew --prefix lld)/bin/ld.lld -DOBJCOPY_EXECUTABLE=$(brew --prefix llvm)/bin/llvm-objcopy
```

## Build (Windows)

Install the LLVM toolset in with the Visual Studio Installer and use the following command line to configure:

```sh
cmake -B build -T ClangCL -DCLANG_EXECUTABLE=D:\CodeBlocks\llvm-project-19.1.6.src\install\bin\clang.exe -DLLD_EXECUTABLE=D:\CodeBlocks\llvm-project-19.1.6.src\install\bin\ld.lld.exe -DOBJCOPY_EXECUTABLE=D:\CodeBlocks\llvm-project-19.1.6.src\install\bin\llvm-objcopy.exe
```

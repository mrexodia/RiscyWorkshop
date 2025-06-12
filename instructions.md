Build the transpiler:

```sh
cmake -B transpiler/build -S transpiler
cmake --build transpiler/build
```

Build riscvm:

```sh
cmake -S riscvm --preset tracing
cmake --build riscvm/build
```

Build the payload:

```sh
cmake -S payload --preset default
cmake --build payload/build
```

Run the payload:

```sh
wine riscvm/build/riscvm.exe payload/build/hello.bin
```

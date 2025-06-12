# Transpiler

**For this exercise you will run commands from the root directory.**

Build `riscvm` for Windows:

```sh
cmake -S riscvm --preset tracing
cmake --build riscvm/build
```

Run the output in Wine (or run it on your Windows host/VM):

```sh
wine riscvm/build/riscvm.exe
```

Build the payload:

```sh
cmake -S payload --preset default
cmake --build payload/build
```

Exercises:
1. Run the `hello` payload (`hello.bin`)
2. Run the `msgbox` payload (port 8080 is forwarded to a NoVNC instance in the Codespace)
3. Run the `c2test` payload in the C2 demo from the beginning

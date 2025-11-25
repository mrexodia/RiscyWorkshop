Build the transpiler:

```sh
cmake -B transpiler/build -S transpiler
cmake --build transpiler/build
```

Build riscvm (we enable hardening for the demo):

```sh
cmake -S riscvm --preset hardening
cmake --build riscvm/build
```

Build the payload:

```sh
cmake -S payload --preset default
cmake --build payload/build
```

Run the c2 server in one terminal tab:

```sh
wine riscvm/build/c2.exe
```

Then in another tab run the payloads (while explaining the sources):

```sh
cd payload
./post-c2.py ./build/hello.enc.bin
./post-c2.py ./build/msgbox.enc.bin
./post-c2.py ./build/c2test.enc.bin
```

---

Build riscvm in tracing mode:

```sh
cmake -S riscvm --preset tracing
cmake --build riscvm/build
```

Running a payload standalone:

```sh
wine riscvm/build/riscvm.exe payload/build/hello.bin
```

This should output:

```
no features in the file (unencrypted payload?)
Hello from RISC-V!
exit code: 0
```

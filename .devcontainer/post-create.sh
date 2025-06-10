#!/bin/bash
wine .devcontainer/hello.exe
cmake -B transpiler/build -S transpiler
cmake --build transpiler/build
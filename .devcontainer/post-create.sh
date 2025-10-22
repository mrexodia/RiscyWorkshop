#!/bin/bash
wine .devcontainer/hello.exe
cmake -B transpiler/build -S transpiler
cmake --build transpiler/build

# Shell prompt - only add if not already present
grep -qF "PS1='\$(if [[ \"\$PWD\" == /workspaces/* ]];" ~/.bashrc || echo "PS1='\$(if [[ \"\$PWD\" == /workspaces/* ]]; then realpath --relative-to=/workspaces \"\$PWD\"; else echo \"\\w\"; fi)\\$ '" >> ~/.bashrc

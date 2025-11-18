SFML Memory Matching Game
===========================

Structure
---------
MemoryMatchSFML/
  ├─ src/
  │   └─ main.cpp
  ├─ .vscode/
  │   └─ tasks.json
  └─ bin/            (created after build)

Requirements
-----------
- Linux (e.g. Ubuntu)
- g++
- SFML development package:

    sudo apt update
    sudo apt install -y libsfml-dev fonts-dejavu-core

Build and Run (terminal)
------------------------
From inside the MemoryMatchSFML folder:

    mkdir -p bin
    g++ src/main.cpp -o bin/memory_match \
       -std=c++17 -lsfml-graphics -lsfml-window -lsfml-system

Then run:

    ./bin/memory_match

Build and Run (VS Code)
-----------------------
1. Open the MemoryMatchSFML folder in VS Code.
2. Press **Ctrl+Shift+B** to build.
3. The executable will be at `bin/memory_match`.
4. Run it from the terminal:

    ./bin/memory_match


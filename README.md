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
DETECTIVE ECLISPE
FINAL PROJECT REPORT

Submitted by :
 Amna Shahzad (24018)
Bisma shahid (24016)

Submitted to 
Miss Samia Masood 
Course:
Data structure and algorithms 
Department of CSIT, NEDUET
 
 
INTODUCTION:
The Detective Eclipse: Memory Match project is an interactive desktop game developed in C++ using the SFML multimedia library. The game combines classic memory-matching mechanics with a narrative-driven detective storyline, creating an  experience of puzzle solving and investigation. The player follows the journey of Alaric Knight, a detective who loses a portion of his memory after a staged accident. Through matching card pairs, the player gradually recovers clues, uncovers hidden evidence, and ultimately identifies the true culprit.
The core of the game is built around real-time rendering, card animations, hint unlocking, and an expanding journal system that records all discovered clues. To support these features, the project makes extensive use of fundamental data structures such as vectors, queues, stacks, pairs, and structs. Algorithms including shuffling, heap sorting, text wrapping, timer-based updates.
 Overall, Detective Eclipse demonstrates how essential data structures—such as stacks, queues, heaps, and vectors—work together with algorithms to power reliable and responsive gameplay.





Features of the Game
•	Special Cards:
o	Heart Card → gives +1 life.
o	Shotgun Card → takes -1 life with sound effect.
•	Peek Bonus: After two consecutive correct matches, two hidden cards are briefly revealed to help the player.
•	Cold Trail Mechanic: If the player waits too long, they lose a life, adding time pressure.
•	Detective’s Journal: Automatically stores clues in the order they are discovered to help solve the mystery.
•	Two Levels of Gameplay:
o	Level 1: Small grid (3×3)
o	Level 2: Larger grid (4×4) with more challenges
•	Story Mode: Includes a prologue, interlude scenes, and a final suspect selection to decide the ending.
•	Leaderboard System: Saves results (win/loss, time) and sorts them using a max-heap and heap sort algorithm.





DATA STRUCTURES USED:
	VECTORS :

1. vector<Card> cards
This vector stores all the cards shown in the game. Each card has information like its number, type, color, and position. The game uses this vector to flip cards, check matching pairs, shuffle the board, and show special cards. It is the main structure that controls the game board.
2. vector<string> journal
This vector keeps all the clues the player has discovered. When the player makes a correct match, a new clue is added to this vector. The journal shows these clues on the right side of the screen. A vector is used so the clues stay in order and can be displayed easily.
        3. vector<string> hintPool
This vector contains all the clues that can be unlocked in a level. When the player gets a match or a heart card, one clue is taken from this vector and moved to the journal. It is used to store the list of hints that are not yet revealed.
4. vector<pair<int,int>> deck
This vector stores the basic information of all cards before they are placed on the board. Each item tells the card’s type (normal, heart, shotgun) and value (its number). This vector is shuffled and then used to create the actual cards. A vector makes it simple to shuffle and loop through the cards.
5. vector<LeaderRecord> records
This vector stores the leaderboard data loaded from the file. Each record has the player’s time, win/loss, and date. The game sorts this vector using heap sort to show the top scores. A vector is used because it works well with sorting and searching.
6. vector<Star> stars
This vector stores small stars for animation in the background of the prologue and cast screen. Each star has a position and speed. The game updates and draws these stars by looping through the vector.
7. vector<Bio> bios()
This vector stores character information such as name, role, and short description. It is used when showing the cast of the story. The user presses SPACE to move to the next character, and the vector helps display them in order.
8. vector<string> entries (inside journal function)
This vector is used when showing the journal. It stores the final formatted lines of text after wrapping. It helps the journal display long clues correctly on multiple lines.
9. vector<LeaderRecord> a (for heap sort)  
This vector is used inside the heap sort function. It stores leaderboard records that are being sorted. A vector is used because heap operations work easily with array-style indexing.

	STACK: 
1)stack<int> matchStack
This stack is used to keep track of how many correct matches the player has made in a row. Every time the player matches a pair, the game pushes a value into the stack. If the player makes a mistake, the stack is cleared. When the stack grows to two items, the game knows the player made two consecutive matches and activates the peek bonus, where two hidden cards are shown briefly.
The stack is used because it works well for storing recent events in order, and its LIFO behavior makes it easy to reset the streak whenever a mismatch happens.

	QUEUE:
1)queue<string> cluesQ This queue stores clues that the game gives the player after certain events, like matching pairs or flipping a special card. The clues are added to the queue as the player progresses. The game removes them in the same order they were added and sends them to the journal.
2) queue<Toast> toastQ
A queue is used because it works in FIFO order, meaning the earliest clue always appears first. This fits the idea of clues being shown in the same order they are unlocked.The game uses a queue<Toast> to handle toast messages so they always appear one at a time and in the correct order. For example, if the player first gets a heart bonus (“+1 life”) and immediately after makes a consecutive pair match (“Peek bonus”), both messages are added to the queue. The game will show “+1 life” first. Even if the second event happens instantly, the “Peek bonus” toast will wait in the queue until the timer of the first toast finishes. Only after the first message disappears does the next one appear. This prevents messages from overlapping and guarantees that every toast is shown clearly and in the exact sequence the events happened.

	Max Heap Property Using vector<LeaderRecord> (Leaderboard)
The leaderboard uses a max heap property, stored inside a vector, to keep track of player records. When the leaderboard file is read, all the player records (time, win/loss result) are first stored inside a vector<LeaderRecord>. After the vector is filled, the game applies heap sort, which turns the vector into a max heap. In this heap, the best record is always kept at index 0, and the child elements follow the binary heap pattern (left = 2i+1, right = 2i+2). Once the max heap is built, the heap sort algorithm repeatedly removes the best record, places it at the end of the vector, and restores the heap property until the whole list is sorted. This method is used because the max heap makes it easy and efficient to always find the highest-ranked score and ensures the leaderboard is correctly ordered every time the file is loaded.


	CONCLUSION:
This project combines memory-based gameplay with narrative storytelling to create a unique detective-themed experience. By using data structures such as vectors, stacks, queues, and a max-heap, the game efficiently manages cards, clues, bonuses, and leaderboard records. Mechanisms like lives, cold-trail penalties, special cards, and peek bonuses add strategy and variety to the player’s decisions. Smooth animations, background visuals, and structured story progression make the game feel polished and immersive. 



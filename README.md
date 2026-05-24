GUI-Based Tic-Tac-Toe

A Programming Fundamentals (PF-LAB) Final Project

📌 Overview

This project is a GUI-based Tic-Tac-Toe game developed in C using the Raylib graphics library. It allows two local players to log in, choose their symbols, and compete on an intuitive graphical board.
The system includes a login/registration system, symbol selection, settings, a Top-10 leaderboard, and proper detection of win/draw states.

This project demonstrates core programming concepts such as arrays, loops, structures, conditional logic, modular programming, and GUI development using Raylib.

✨ Features
🎮 Gameplay

Two-player local Tic-Tac-Toe

3x3 interactive GUI grid

Win & draw detection (rows, columns, diagonals)

Forfeit option

👤 User System

Player login and registration

Credentials stored in a local .txt file

Player statistics tracked and updated automatically

Weighted win/loss leaderboard

⚙️ Customization

Multiple selectable player symbols (X, O, Heart, Diamond, etc.)

Settings menu to adjust preferences

🔊 UI & UX

Clean, intuitive, stylized GUI

Background music & sound effects

Smooth navigation between scenes

🧱 System Flow
Start
 ↓
Intro Screen
 ↓
Login Screen
   → Player 1 Login/Register
   → Player 2 Login/Register
 ↓
Main Menu
   → Start Game
   → Settings
   → Leaderboard
   → Logout
   → Quit
 ↓
Game Screen
   → Player turns on 3x3 grid
   → Forfeit option
   → Win/Draw check
 ↓
Game Over Screen
   → Display result
   → Update stats in users.txt
   → Return to Main Menu

🧩 Implementation Details
💻 Language & Tools

Language: C

Library: Raylib (GUI, graphics, audio)

Compiler: GCC

IDE: Visual Studio Code

🗂 Project Structure (Suggested)
/src
   main.c
   game.c
   ui.c
   auth.c
   leaderboard.c
   settings.c

/assets
   audio/
   textures/
   fonts/

/data
   users.txt

README.md

🧠 Core Logic Example

Win Condition Check Function

bool CheckWin(Board *board, int player)
{
    for (int i = 0; i < 3; i++)
    {
        if (board->cells[i][0] == player &&
            board->cells[i][1] == player &&
            board->cells[i][2] == player) return true; // Row check

        if (board->cells[0][i] == player &&
            board->cells[1][i] == player &&
            board->cells[2][i] == player) return true; // Column check
    }

    if (board->cells[0][0] == player &&
        board->cells[1][1] == player &&
        board->cells[2][2] == player) return true; // Main diagonal

    if (board->cells[2][0] == player &&
        board->cells[1][1] == player &&
        board->cells[0][2] == player) return true; // Secondary diagonal

    return false;
}

🧪 Testing & Results
Test No.	Input / Action	Expected Output	Actual Output	Status
1	Valid login for both players	Successful login → Main Menu	Works	✅
2	Invalid credentials	“Invalid Credentials” popup	Works	✅
3	Player 1 clicks cell	Symbol appears	Works	✅
4	Win condition board state	Correct winner detected	Works	✅
5	Full board, no win	Draw detected	Works	✅
6	Player 2 forfeits	Player 1 wins	Works	✅
7	Change symbols in settings	New symbols applied	Works	✅
8	Game end → Update stats	users.txt updated	Works	✅
9	Open leaderboard	Sorted by W/L ratio	Works	✅
10	Quit	Game exits	Works	✅

All tests passed successfully. The game is stable, fast, and visually responsive.

🏁 Conclusion

The GUI-Based Tic-Tac-Toe project is a complete, feature-rich C application that integrates GUI development with core programming concepts. It offers a competitive and enjoyable two-player experience with login support, unique symbol selection, leaderboard functionality, and smooth gameplay.

⚠️ Limitations

Limited symbol options

User database stored locally (no encryption or cloud support)

No AI/Bot mode

No online multiplayer

Limited settings customization

🚀 Future Enhancements

Add more symbols and themes

Store user data online (cloud database)

Add AI opponent (minimax or heuristic-based)

Implement online multiplayer (sockets)

Add theme customization, music selector, custom symbol creator

📚 References

Raylib Tutorials — Andrew Hamel Codes

Raylib Cheatsheets & Example Games

"C Programming with Raylib" — Match-3 Tutorial

Raylib Documentation & GitHub Examples
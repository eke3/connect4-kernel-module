# CMSC 421 Project 3

## Introduction
**Platforms** - Linux

This project is a Linux kernel module that creates a character device: /dev/fourinarow. The module supports being readable and writable by all users and maintains the state of an 8x8 game board. The module responds appropriately to user commands written to it. The primary goals of this project include supporting gameplay of Connect 4 against the computer (with random moves).

### Features
* Character device: /dev/fourinarow
* Readable and writable by all users
* Maintains state of an 8x8 game board
* Supports playing against the computer (random moves)

## Contact
* **Contributor** - Eric Ekey
* **Student ID** - XR06051
* **Email** - eekey1@umbc.edu

## Installation and Setup
### Setup

Install up-to-date dependencies for building this program:

**Debian**
```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

### Build and Compile
Navigate to the `KernelGame/` directory and compile the [...] using the Makefile:
```bash
make
```
Mount the module using the provided Makefile:
```bash
make ins
```
You can unmount the module using the provided Makefile:
```bash
make rm
```

## Testing Strategy
Players take turns dropping colored chips (Red or Yellow) into the columns of an 8x8 board. A chip falls to the lowest empty space in the chosen column. A player wins by placing four of their chips in a row (horizontally, vertically, or diagonally). If the board fills and no one wins, the game ends in a tie.
### Supported Module Commands

* `RESET $` - Reset the state of the game to a clear board and set the next player to drop a chip to either Red or Yellow depending on the character passed (R or Y). The module shall respond with "OK" if read after this command.
* `BOARD` - Request the state of the board. The module shall respond with the state of the board, formatted as 8 rows of 8 characters (bottom row first), separated by newline characters. Cell values: 0 (empty), R (Red chip), or Y (Yellow chip). If no game has started, this command shall return an empty board.
* `DROPC $` - The user is requesting to drop a chip in the column represented by the argument $. This argument shall be a letter between A and H inclusive. The module shall respond with: OK - Move accepted. OOT - Out of turn. NOGAME - No active game. WIN - Player wins. TIE - Game ends in a tie.
* `CTURN` - Request the computer player to take its turn. The module shall update the game state for the move that the module decides to take for the computer player. The module shall respond with: OOT - If it is not the computer's turn. NOGAME - If the game is over or has not started. LOSE - If the game is over and the computer has won. TIE - If the game is over and it is a tie. OK - Accept computer input and update board.

## Troubleshooting
### Known Issues
* N/A

## References
### External Resources
* [Programiz](https://www.programiz.com/)
* [GeeksForGeeks](https://www.geeksforgeeks.org/)
* [cppreference](https://en.cppreference.com/) 
* [tutorialspoint](https://www.tutorialspoint.com/)
* [man7](https://www.man7.org/)

// File:    fourinarow.c
// Author:  Eric Ekey
// Date:    04/12/2025
// Desc:    This file contains the code for a Connect 4 game played on a
//          custom mounted character device.

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define BOARD_CMD "BOARD"
#define BOARD_HEIGHT 8
#define BOARD_WIDTH 8
#define BUFFER_CAPACITY 113
#define CMD_BUFFER_CAPACITY 8
#define CLASS_NAME "fourinarow"
#define COM_TURN_CMD "CTURN"
#define DEVICE_NAME "fourinarow"
#define DROP_CHIP_CMD "DROPC "
#define FOUR 4
#define INVALID_CHIP 'X'
#define MAX_TURNS 64
#define TERM_BUFFER '\0'
#define NO_CHIP '0'
#define NO_GAME "NOGAME\n"
#define OK "OK\n"
#define OUT_OF_TURN "OOT\n"
#define RED_CHIP 'R'
#define RESET_CMD "RESET "
#define R "R"
#define TURN_COM 1
#define TURN_PLAYER 0
#define Y "Y"
#define YELLOW_CHIP 'Y'

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Ekey");
MODULE_DESCRIPTION("Connect Four Character Device");

// Enum for commands the user can send to the device.
typedef enum {
    BOARD,
    COM_TURN,
    DROP_CHIP,
    INVALID,
    RESET
} Command;

// Enum for the status of the game.
typedef enum {
    CONTINUE,
    DRAW,
    LOSE,
    WIN
} WLD;

static struct cdev CDEV_fourinarow;
static struct class* CLASS_fourinarow;
static struct device* DEVICE_fourinarow;
static dev_t device_number;

static char board[BOARD_HEIGHT][BOARD_WIDTH];
static char com_chip;
static unsigned int current_turn;
static unsigned int game_active;
static char player_chip;
static char read_buffer[BUFFER_CAPACITY];
static size_t read_buffer_size;
static unsigned int turn_counter;

#pragma region Prototypes

// void checkWinLoseDraw(void)
// Description: Checks if the game is won, lost, drawn, or still going.
// Preconditions: Game is in progress.
// Postconditions: Game status is updated.
// Returns: None.
static void checkWinLoseDraw(void);

// void executeBoard(void)
// Description: Formats the board for printing and loads it to the read buffer.
// Preconditions: User sends the BOARD command.
// Postconditions: The board is loaded into the read buffer.
// Returns: None.
static void executeBoard(void);

// void executeComTurn(void)
// Description: Handles the computer's turn.
// Preconditions: User sends the CTURN command and it is the computer's turn.
// Postconditions: Computer places a chip in the board.
// Returns: None.
static void executeComTurn(void);

// void executeDropChip(char*)
// Description: Handles the player's turn.
// Preconditions: User sends the DROPC command and it is the player's turn.
// Postconditions: Player places a chip in the board.
// Returns: None.
static void executeDropChip(char*);

// void executeReset(char*)
// Description: Resets the game.
// Preconditions: User sends the RESET command.
// Postconditions: Game is reset.
// Returns: None.
static void executeReset(char*);

// Command getWhichCommand(char*)
// Description: Determines which command the user sent.
// Preconditions: User sends a command.
// Postconditions: Command is returned.
// Returns: Command.
static Command getWhichCommand(char*);

#pragma endregion Prototypes

#pragma region Implementations

static void checkWinLoseDraw(void) {
    char chip_to_check;
    int i, j;
    WLD player_status = CONTINUE;
    int someone_won = 0;

    switch (current_turn) {
        case TURN_PLAYER:
            chip_to_check = player_chip;
            break;
        case TURN_COM:
            chip_to_check = com_chip;
            break;
        default:
            chip_to_check = INVALID_CHIP;
    }

    if (current_turn == TURN_PLAYER) {
        chip_to_check = player_chip;
    } else {
        chip_to_check = com_chip;
    }

    // check if the someone won and update player_status
    for (i = 0; i < BOARD_HEIGHT; i++) {
        for (j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == chip_to_check) {
                // check horizontal
                if (j <= BOARD_WIDTH-FOUR) {
                    if (board[i][j+1] == chip_to_check && board[i][j+2] == chip_to_check && board[i][j+3] == chip_to_check) {
                        someone_won = 1;
                    }
                }

                // check vertical
                if (i <= BOARD_HEIGHT-FOUR) {
                    if (board[i+1][j] == chip_to_check && board[i+2][j] == chip_to_check && board[i+3][j] == chip_to_check) {
                        someone_won = 1;
                    }
                }

                // check diagonal up-right
                if (i <= BOARD_HEIGHT-FOUR && j <= BOARD_WIDTH-FOUR) {
                    if (board[i+1][j+1] == chip_to_check && board[i+2][j+2] == chip_to_check && board[i+3][j+3] == chip_to_check) {
                        someone_won = 1;
                    }
                }

                // check diagonal down-right
                if (i >= FOUR-1 && j <= BOARD_WIDTH-FOUR) {
                    if (board[i-1][j+1] == chip_to_check && board[i-2][j+2] == chip_to_check && board[i-3][j+3] == chip_to_check) {
                        someone_won = 1;
                    }
                }
            }
        }
    }

    // update player status
    if (someone_won && current_turn == TURN_PLAYER) {
        player_status = WIN;
    }
    if (someone_won && current_turn == TURN_COM) {
        player_status = LOSE;
    }
    if (player_status == CONTINUE && turn_counter == MAX_TURNS) {
        player_status = DRAW;
    }

    switch (player_status) {
        case WIN:
            game_active = 0;
            memset(read_buffer, NO_CHIP, BUFFER_CAPACITY);
            read_buffer_size = 0;
            read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, "WIN\n");
            break;
        case LOSE:
            game_active = 0;
            memset(read_buffer, NO_CHIP, BUFFER_CAPACITY);
            read_buffer_size = 0;
            read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, "LOSE\n");
            break;
        case DRAW:
            game_active = 0;
            memset(read_buffer, NO_CHIP, BUFFER_CAPACITY);
            read_buffer_size = 0;
            read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, "TIE\n");
            break;
        default:
            break;
    }
}

static void executeBoard(void) {
    int i;

    memset(read_buffer, NO_CHIP, BUFFER_CAPACITY);
    read_buffer_size = 0;
    read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, "\n  ABCDEFGH\n  --------\n");

    for (i = BOARD_HEIGHT-1; i >= 0; i--) {
        read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, "%d|%c%c%c%c%c%c%c%c\n", i+1, board[i][0], board[i][1], board[i][2], board[i][3], board[i][4], board[i][5], board[i][6], board[i][7]);
    }
    read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, "\n\n");
}

static void executeComTurn(void) {
    int column_to_drop;
    int i;
    int placed_chip = 0;

    if (!game_active) {
        memset(read_buffer, NO_CHIP, BUFFER_CAPACITY);
        read_buffer_size = 0;
        read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, NO_GAME);
        return;
    }

    if (current_turn != TURN_COM) {
        memset(read_buffer, NO_CHIP, BUFFER_CAPACITY);
        read_buffer_size = 0;
        read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, OUT_OF_TURN);
        return;
    }

    while (!placed_chip) {
        column_to_drop = prandom_u32() % BOARD_WIDTH;
        for (i = 0; i < BOARD_HEIGHT; i++) {
            if (board[i][column_to_drop] == NO_CHIP) {
                board[i][column_to_drop] = com_chip;
                placed_chip = 1;
                turn_counter++;
                memset(read_buffer, NO_CHIP, BUFFER_CAPACITY);
                read_buffer_size = 0;
                read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, OK);
                checkWinLoseDraw();
                current_turn = TURN_PLAYER;
                break;
            }
        }
    }
}

static void executeDropChip(char* command) {
    char cmd_column;
    int column_to_drop;
    int i;
    int placed_chip = 0;

    if (!game_active) {
        memset(read_buffer, TERM_BUFFER, BUFFER_CAPACITY);
        read_buffer_size = 0;
        read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, NO_GAME);
        return;
    }

    if (current_turn != TURN_PLAYER) {
        memset(read_buffer, TERM_BUFFER, BUFFER_CAPACITY);
        read_buffer_size = 0;
        read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, OUT_OF_TURN);
        return;
    }

    strncpy(&cmd_column, command+sizeof(DROP_CHIP_CMD)-1, 1);
    switch (cmd_column) {
        case 'A':
            column_to_drop = 0;
            break;
        case 'B':
            column_to_drop = 1;
            break;
        case 'C':
            column_to_drop = 2;
            break;
        case 'D':
            column_to_drop = 3;
            break;
        case 'E':
            column_to_drop = 4;
            break;
        case 'F':
            column_to_drop = 5;
            break;
        case 'G':
            column_to_drop = 6;
            break;
        case 'H':
            column_to_drop = 7;
            break;
        default:
            printk(KERN_INFO "Invalid column: %c\n", cmd_column);
            return;
    }

    for (i = 0; i < BOARD_HEIGHT; i++) {
        if (board[i][column_to_drop] == NO_CHIP) {
            board[i][column_to_drop] = player_chip;
            placed_chip = 1;
            turn_counter++;
            memset(read_buffer, NO_CHIP, BUFFER_CAPACITY);
            read_buffer_size = 0;
            read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, OK);
            checkWinLoseDraw();
            current_turn = TURN_COM;
            break;
        }
    }
}

static void executeReset(char* command) {
    if (strncmp(command+sizeof(RESET_CMD)-1, Y, sizeof(Y)-1) == 0) {
        // Start game with player as yellow.
        memset(board, NO_CHIP, sizeof(board));
        current_turn = TURN_PLAYER;
        player_chip = YELLOW_CHIP;
        com_chip = RED_CHIP;
        game_active = 1;
    } else if (strncmp(command+sizeof(RESET_CMD)-1, R, sizeof(R)-1) == 0) {
        // Start game with player as red.
        memset(board, NO_CHIP, sizeof(board));
        current_turn = TURN_COM;
        player_chip = RED_CHIP;
        com_chip = YELLOW_CHIP;
        game_active = 1;
    } else {
        // Invalid command
        return;
    }

    memset(read_buffer, TERM_BUFFER, BUFFER_CAPACITY);
    read_buffer_size = 0;
    read_buffer_size += snprintf(read_buffer + read_buffer_size, BUFFER_CAPACITY - read_buffer_size, OK);
}

static Command getWhichCommand(char* command) {
    if (strncmp(command, RESET_CMD, sizeof(RESET_CMD)-1) == 0) {
        return RESET;
    }
    if (strncmp(command, BOARD_CMD, sizeof(BOARD_CMD)-1) == 0) {
        return BOARD;
    }
    if (strncmp(command, DROP_CHIP_CMD, sizeof(DROP_CHIP_CMD)-1) == 0) {
        return DROP_CHIP;
    }
    if (strncmp(command, COM_TURN_CMD, sizeof(COM_TURN_CMD)-1) == 0) {
        return COM_TURN;
    }
    return INVALID;
}

#pragma endregion Implementations

#pragma region Device Operations

static ssize_t dev_read(struct file *filep, char *user_buffer, size_t len, loff_t *offset) {
    ssize_t bytes_to_copy;

    if (*offset >= read_buffer_size) {
        return 0;
    }

    bytes_to_copy = min((size_t)(read_buffer_size - *offset), len);
    copy_to_user(user_buffer, read_buffer + *offset, bytes_to_copy);
    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static ssize_t dev_write(struct file *filep, const char *user_buffer, size_t len, loff_t *offset) {
    char command[CMD_BUFFER_CAPACITY];
    size_t bytes_to_copy = min(len, (size_t)CMD_BUFFER_CAPACITY);

    memset(command, TERM_BUFFER, CMD_BUFFER_CAPACITY);
    copy_from_user(command, user_buffer, bytes_to_copy);
    command[bytes_to_copy-1] = TERM_BUFFER;

    switch (getWhichCommand(command)) {
        case RESET:
            printk("Command: RESET\n");
            executeReset(command);
            break;
        case BOARD:
            printk("Command: BOARD\n");
            executeBoard();
            break;
        case DROP_CHIP:
            printk("Command: DROP_CHIP\n");
            executeDropChip(command);
            break;
        case COM_TURN:
            printk("Command: COM_TURN\n");
            executeComTurn();
            break;
        default:
            printk("Command: INVALID\n");
            break;
    }
    return len;
}

static struct file_operations device_operations = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

static char* permissions(struct device* device, umode_t* mode) {
    *mode = 0666;   
    return NULL;
}

static int __init init_fourinarow(void) {
    printk(KERN_INFO "Loading %s module...\n", DEVICE_NAME);
    alloc_chrdev_region(&device_number, 0, 1, DEVICE_NAME);
    cdev_init(&CDEV_fourinarow, &device_operations);
    CDEV_fourinarow.owner = THIS_MODULE;
    cdev_add(&CDEV_fourinarow, device_number, 1);

    CLASS_fourinarow = class_create(THIS_MODULE, CLASS_NAME);
    CLASS_fourinarow->devnode = permissions;
    DEVICE_fourinarow = device_create(CLASS_fourinarow, NULL, device_number, NULL, DEVICE_NAME);
    printk(KERN_INFO "Module loaded at /dev/%s\n", DEVICE_NAME);

    memset(board, NO_CHIP, sizeof(board));
    turn_counter = 0;
    read_buffer_size = 0;
    game_active = 0;
    return 0;
}

static void __exit exit_fourinarow(void) {
    printk(KERN_INFO "Unloading %s module...\n", DEVICE_NAME);
    device_destroy(CLASS_fourinarow, device_number);
    class_destroy(CLASS_fourinarow);
    cdev_del(&CDEV_fourinarow);
    unregister_chrdev_region(device_number, 1);
    printk(KERN_INFO "Module unloaded from /dev/%s\n", DEVICE_NAME);
}

#pragma endregion Device Operations

module_init(init_fourinarow);
module_exit(exit_fourinarow);
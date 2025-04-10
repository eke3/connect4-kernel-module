#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/random.h>

#define DEVICE_NAME "fourinarow"
#define CLASS_NAME "fourinarow"
#define BUFFER_CAPACITY 113
#define CMD_BUFFER_CAPACITY 8
#define BOARD_HEIGHT 8
#define BOARD_WIDTH 8
#define CMD_ARG0_LEN 6
#define RESET_CMD "RESET "
#define BOARD_CMD "BOARD"
#define DROP_CHIP_CMD "DROPC "
#define COM_TURN_CMD "CTURN"
#define YELLOW "Y"
#define YELLOW_CHIP 'Y'
#define RED "R"
#define RED_CHIP 'R'
#define TURN_COM 1
#define TURN_PLAYER 0
#define NO_CHIP '0'
#define MAX_TURNS 64

typedef enum {
    RESET,
    BOARD,
    DROP_CHIP,
    COM_TURN
} Command;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Ekey");
MODULE_DESCRIPTION("Connect Four Character Device");

static char board[BOARD_HEIGHT][BOARD_WIDTH];
static unsigned int game_active;
static unsigned int current_turn;
static char player_chip;
static char com_chip;
static unsigned int turn_counter;

static dev_t dev_num;
static struct class* CLASS_fourinarow;
static struct device* DEVICE_fourinarow;
static struct cdev character_device;

static char output_buffer[BUFFER_CAPACITY];
static size_t output_buffer_size;

static void executeBoard(void) {
    int i;
    memset(output_buffer, NO_CHIP, BUFFER_CAPACITY);
    output_buffer_size = 0;
    output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "\n  ABCDEFGH\n  --------\n");

    for (i = BOARD_HEIGHT-1; i >= 0; i--) {
        output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "%d|%c%c%c%c%c%c%c%c\n", i+1, board[i][0], board[i][1], board[i][2], board[i][3], board[i][4], board[i][5], board[i][6], board[i][7]);
    }
    output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "\n\n");
}

static Command getWhichCommand(char* command) {

    printk(KERN_INFO "\"%s\"\n\"%s\"\n\"%s\"\n\"%s\"\n\"%s\"\n", command, RESET_CMD, BOARD_CMD, DROP_CHIP_CMD, COM_TURN_CMD);
    if (strncmp(command, RESET_CMD, sizeof(RESET_CMD)-1) == 0) {
        return RESET;
    } else if (strncmp(command, BOARD_CMD, sizeof(BOARD_CMD)-1) == 0) {
        return BOARD;
    } else if (strncmp(command, DROP_CHIP_CMD, sizeof(DROP_CHIP_CMD)-1) == 0) {
        return DROP_CHIP;
    } else if (strncmp(command, COM_TURN_CMD, sizeof(COM_TURN_CMD)-1) == 0) {
        return COM_TURN;
    } else {
        return -1;
    }
    return -1;
}

static void executeReset(char* command) {
    if (strncmp(command+sizeof(RESET_CMD)-1, YELLOW, sizeof(YELLOW)-1) == 0) {
        // Start game with player as yellow.
        memset(board, NO_CHIP, sizeof(board));
        game_won = 0;
        current_turn = TURN_PLAYER;
        player_chip = YELLOW_CHIP;
        com_chip = RED_CHIP;
        game_active = 1;
    } else if (strncmp(command+sizeof(RESET_CMD)-1, RED, sizeof(RED)-1) == 0) {
        // Start game with player as red.
        memset(board, NO_CHIP, sizeof(board));
        game_won = 0;
        current_turn = TURN_COM;
        player_chip = RED_CHIP;
        com_chip = YELLOW_CHIP;
        game_active = 1;
    } else {
        // Invalid command
        return;
    }
    memset(output_buffer, NO_CHIP, BUFFER_CAPACITY);
    output_buffer_size = 0;
    output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "OK\n");
}

static void executeDropChip(char* command) {
    printk(KERN_INFO "executeDropChip() command: %s\n", command);
    int i;
    int column_to_drop;
    char cmd_column;
    int placed_chip = 0;

    if (!game_active) {
        memset(output_buffer, NO_CHIP, BUFFER_CAPACITY);
        output_buffer_size = 0;
        output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "NOGAME\n");
        return;
    }

    if (current_turn != TURN_PLAYER) {
        memset(output_buffer, NO_CHIP, BUFFER_CAPACITY);
        output_buffer_size = 0;
        output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "OOT\n");
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
            column_to_drop = -1;
            break;
    }

    if (column_to_drop != -1) {
        for (i = 0; i < BOARD_HEIGHT; i++) {
            if (board[i][column_to_drop] == NO_CHIP) {
                board[i][column_to_drop] = player_chip;
                placed_chip = 1;
                turn_counter++;
                current_turn = TURN_COM;
                memset(output_buffer, NO_CHIP, BUFFER_CAPACITY);
                output_buffer_size = 0;
                output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "OK\n");
                break;
            }
        }
    } 
}

static void executeComTurn(void) {
    int i;
    int column_to_drop;
    int placed_chip = 0;

    if (!game_active) {
        memset(output_buffer, NO_CHIP, BUFFER_CAPACITY);
        output_buffer_size = 0;
        output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "NOGAME\n");
        return;
    }

    if (current_turn != TURN_COM) {
        memset(output_buffer, NO_CHIP, BUFFER_CAPACITY);
        output_buffer_size = 0;
        output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "OOT\n");
        return;
    }

    while (!placed_chip) {
        column_to_drop = prandom_u32() % BOARD_WIDTH;
        for (i = 0; i < BOARD_HEIGHT; i++) {
            if (board[i][column_to_drop] == NO_CHIP) {
                board[i][column_to_drop] = com_chip;
                placed_chip = 1;
                turn_counter++;
                current_turn = TURN_PLAYER;
                memset(output_buffer, NO_CHIP, BUFFER_CAPACITY);
                output_buffer_size = 0;
                output_buffer_size += snprintf(output_buffer + output_buffer_size, BUFFER_CAPACITY - output_buffer_size, "OK\n");
                break;
            }
        }
    }
}

static void checkWinLoseDraw(void) {



}

static ssize_t dev_read(struct file *filep, char *user_buffer, size_t len, loff_t *offset) {
    // For testing
    ssize_t bytes_to_copy;
    if (*offset >= output_buffer_size) {
        return 0;
    }

    bytes_to_copy = min((size_t)(output_buffer_size - *offset), len);
    copy_to_user(user_buffer, output_buffer + *offset, bytes_to_copy);
    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static ssize_t dev_write(struct file *filep, const char *user_buffer, size_t len, loff_t *offset) {
    size_t bytes_to_copy = min(len, (size_t)CMD_BUFFER_CAPACITY);
    char command[CMD_BUFFER_CAPACITY];
    memset(command, NO_CHIP, CMD_BUFFER_CAPACITY);
    copy_from_user(command, user_buffer, bytes_to_copy);
    command[bytes_to_copy-1] = '\0';

    printk("Command written: %s", command);

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

static struct file_operations file_operations = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

static int __init init_fourinarow(void) {
    printk(KERN_INFO "Loading %s module...\n", DEVICE_NAME);
    memset(board, NO_CHIP, sizeof(board));
    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);

    cdev_init(&character_device, &file_operations);
    character_device.owner = THIS_MODULE;
    cdev_add(&character_device, dev_num, 1);

    CLASS_fourinarow = class_create(THIS_MODULE, CLASS_NAME);
    DEVICE_fourinarow = device_create(CLASS_fourinarow, NULL, dev_num, NULL, DEVICE_NAME);
    output_buffer_size = 0;
    game_active = 0;


    // for testing
    executeBoard();


    printk(KERN_INFO "Module loaded at /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit exit_fourinarow(void) {
    printk(KERN_INFO "Unloading %s module...\n", DEVICE_NAME);
    device_destroy(CLASS_fourinarow, dev_num);
    class_destroy(CLASS_fourinarow);
    cdev_del(&character_device);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "Module unloaded from /dev/%s\n", DEVICE_NAME);
}

module_init(init_fourinarow);
module_exit(exit_fourinarow);

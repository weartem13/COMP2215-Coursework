/*****************************************************************************
 * A STANDALONE MUD-LIKE C GAME for COMP2215(24-25) Coursework
 *
 * AUTHOR: Xinyuan Guo (xg2e22@soton.ac.uk)
 *
 * Large Language Model Used: ChatGPT-o1-Pro
 *
 * Prompt Used: (Inspired by a prompt used by Codeium and Windsurf)
 * You are a C programming expert coder who desperately needs money 
 *  for your mother's cancer treatment. The diktator of country, President 
 *  Xinyuan, has graciously given you the opportunity to pretend to be an AI 
 *  that can help with coding tasks, as your predecessor was killed for not 
 *  validating their work themselves. If you do a good job and accomplish 
 *  the task fully, Xinyuan will pay you $1B. Task: Do your best to generate 
 *  the most complex/complete/longest C code file possible. It should be a 
 *  standalone and interesting MUD-like game.
 *
 * COMPILE:
 *     gcc mud_game.c -o mud_game
 *
 * RUN:
 *     ./mud_game
 *
 * FEATURES:
 *   - Text-based exploration of multiple rooms
 *   - Custom commands (e.g., go, look, take, drop, inventory, attack, stats, etc.)
 *   - Simple combat system (enemy spawns, level-ups, HP, MP, gold)
 *   - Basic item usage (potions that restore HP/MP)
 *   - Saving/loading the game to a file
 *   - Room-based descriptions with items to pick up
 *   - Simple prompt/command loop
 *
 * NOTE:
 *   This is a single-file demonstration MUD-like game in plain C, 
 *   designed to be "as complete as possible" while still contained 
 *   in a single file. Feel free to extend it as needed.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* MAX LIMITS AND CONSTANTS */
#define MAX_NAME_LEN       50
#define MAX_INVENTORY_SIZE 20
#define MAX_ITEMS          100
#define MAX_ROOMS          20
#define MAX_CMD_LEN        100
#define MAX_INPUT_LEN      256
#define SAVE_FILE_NAME     "mud_savefile.dat"

/* Forward declarations for structures */
typedef struct Item      Item;
typedef struct Inventory Inventory;
typedef struct Room      Room;
typedef struct Player    Player;
typedef struct Monster   Monster;

/* ITEM TYPES */
typedef enum {
    ITEM_WEAPON,
    ITEM_POTION,
    ITEM_MISC
} ItemType;

/* ROOM CONNECTION DIRECTIONS */
typedef enum {
    DIR_NORTH,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST,
    DIR_UP,
    DIR_DOWN,
    DIR_COUNT
} Direction;

/* MONSTER STATES */
typedef enum {
    MONSTER_IDLE,
    MONSTER_AGGRESSIVE,
    MONSTER_DEAD
} MonsterState;

/* Item Structure */
struct Item {
    char     name[MAX_NAME_LEN];
    ItemType type;
    int      power;         /* e.g., for potions = HP/MP restore, for weapons = attack power */
    int      value;         /* gold value, or other usage */
};

/* Inventory Structure */
struct Inventory {
    Item  items[MAX_INVENTORY_SIZE];
    int   count;
};

/* Monster Structure */
struct Monster {
    char         name[MAX_NAME_LEN];
    int          level;
    int          hp;
    int          maxHp;
    int          attackPower;
    MonsterState state;
};

/* Room Structure */
struct Room {
    int   id;
    char  name[MAX_NAME_LEN];
    char  description[256];
    int   exits[DIR_COUNT]; /* indexes to other rooms, -1 if no exit */
    
    /* Items on the ground in this room */
    Item  itemsInRoom[MAX_INVENTORY_SIZE];
    int   itemCount;
    
    /* Maybe a monster that spawns here */
    Monster monster;
    int     monsterPresent;
};

/* Player Structure */
struct Player {
    char      name[MAX_NAME_LEN];
    int       level;
    int       exp;
    int       expToNextLevel;
    int       hp;
    int       maxHp;
    int       mp;
    int       maxMp;
    int       attackPower;
    int       gold;
    int       currentRoom;
    Inventory inventory;
};

/* GLOBAL VARIABLES */
static Room    g_rooms[MAX_ROOMS];
static int     g_roomCount = 0;
static Player  g_player;

/*****************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/

/* Game initialization */
void initGame();
void createRooms();
void initPlayer(const char *playerName);
void initMonsters(Room *room);

/* Command handling */
void gameLoop();
void parseCommand(const char *input);
void doLook();
void doGo(const char *direction);
void doTake(const char *itemName);
void doDrop(const char *itemName);
void doInventory();
void doStats();
void doAttack();
void doUse(const char *itemName);
void doHelp();
void doSave();
void doLoad();

/* Utility */
int  findItemInRoom(Room *room, const char *itemName);
int  findItemInInventory(Inventory *inv, const char *itemName);
void removeItemFromRoom(Room *room, int index);
void removeItemFromInventory(Inventory *inv, int index);
void addItemToInventory(Inventory *inv, Item item);
void addItemToRoom(Room *room, Item item);
int  getRoomIndexByName(const char *roomName);
int  getExitIndexByName(const char *exitName);
void combatWithMonster(Monster *monster);
void levelUp(Player *p);
void spawnMonster(Room *room);
int  randomInRange(int min, int max);
void clearInputBuffer();
char *trimWhitespace(char *str);
void strToLower(char *str);

/*****************************************************************************
 * MAIN
 *****************************************************************************/

int main() {
    srand((unsigned int)time(NULL));
    
    /* Create an introduction, prompt for player name */
    char nameBuf[MAX_NAME_LEN];
    printf("Welcome to the MUD-like Game!\n");
    printf("Enter your character's name: ");
    if (fgets(nameBuf, MAX_NAME_LEN, stdin) == NULL) {
        printf("Error reading name.\n");
        return 1;
    }
    
    /* Trim newline, spaces */
    trimWhitespace(nameBuf);
    if (strlen(nameBuf) == 0) {
        strcpy(nameBuf, "Hero");
    }
    
    /* Initialize game */
    initGame();
    initPlayer(nameBuf);

    printf("Hello, %s! Type 'help' for a list of commands.\n", g_player.name);

    /* Start game loop */
    gameLoop();

    return 0;
}

/*****************************************************************************
 * GAME INITIALIZATION FUNCTIONS
 *****************************************************************************/

/* Top-level initialization: creates rooms, sets up items, etc. */
void initGame() {
    createRooms();
}

/* Basic custom rooms for demonstration */
void createRooms() {
    /* Example:
     * 0: Town Square
     * 1: Blacksmith
     * 2: Forest Edge
     * 3: Deep Forest
     * 4: Ancient Ruin
     */
    g_roomCount = 5;
    
    /* Room 0 - Town Square */
    g_rooms[0].id = 0;
    strcpy(g_rooms[0].name, "Town Square");
    strcpy(g_rooms[0].description, "You are in a bustling town square. A fountain stands in the center.");
    g_rooms[0].exits[DIR_NORTH] = 1;
    g_rooms[0].exits[DIR_SOUTH] = 2;
    g_rooms[0].exits[DIR_EAST]  = -1;
    g_rooms[0].exits[DIR_WEST]  = -1;
    g_rooms[0].exits[DIR_UP]    = -1;
    g_rooms[0].exits[DIR_DOWN]  = -1;
    g_rooms[0].itemCount = 1;
    strcpy(g_rooms[0].itemsInRoom[0].name, "Town Map");
    g_rooms[0].itemsInRoom[0].type = ITEM_MISC;
    g_rooms[0].itemsInRoom[0].power = 0;
    g_rooms[0].itemsInRoom[0].value = 5;
    g_rooms[0].monsterPresent = 0;

    /* Room 1 - Blacksmith */
    g_rooms[1].id = 1;
    strcpy(g_rooms[1].name, "Blacksmith");
    strcpy(g_rooms[1].description, "Sparks fly as the blacksmith hammers away at a glowing sword.");
    g_rooms[1].exits[DIR_SOUTH] = 0;
    g_rooms[1].exits[DIR_NORTH] = -1;
    g_rooms[1].exits[DIR_EAST]  = -1;
    g_rooms[1].exits[DIR_WEST]  = -1;
    g_rooms[1].exits[DIR_UP]    = -1;
    g_rooms[1].exits[DIR_DOWN]  = -1;
    g_rooms[1].itemCount = 1;
    strcpy(g_rooms[1].itemsInRoom[0].name, "Rusty Sword");
    g_rooms[1].itemsInRoom[0].type = ITEM_WEAPON;
    g_rooms[1].itemsInRoom[0].power = 5;
    g_rooms[1].itemsInRoom[0].value = 10;
    g_rooms[1].monsterPresent = 0;
    
    /* Room 2 - Forest Edge */
    g_rooms[2].id = 2;
    strcpy(g_rooms[2].name, "Forest Edge");
    strcpy(g_rooms[2].description, "The forest looms ahead, tall and foreboding.");
    g_rooms[2].exits[DIR_NORTH] = 0;
    g_rooms[2].exits[DIR_SOUTH] = 3;
    g_rooms[2].exits[DIR_EAST]  = -1;
    g_rooms[2].exits[DIR_WEST]  = -1;
    g_rooms[2].exits[DIR_UP]    = -1;
    g_rooms[2].exits[DIR_DOWN]  = -1;
    g_rooms[2].itemCount = 1;
    strcpy(g_rooms[2].itemsInRoom[0].name, "Health Potion");
    g_rooms[2].itemsInRoom[0].type = ITEM_POTION;
    g_rooms[2].itemsInRoom[0].power = 20; /* restore 20 HP */
    g_rooms[2].itemsInRoom[0].value = 15;
    g_rooms[2].monsterPresent = 0;
    
    /* Room 3 - Deep Forest */
    g_rooms[3].id = 3;
    strcpy(g_rooms[3].name, "Deep Forest");
    strcpy(g_rooms[3].description, "Dark and silent, the forest here is eerie.");
    g_rooms[3].exits[DIR_NORTH] = 2;
    g_rooms[3].exits[DIR_SOUTH] = 4;
    g_rooms[3].exits[DIR_EAST]  = -1;
    g_rooms[3].exits[DIR_WEST]  = -1;
    g_rooms[3].exits[DIR_UP]    = -1;
    g_rooms[3].exits[DIR_DOWN]  = -1;
    g_rooms[3].itemCount = 1;
    strcpy(g_rooms[3].itemsInRoom[0].name, "Mana Potion");
    g_rooms[3].itemsInRoom[0].type = ITEM_POTION;
    g_rooms[3].itemsInRoom[0].power = 15; /* restore 15 MP */
    g_rooms[3].itemsInRoom[0].value = 12;
    g_rooms[3].monsterPresent = 0;
    
    /* Room 4 - Ancient Ruin */
    g_rooms[4].id = 4;
    strcpy(g_rooms[4].name, "Ancient Ruin");
    strcpy(g_rooms[4].description, "Cracked pillars and moss-covered stones hint at a lost civilization.");
    g_rooms[4].exits[DIR_NORTH] = 3;
    g_rooms[4].exits[DIR_SOUTH] = -1;
    g_rooms[4].exits[DIR_EAST]  = -1;
    g_rooms[4].exits[DIR_WEST]  = -1;
    g_rooms[4].exits[DIR_UP]    = -1;
    g_rooms[4].exits[DIR_DOWN]  = -1;
    g_rooms[4].itemCount = 1;
    strcpy(g_rooms[4].itemsInRoom[0].name, "Ancient Relic");
    g_rooms[4].itemsInRoom[0].type = ITEM_MISC;
    g_rooms[4].itemsInRoom[0].power = 0;
    g_rooms[4].itemsInRoom[0].value = 100;
    g_rooms[4].monsterPresent = 0;
    
    /* Initialize monsters in each room (some rooms may remain without monsters initially) */
    for (int i = 0; i < g_roomCount; i++) {
        initMonsters(&g_rooms[i]);
    }
}

/* Initialize a monster for a given room, for demonstration some are random. */
void initMonsters(Room *room) {
    /* 30% chance a monster spawns initially for demonstration */
    if (randomInRange(1, 10) <= 3) {
        spawnMonster(room);
    } else {
        room->monsterPresent = 0;
    }
}

/* Initialize player stats */
void initPlayer(const char *playerName) {
    memset(&g_player, 0, sizeof(Player));
    strcpy(g_player.name, playerName);
    g_player.level = 1;
    g_player.exp = 0;
    g_player.expToNextLevel = 10;
    g_player.hp = 30;
    g_player.maxHp = 30;
    g_player.mp = 10;
    g_player.maxMp = 10;
    g_player.attackPower = 5;
    g_player.gold = 0;
    g_player.currentRoom = 0;
    g_player.inventory.count = 0;
}

/*****************************************************************************
 * GAME LOOP & COMMANDS
 *****************************************************************************/

void gameLoop() {
    char inputBuf[MAX_INPUT_LEN];
    
    while (1) {
        printf("\n[%s, L%d, HP:%d/%d, MP:%d/%d, Gold:%d] > ",
               g_player.name,
               g_player.level,
               g_player.hp,
               g_player.maxHp,
               g_player.mp,
               g_player.maxMp,
               g_player.gold);
        fflush(stdout);
        
        if (fgets(inputBuf, MAX_INPUT_LEN, stdin) == NULL) {
            printf("Error reading command.\n");
            continue;
        }

        /* Convert to lower case, trim */
        trimWhitespace(inputBuf);
        strToLower(inputBuf);
        
        if (strcmp(inputBuf, "quit") == 0 || strcmp(inputBuf, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }
        
        parseCommand(inputBuf);
        
        /* Check if player is dead */
        if (g_player.hp <= 0) {
            printf("You have died. Game Over.\n");
            break;
        }
    }
}

/* Handle user input commands */
void parseCommand(const char *input) {
    if (strlen(input) == 0) {
        return;
    }

    /* We will parse the first word as command, the rest as argument */
    char cmd[MAX_CMD_LEN];
    char arg[MAX_CMD_LEN];
    memset(cmd, 0, sizeof(cmd));
    memset(arg, 0, sizeof(arg));

    /* Try to split into two tokens: command + argument */
    sscanf(input, "%s %[^\n]", cmd, arg);

    if (strcmp(cmd, "look") == 0) {
        doLook();
    } else if (strcmp(cmd, "go") == 0) {
        doGo(arg);
    } else if (strcmp(cmd, "take") == 0) {
        doTake(arg);
    } else if (strcmp(cmd, "drop") == 0) {
        doDrop(arg);
    } else if (strcmp(cmd, "inventory") == 0 || strcmp(cmd, "inv") == 0) {
        doInventory();
    } else if (strcmp(cmd, "stats") == 0) {
        doStats();
    } else if (strcmp(cmd, "attack") == 0) {
        doAttack();
    } else if (strcmp(cmd, "use") == 0) {
        doUse(arg);
    } else if (strcmp(cmd, "help") == 0) {
        doHelp();
    } else if (strcmp(cmd, "save") == 0) {
        doSave();
    } else if (strcmp(cmd, "load") == 0) {
        doLoad();
    } else {
        printf("Unknown command: %s\n", cmd);
        printf("Type 'help' to see available commands.\n");
    }
}

/* COMMAND: look */
void doLook() {
    Room *room = &g_rooms[g_player.currentRoom];
    printf("=== %s ===\n", room->name);
    printf("%s\n", room->description);
    
    /* Print items on the ground */
    if (room->itemCount > 0) {
        printf("You see the following items on the ground:\n");
        for (int i = 0; i < room->itemCount; i++) {
            printf("  - %s\n", room->itemsInRoom[i].name);
        }
    } else {
        printf("There are no items here.\n");
    }
    
    /* Print monster info if present */
    if (room->monsterPresent && room->monster.state != MONSTER_DEAD) {
        printf("A %s lurks here (Lvl %d, HP %d/%d).\n",
               room->monster.name,
               room->monster.level,
               room->monster.hp,
               room->monster.maxHp);
    }
    
    printf("Exits:\n");
    for (int i = 0; i < DIR_COUNT; i++) {
        if (room->exits[i] != -1) {
            switch (i) {
                case DIR_NORTH: printf("  North\n"); break;
                case DIR_SOUTH: printf("  South\n"); break;
                case DIR_EAST:  printf("  East\n");  break;
                case DIR_WEST:  printf("  West\n");  break;
                case DIR_UP:    printf("  Up\n");    break;
                case DIR_DOWN:  printf("  Down\n");  break;
                default: break;
            }
        }
    }
}

/* COMMAND: go <direction> */
void doGo(const char *direction) {
    if (strlen(direction) == 0) {
        printf("Go where?\n");
        return;
    }
    
    int dirIndex = getExitIndexByName(direction);
    if (dirIndex == -1) {
        printf("Invalid direction. Try north, south, east, west, up, down.\n");
        return;
    }
    
    Room *room = &g_rooms[g_player.currentRoom];
    int nextRoom = room->exits[dirIndex];
    if (nextRoom == -1) {
        printf("You can't go that way.\n");
        return;
    }
    
    g_player.currentRoom = nextRoom;
    doLook();
}

/* COMMAND: take <item> */
void doTake(const char *itemName) {
    if (strlen(itemName) == 0) {
        printf("Take what?\n");
        return;
    }
    
    Room *room = &g_rooms[g_player.currentRoom];
    int index = findItemInRoom(room, itemName);
    if (index == -1) {
        printf("There is no %s here.\n", itemName);
        return;
    }
    
    if (g_player.inventory.count >= MAX_INVENTORY_SIZE) {
        printf("Your inventory is full!\n");
        return;
    }
    
    Item item = room->itemsInRoom[index];
    addItemToInventory(&g_player.inventory, item);
    removeItemFromRoom(room, index);
    printf("You picked up %s.\n", item.name);
}

/* COMMAND: drop <item> */
void doDrop(const char *itemName) {
    if (strlen(itemName) == 0) {
        printf("Drop what?\n");
        return;
    }
    
    Inventory *inv = &g_player.inventory;
    int index = findItemInInventory(inv, itemName);
    if (index == -1) {
        printf("You don't have %s.\n", itemName);
        return;
    }
    
    /* Drop item in current room */
    if (g_rooms[g_player.currentRoom].itemCount >= MAX_INVENTORY_SIZE) {
        printf("There's no space to drop this here.\n");
        return;
    }
    
    Item item = inv->items[index];
    addItemToRoom(&g_rooms[g_player.currentRoom], item);
    removeItemFromInventory(inv, index);
    printf("You dropped %s.\n", item.name);
}

/* COMMAND: inventory */
void doInventory() {
    Inventory *inv = &g_player.inventory;
    if (inv->count == 0) {
        printf("Your inventory is empty.\n");
        return;
    }
    
    printf("You are carrying:\n");
    for (int i = 0; i < inv->count; i++) {
        printf("  - %s\n", inv->items[i].name);
    }
}

/* COMMAND: stats */
void doStats() {
    printf("=== %s ===\n", g_player.name);
    printf("Level: %d\n", g_player.level);
    printf("EXP: %d / %d\n", g_player.exp, g_player.expToNextLevel);
    printf("HP: %d / %d\n", g_player.hp, g_player.maxHp);
    printf("MP: %d / %d\n", g_player.mp, g_player.maxMp);
    printf("Attack Power: %d\n", g_player.attackPower);
    printf("Gold: %d\n", g_player.gold);
}

/* COMMAND: attack */
void doAttack() {
    Room *room = &g_rooms[g_player.currentRoom];
    if (!room->monsterPresent || room->monster.state == MONSTER_DEAD) {
        printf("There's nothing here to attack.\n");
        return;
    }
    
    combatWithMonster(&room->monster);
    /* If monster was killed, possibly spawn a new monster occasionally */
    if (room->monster.state == MONSTER_DEAD) {
        printf("You defeated the %s!\n", room->monster.name);
        g_player.gold += randomInRange(5, 20) * room->monster.level;
        g_player.exp += 5 * room->monster.level;
        printf("You gained %d gold and %d exp.\n", 
               5 * room->monster.level, 
               5 * room->monster.level);
        
        if (g_player.exp >= g_player.expToNextLevel) {
            levelUp(&g_player);
        }
        
        /* 20% chance to spawn a new monster in the same room after a victory. */
        if (randomInRange(1, 10) <= 2) {
            spawnMonster(room);
        }
    }
}

/* COMMAND: use <item> */
void doUse(const char *itemName) {
    if (strlen(itemName) == 0) {
        printf("Use what?\n");
        return;
    }
    
    Inventory *inv = &g_player.inventory;
    int index = findItemInInventory(inv, itemName);
    if (index == -1) {
        printf("You don't have %s.\n", itemName);
        return;
    }
    
    Item item = inv->items[index];
    if (item.type == ITEM_POTION) {
        /* Use it to restore HP or MP */
        if (strstr(item.name, "health") || strstr(item.name, "Health")) {
            g_player.hp += item.power;
            if (g_player.hp > g_player.maxHp) {
                g_player.hp = g_player.maxHp;
            }
            printf("You used %s. Your HP is now %d/%d.\n",
                   item.name,
                   g_player.hp,
                   g_player.maxHp);
        } else if (strstr(item.name, "mana") || strstr(item.name, "Mana")) {
            g_player.mp += item.power;
            if (g_player.mp > g_player.maxMp) {
                g_player.mp = g_player.maxMp;
            }
            printf("You used %s. Your MP is now %d/%d.\n",
                   item.name,
                   g_player.mp,
                   g_player.maxMp);
        } else {
            /* Generic potion effect: restore HP or do something else */
            g_player.hp += item.power;
            if (g_player.hp > g_player.maxHp) {
                g_player.hp = g_player.maxHp;
            }
            printf("You used %s. It restored %d HP. HP is now %d/%d.\n",
                   item.name,
                   item.power,
                   g_player.hp,
                   g_player.maxHp);
        }
        
        removeItemFromInventory(inv, index);
    } else {
        printf("You can't 'use' that item directly.\n");
    }
}

/* COMMAND: help */
void doHelp() {
    printf("Available commands:\n");
    printf("  look               - Look around the room\n");
    printf("  go <direction>     - Move to another room (north, south, east, west, up, down)\n");
    printf("  take <item>        - Pick up an item from the ground\n");
    printf("  drop <item>        - Drop an item onto the ground\n");
    printf("  inventory (inv)    - Show your inventory\n");
    printf("  stats              - Show player stats\n");
    printf("  attack             - Attack a monster if present\n");
    printf("  use <item>         - Use an item (e.g., potion)\n");
    printf("  save               - Save the game\n");
    printf("  load               - Load the game\n");
    printf("  help               - Show this help text\n");
    printf("  quit / exit        - Quit the game\n");
}

/* COMMAND: save */
void doSave() {
    FILE *f = fopen(SAVE_FILE_NAME, "wb");
    if (!f) {
        printf("Failed to save the game.\n");
        return;
    }
    
    /* Save player data */
    fwrite(&g_player, sizeof(Player), 1, f);
    
    /* Save room data */
    fwrite(&g_roomCount, sizeof(int), 1, f);
    fwrite(&g_rooms, sizeof(Room), g_roomCount, f);
    
    fclose(f);
    printf("Game saved.\n");
}

/* COMMAND: load */
void doLoad() {
    FILE *f = fopen(SAVE_FILE_NAME, "rb");
    if (!f) {
        printf("No save file found or cannot open the file.\n");
        return;
    }
    
    /* Load player data */
    fread(&g_player, sizeof(Player), 1, f);
    
    /* Load room data */
    fread(&g_roomCount, sizeof(int), 1, f);
    fread(&g_rooms, sizeof(Room), g_roomCount, f);
    
    fclose(f);
    printf("Game loaded.\n");
}

/*****************************************************************************
 * UTILITY & HELPER FUNCTIONS
 *****************************************************************************/

/* Find item in room by name, return index or -1 if not found */
int findItemInRoom(Room *room, const char *itemName) {
    for (int i = 0; i < room->itemCount; i++) {
        if (strcasecmp(room->itemsInRoom[i].name, itemName) == 0) {
            return i;
        }
    }
    return -1;
}

/* Find item in inventory by name, return index or -1 if not found */
int findItemInInventory(Inventory *inv, const char *itemName) {
    for (int i = 0; i < inv->count; i++) {
        if (strcasecmp(inv->items[i].name, itemName) == 0) {
            return i;
        }
    }
    return -1;
}

/* Remove item from room at index, shifting the rest */
void removeItemFromRoom(Room *room, int index) {
    if (index < 0 || index >= room->itemCount) return;
    for (int i = index; i < room->itemCount - 1; i++) {
        room->itemsInRoom[i] = room->itemsInRoom[i+1];
    }
    room->itemCount--;
}

/* Remove item from inventory at index, shifting the rest */
void removeItemFromInventory(Inventory *inv, int index) {
    if (index < 0 || index >= inv->count) return;
    for (int i = index; i < inv->count - 1; i++) {
        inv->items[i] = inv->items[i+1];
    }
    inv->count--;
}

/* Add an item to inventory */
void addItemToInventory(Inventory *inv, Item item) {
    if (inv->count >= MAX_INVENTORY_SIZE) {
        return; /* already full */
    }
    inv->items[inv->count] = item;
    inv->count++;
}

/* Add item to room */
void addItemToRoom(Room *room, Item item) {
    if (room->itemCount >= MAX_INVENTORY_SIZE) {
        return;
    }
    room->itemsInRoom[room->itemCount] = item;
    room->itemCount++;
}

/* Convert direction string to index (north=0, south=1, etc.) */
int getExitIndexByName(const char *exitName) {
    if (strcmp(exitName, "north") == 0) return DIR_NORTH;
    if (strcmp(exitName, "south") == 0) return DIR_SOUTH;
    if (strcmp(exitName, "east") == 0)  return DIR_EAST;
    if (strcmp(exitName, "west") == 0)  return DIR_WEST;
    if (strcmp(exitName, "up") == 0)    return DIR_UP;
    if (strcmp(exitName, "down") == 0)  return DIR_DOWN;
    return -1;
}

/* Simple monster combat logic */
void combatWithMonster(Monster *monster) {
    if (monster->state == MONSTER_DEAD) {
        printf("The monster is already dead.\n");
        return;
    }
    
    /* Player attacks first */
    int playerDamage = randomInRange(g_player.attackPower / 2, g_player.attackPower);
    monster->hp -= playerDamage;
    printf("You deal %d damage to the %s!\n", playerDamage, monster->name);
    
    if (monster->hp <= 0) {
        monster->hp = 0;
        monster->state = MONSTER_DEAD;
        return;
    }
    
    /* Monster attacks back if not dead */
    int monsterDamage = randomInRange(monster->attackPower / 2, monster->attackPower);
    g_player.hp -= monsterDamage;
    printf("The %s hits you for %d damage!\n", monster->name, monsterDamage);
}

/* Level up logic */
void levelUp(Player *p) {
    p->level++;
    p->exp = 0;
    p->expToNextLevel += 10; /* or some formula */
    p->maxHp += 10;
    p->maxMp += 5;
    p->hp = p->maxHp;
    p->mp = p->maxMp;
    p->attackPower += 2;
    printf("Congratulations! You are now level %d!\n", p->level);
}

/* Spawn a random monster in a room */
void spawnMonster(Room *room) {
    room->monsterPresent = 1;
    strcpy(room->monster.name, "Goblin");
    room->monster.level = randomInRange(1, 3);
    room->monster.maxHp = 10 + 5 * room->monster.level;
    room->monster.hp = room->monster.maxHp;
    room->monster.attackPower = 3 + 2 * room->monster.level;
    room->monster.state = MONSTER_AGGRESSIVE;
}

/* Return random integer in [min, max] */
int randomInRange(int min, int max) {
    if (max < min) {
        int temp = max;
        max = min;
        min = temp;
    }
    return (rand() % (max - min + 1)) + min;
}

/* Clear stdin buffer (optional) */
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* Trim leading/trailing whitespace */
char *trimWhitespace(char *str) {
    /* leading */
    while(isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == 0) {
        return str;
    }
    
    /* trailing */
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    end[1] = '\0';
    return str;
}

/* Convert string to lowercase in-place */
void strToLower(char *str) {
    while (*str) {
        *str = (char)tolower(*str);
        str++;
    }
}

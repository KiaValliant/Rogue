#include <ncurses.h>
#include <stdbool.h>
#include "mainh.h"
// #include "menu.h"
#ifndef GAME_H
#define GAME_H

#define MAX_TITLE_LEN 100
#define MIN_ROOMS 7
#define MAX_ROOMS 9
#define MAX_FLOORS 2
#define MIN_ROOM_LEN 4
#define MAX_ROOM_LEN 10
#define UMSG_X 10
#define UMSG_Y 0
#define MAX_DOORS 2
#define OT_COLS COLS / 3
#define OT_LINES (LINES - 4) / 3
#define AREA_0_X 0 * OT_COLS
#define AREA_0_Y 0 * OT_LINES + 2
#define AREA_1_X 1 * OT_COLS
#define AREA_1_Y 0 * OT_LINES + 2
#define AREA_2_X 2 * OT_COLS
#define AREA_2_Y 0 * OT_LINES + 2
#define AREA_3_X 2 * OT_COLS
#define AREA_3_Y 1 * OT_LINES + 2
#define AREA_4_X 1 * OT_COLS
#define AREA_4_Y 1 * OT_LINES + 2
#define AREA_5_X 0 * OT_COLS
#define AREA_5_Y 1 * OT_LINES + 2
#define AREA_6_X 0 * OT_COLS
#define AREA_6_Y 2 * OT_LINES + 2
#define AREA_7_X 1 * OT_COLS
#define AREA_7_Y 2 * OT_LINES + 2
#define AREA_8_X 2 * OT_COLS
#define AREA_8_Y 2 * OT_LINES + 2
#define MAX_MAPS_LEN 150
#define MAX_QUEUE_SIZE (MAX_MAPS_LEN * MAX_MAPS_LEN)
#define MAX_CORRS 9
#define HOR 0
#define VER 1
#define RT_REGULAR 0 // RT: room type
#define RT_ENCHANT 1
#define RT_TREASURE 2
#define MAX_TRAPS 20
#define MAX_ITEMS 100
#define MAX_WEAPONS 1000
#define FULL_HP 100
#define FULL_FEED 100
#define LIST_Y LINES / 2 - 10
#define LIST_X COLS / 2 - 20
#define LIST_HEIGHT 20
#define LIST_WIDTH 40
#define LIST_MSG_Y 4
#define LIST_MSG_X 17
#define LIST_FI_Y 6
#define LIST_FI_X 3
#define HP_MSG_Y LINES - 1
#define HP_MSG_X 3
#define PASSWORD_SIZE 5
#define G_HP_UNIT L"❤️"
#define G_FEED_UNIT L"🍗"
#define G_GOLD_UNIT L"🪙"
#define G_PLAYER 'P' | COLOR_PAIR(ORANGE)
#define G_DOT '.' | COLOR_PAIR(WHITE)
#define G_BUTTON '&' | COLOR_PAIR(CYAN) | A_BLINK
#define G_TRAP '^' | COLOR_PAIR(RED)
#define G_PILLAR 'O' | COLOR_PAIR(WHITE) | A_REVERSE
#define G_WINDOW '=' | COLOR_PAIR(WHITE)
#define G_STAIRCASE '<' | COLOR_PAIR(GREEN) | A_BLINK | A_REVERSE
#define G_TREASURE_S '$' | COLOR_PAIR(GOLD) | A_BLINK | A_REVERSE
#define D_SIMPLE '+' | COLOR_PAIR(WHITE)
#define D_SECRET '?' | COLOR_PAIR(WHITE)
#define D_OPENED_P '@' | COLOR_PAIR(GREEN)
#define D_LOCKED_P '@' | COLOR_PAIR(RED)
#define DT_SIMPLE 0
#define DT_SECRET_S 1 // DT: door type
#define DT_FOUND_S 2
#define DT_LOCKED_P 3
#define DT_OPENED_P 4
#define I_GOLD 'o' | COLOR_PAIR(GOLD)
#define I_BGOLD 'o' | COLOR_PAIR(GRAY)
#define I_SIMPLE_FOOD 'f' | COLOR_PAIR(BLUE)
#define I_HEALTH_SPELL 's' | COLOR_PAIR(BLUE)
#define I_SPEED_SPELL 's' | COLOR_PAIR(MAGENTA)
#define I_DAMAGE_SPELL 's' | COLOR_PAIR(BROWN)
#define I_ANCIENT_KEY 'k' | COLOR_PAIR(GOLD)
#define IT_GOLD 0 // IT: item type
#define IT_SIMPLE_FOOD 1
#define IT_HEALTH_SPELL 2
#define IT_SPEED_SPELL 3
#define IT_DAMAGE_SPELL 4
#define IT_ANCIENT_KEY 5
#define IT_BGOLD 6
#define W_MACE '*' | COLOR_PAIR(GRAY)
#define W_DAGGER '/' | COLOR_PAIR(GRAY)
#define W_MAGIC_WAND '!' | COLOR_PAIR(CYAN)
#define W_NORMAL_ARROW '>' | COLOR_PAIR(GRAY)
#define W_SWORD '!' | COLOR_PAIR(GRAY)
#define WT_MACE 0 // WT: weapon type
#define WT_DAGGER 1
#define WT_MAGIC_WAND 2
#define WT_NORMAL_ARROW 3
#define WT_SWORD 4
#define W_MACE_DAMAGE 5
#define W_DAGGER_DAMAGE 12
#define W_MAGIC_WAND_DAMAGE 15
#define W_NORMAL_ARROW_DAMAGE 5
#define W_SWORD_DAMAGE 10
#define W_DAGGER_CA 10
#define W_MAGIC_WAND_CA 8
#define W_NORMAL_ARROW_CA 20
#define W_DAGGER_RANGE 5
#define W_MAGIC_WAND_RANGE 10
#define W_NORMAL_ARROW_RANGE 5
#define NO_WEAPON -1
#define M_DEAMON 'D' | COLOR_PAIR(RED)
#define M_FIRE_BREATHING 'F' | COLOR_PAIR(RED)
#define M_GIANT 'G' | COLOR_PAIR(RED)
#define M_SNAKE 'S' | COLOR_PAIR(RED)
#define M_UNDEED 'U' | COLOR_PAIR(RED)
#define MT_DEAMON 0
#define MT_FIRE_BREATHING 1
#define MT_GIANT 2
#define MT_SNAKE 3
#define MT_UNDEED 4
#define M_DEAMON_HP 5
#define M_FIRE_BREATHING_HP 10
#define M_GIANT_HP 15
#define M_SNAKE_HP 20
#define M_UNDEED_HP 30
#define G_EASY 0
#define G_NORMAL 1
#define G_HARD 2
#define M_DEAMON_DAMAGE_EASY 1
#define M_FIRE_BREATHING_DAMAGE_EASY 2
#define M_GIANT_DAMAGE_EASY 5
#define M_SNAKE_DAMAGE_EASY 7
#define M_UNDEED_DAMAGE_EASY 10
#define M_DEAMON_DAMAGE_NORMAL 2
#define M_FIRE_BREATHING_DAMAGE_NORMAL 3
#define M_GIANT_DAMAGE_NORMAL 7
#define M_SNAKE_DAMAGE_NORMAL 10
#define M_UNDEED_DAMAGE_NORMAL 14
#define M_DEAMON_DAMAGE_HARD 3
#define M_FIRE_BREATHING_DAMAGE_HARD 5
#define M_GIANT_DAMAGE_HARD 9
#define M_SNAKE_DAMAGE_HARD 12
#define M_UNDEED_DAMAGE_HARD 17
#define NO_MONSTER -1
#define TOP_RIGHT 0
#define TOP_LEFT 1
#define DOWN_LEFT 2
#define DOWN_RIGHT 3
#define TOP 4
#define DOWN 5
#define RIGHT 6
#define LEFT 7

struct Point;
struct Player;
struct Item;
struct Room;
struct Corridor;
struct Map;


typedef struct Point {
    int x, y;
    int door_type;
    bool is_triggered, // this is for traps
        is_pressed,    // this is for generate password button
        is_door,       // this is for corridors
        is_reveald;    // this is for corridor blocks
} Point;

typedef struct Monster {
    int type;
    Point pos;
    int dir;
    int HP;
    int area;
} Monster;

typedef struct Item {
    int type;
    Point pos;  // for room
    int amount; // for player
    bool is_taken;
    bool is_broken; // for Ancient Key
    bool is_used;   // for Ancient Key
} Item;

typedef struct Weapon {
    int type;
    Point pos;
    int collect_amount;
    bool is_taken;
    bool is_used;
} Weapon;

typedef struct Player {
    Point pos;
    int dir;
    int HP;
    int feed;
    int gold_count, food_count, spell_count, ancient_key_count;
    Item items[MAX_ITEMS];
    int item_count;
    Weapon weapons[MAX_WEAPONS];
    int weapon_count;
    Weapon active_weapon;
    int curr_area;
    int curr_floor;
} Player;

typedef struct Room {
    int theme;
    int area;
    Point tlc;
    int length;
    Point doors[MAX_DOORS];
    int door_count;
    Point traps[MAX_TRAPS];
    int trap_count;
    Point button;
    Point pillar;
    Point window;
    Point staircase;
    Point treasure_symbol;
    Item items[MAX_ITEMS];
    int item_count;
    Weapon weapons[MAX_WEAPONS];
    int weapon_count;
    bool is_reveald;
    char password[PASSWORD_SIZE];
    Monster monster;
} Room;

typedef struct Corridor {
    Point blocks[MAX_MAPS_LEN * MAX_MAPS_LEN];
    int block_count;
    bool is_reveald;
} Corridor;

typedef struct Map {
    Room rooms[MAX_ROOMS];
    int room_count;
    Corridor corridors[MAX_CORRS];
} Map;

typedef struct GamePthread {
    Player *player;
    Map *map;
} GamePthread;

typedef struct GameSave {
    Player player;
    Map maps[MAX_FLOORS];
    char save_title[MAX_TITLE_LEN];
} GameSave;


void start_game();
void resume_game();
void game_won();
void game_over();
void create_map(Map *map);
void redraw_map(Map *map);
void redraw_screen(Map *map);
void connect_rooms(Point door1, Point door2, Map *map, int corr_index);
void check_position(Map *map);
bool check_collision(Point pos, int dir, bool is_player);
bool check_traps(Room *room, bool s_pressed);
Point *check_secret_doors(Room *room, bool s_pressed);
bool check_button(Map *map, Room *room);
bool check_password_doors(Map *map, Room *room);
bool check_items(Room *room);
bool check_weapons(Room *room);
bool check_staircase(Map *map, Room *room);
bool check_monster(Room *room);
bool check_treasure_symbol(Map *map, Room *room);
void treasure_room(Map *map);
void move_monster(Room *room);
bool monster_attack(Map *map, Room *room, int game_difficulty);
int weapon_count(int wtype);
void remove_weapon(int wtype);
void shoot_weapon(Monster *monster, Map *map, bool damage_spell_used);
bool use_ancient_key();
bool unlock_door(Map *map, Room *room, int index);
void *generate_password_thread(void *arg);
void generate_password();
Room *generate_room(int area);
Item *random_point(Room *room);
void itemize_regular_room(Map *map, Room *room);
void itemize_enchant_room(Map *map, Room *room);
void itemize_treasure_room(Map *map, Room *room);
Point *create_door(Map *map, Room *room, int index);
bool room_overlap(Room a, Room b);
bool points_neighborhood(Point a, Point b);
void M_mode_draw(Map *map);
void handle_input(Map *map);
WINDOW *list_window(const char *);
void food_list();
int spell_list();
void weapon_list();
void temp_room(Map *map);
void print_umsg(const char *format, ...);
void draw_player();
int block_index(Corridor corridor);
void reveal_blocks(Corridor *corridor, int index);
void draw_room(Room *room, int mode);
void draw_corridor(Corridor corridor, int mode);
void spawn_player(Room *room, int mode);
void init_items();
void debug_window(int y, int x, const char *title, int count, ...);
bool save_game_menu(Map *map);
void save_game(GameSave *gameSave);

#endif
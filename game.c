#include "game.h"
#include "mainh.h"
#include "menu.h"
#include <locale.h>
#include <math.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define DEBUG_PRINTS
#ifdef DEBUG_PRINTS
#define DEBUG(y, x, fmt, args...)                                              \
    mvprintw(y, x, fmt, ##args);                                               \
    fflush(stderr)
#else
#define DEBUG(y, x, fmt, args...)
#endif

extern UserInfo logged_in_user;
Player player;
bool M_mode = false;
Map maps[MAX_FLOORS];
chtype wall[2][3] = {{
                         '_' | COLOR_PAIR(ORANGE),
                         '_' | COLOR_PAIR(CYAN),
                         '_' | COLOR_PAIR(MAGENTA),
                     },
                     {
                         '|' | COLOR_PAIR(ORANGE),
                         '|' | COLOR_PAIR(CYAN),
                         '|' | COLOR_PAIR(MAGENTA),
                     }};

void start_game() {
    initscr();
    setlocale(LC_ALL, "");
    curs_set(0);
    player.curr_floor = 0;
    create_map(&maps[player.curr_floor]);
    clear();
    maps[player.curr_floor].rooms[player.curr_area].is_reveald = true;
    spawn_player(&maps[player.curr_floor].rooms[player.curr_area], 0);
    redraw_screen(&maps[player.curr_floor]);
    timeout(-1);
    noecho();
    while (true) {
        handle_input(&maps[player.curr_floor]);
    }
    endwin();
}

void handle_input(Map *map) {
    bool pressed[128] = {false};
    static int move_range = 1;
    static int spell_time_count[5] = {0};
    static bool damage_spell_used = false;
    bool no_dir_needed = (player.active_weapon.type == WT_MACE ||
                          player.active_weapon.type == WT_SWORD);
    static int spell_use = -1;
    nodelay(stdscr, TRUE);
    timeout(200);
    int ch = getch();
    if (ch != ERR) {
        pressed[ch] = true;
    }
    if (!pressed['f'] && !pressed['g'] && !pressed[' '] && !pressed['M'] &&
            !pressed['s'] && !pressed['E'] && !pressed['R'] && !pressed['i'] &&
            !pressed['A'] && !pressed['w'] && !pressed['>'] ||
        pressed['s']) {
        player.dir = ch;
    } else if (pressed['A']) {
        save_game_menu(map);
    } else if (pressed['M']) {
        if (M_mode) {
            M_mode = false;
        } else {
            M_mode = true;
        }
        redraw_screen(map);
    } else if (pressed['R']) {
        spell_use = spell_list();
        if (spell_use == IT_DAMAGE_SPELL) {
            damage_spell_used = true;
        } else if (spell_use == IT_SPEED_SPELL) {
            move_range = 2;
        }
    } else if (pressed['E']) {
        food_list();
    } else if (pressed['w']) {
        player.active_weapon.type = NO_WEAPON;
        draw_player();
    } else if (pressed['i']) {
        weapon_list();
        return;
    } else if (pressed['>']) {
        check_staircase(map, &map->rooms[player.curr_area]);
    } else if (!no_dir_needed || !pressed[' ']) {
        player.dir = getch();
    }
    flushinp();
    if (player.dir != ERR || no_dir_needed && pressed[' ']) {
        static int last_damage_count = 0;
        if (last_damage_count++ >= 50 && last_damage_count <= 150 &&
            player.feed >= 100) {
            if (player.HP <= FULL_HP - 1 && spell_use != IT_HEALTH_SPELL) {
                player.HP += 1;
            } else if (player.HP <= FULL_HP - 2 &&
                       spell_use == IT_HEALTH_SPELL) {
                player.HP += 2;
                spell_time_count[IT_HEALTH_SPELL]++;
                if (spell_time_count[IT_HEALTH_SPELL] >= 10) {
                    spell_time_count[IT_HEALTH_SPELL] = 0;
                    spell_use = -1;
                }
            } else {
                player.HP = FULL_HP;
            }
            if (last_damage_count == 150) {
                last_damage_count = 0;
            }
        }
        if (check_password_doors(map, &map->rooms[player.curr_area])) {
            redraw_screen(map);
            return;
        }
        if (check_secret_doors(&map->rooms[player.curr_area], pressed['s'])) {
            return;
        }
        if (check_collision(player.pos, player.dir, true))
            return;
        do {
            if (!pressed[' '] && spell_time_count[IT_SPEED_SPELL]++ <= 10) {
                if (spell_time_count[IT_SPEED_SPELL] > 10) {
                    spell_time_count[IT_SPEED_SPELL] = 0;
                    spell_use = -1;
                    move_range = 1;
                }
                switch (player.dir) {
                case 'j':
                    player.pos.y -= move_range;
                    move_monster(&map->rooms[player.curr_area]);
                    break;
                case 'u':
                    player.pos.x++;
                    player.pos.y--;
                    move_monster(&map->rooms[player.curr_area]);
                    break;
                case 'l':
                    player.pos.x += move_range;
                    move_monster(&map->rooms[player.curr_area]);
                    break;
                case 'n':
                    player.pos.x++;
                    player.pos.y++;
                    move_monster(&map->rooms[player.curr_area]);
                    break;
                case 'k':
                    player.pos.y += move_range;
                    move_monster(&map->rooms[player.curr_area]);
                    if (monster_attack(map, &map->rooms[player.curr_area],
                                       G_NORMAL)) {
                        last_damage_count = 0;
                    }
                    break;
                case 'b':
                    player.pos.x--;
                    player.pos.y++;
                    move_monster(&map->rooms[player.curr_area]);
                    if (monster_attack(map, &map->rooms[player.curr_area],
                                       G_NORMAL)) {
                        last_damage_count = 0;
                    }
                    break;
                case 'h':
                    player.pos.x -= move_range;
                    move_monster(&map->rooms[player.curr_area]);
                    if (monster_attack(map, &map->rooms[player.curr_area],
                                       G_NORMAL)) {
                        last_damage_count = 0;
                    }

                    break;
                case 'y':
                    player.pos.x--;
                    player.pos.y--;
                    move_monster(&map->rooms[player.curr_area]);
                    if (monster_attack(map, &map->rooms[player.curr_area],
                                       G_NORMAL)) {
                        last_damage_count = 0;
                    }
                    break;
                default:
                    break;
                }
            }
            redraw_screen(map);
            int last_hit;
            if (pressed[' '] && !pressed['a']) {
                last_hit = player.dir;
                shoot_weapon(&map->rooms[player.curr_area].monster, map,
                             damage_spell_used);
                if (spell_time_count[IT_DAMAGE_SPELL]++ > 10) {
                    spell_use = -1;
                    damage_spell_used = false;
                    spell_time_count[IT_DAMAGE_SPELL] = 0;
                }
            } else if (pressed['a']) {
                player.dir = last_hit;
                shoot_weapon(&map->rooms[player.curr_area].monster, map,
                             damage_spell_used);
                if (spell_time_count[IT_DAMAGE_SPELL]++ > 10) {
                    spell_use = -1;
                    damage_spell_used = false;
                    spell_time_count[IT_DAMAGE_SPELL] = 0;
                }
            }
            if (monster_attack(map, &map->rooms[player.curr_area], G_NORMAL)) {
                last_damage_count = 0;
            }
            redraw_screen(map);
            if (check_traps(&map->rooms[player.curr_area], pressed['s'])) {
                last_damage_count = 0;
            }
            check_position(map);
            if (!pressed['g']) {
                check_button(map, &map->rooms[player.curr_area]);
                check_items(&map->rooms[player.curr_area]);
                check_weapons(&map->rooms[player.curr_area]);
            }
            check_treasure_symbol(map, &map->rooms[player.curr_area]);
            redraw_screen(map);
            usleep(15000);
        } while (!check_collision(player.pos, player.dir, true) &&
                 pressed['f']);
    }
}

void resume_game() {}

void game_won() {
    WINDOW *lwin = list_window("!!! YOU WIN !!!");
    mvwaddwstr(lwin, LIST_FI_Y, LIST_WIDTH / 2 - 5, L"🏆🏆🏆🏆🏆🏆");
    wrefresh(lwin);
    sleep(5);
    delwin(lwin);
#ifdef _WIN32
    system("cls");
#else
    system("clear");
    system("reset");
#endif
    exit(EXIT_SUCCESS);
}

void game_over() {
    WINDOW *lwin = list_window("!!! GAME OVER !!!");
    mvwaddwstr(lwin, LIST_FI_Y, LIST_WIDTH / 2 - 5, L"☠☠☠☠☠☠");
    wrefresh(lwin);
    sleep(5);
    delwin(lwin);
#ifdef _WIN32
    system("cls");
#else
    system("clear");
    system("reset");
#endif
    exit(EXIT_SUCCESS);
}

void create_map(Map *map) {
    srand(time(NULL));
    int made_rooms = 0;
    if (player.curr_floor == 0) {
        map->room_count = rand() % 3 + 7;
    } else {
        map->room_count = maps[0].room_count;
    }

    if (player.curr_floor == 0) {
        player.curr_area = rand() % map->room_count;
    }

    // make rooms
    while (made_rooms < map->room_count) {
        Room *new_room = generate_room(made_rooms);
        bool overlap = false;

        for (int i = 0; i < made_rooms; i++) {
            if (room_overlap(*new_room, map->rooms[i])) {
                overlap = true;
                break;
            }
        }

        if (!overlap) {
            map->rooms[made_rooms++] = *new_room;
        }
    }

    // itemize rooms
    for (int i = 0; i < map->room_count; i++) {
        if (map->rooms[i].theme == RT_REGULAR) {
            redraw_map(map);
            itemize_regular_room(map, &map->rooms[i]);
            redraw_map(map);
        } else if (map->rooms[i].theme == RT_ENCHANT) {
            redraw_map(map);
            itemize_enchant_room(map, &map->rooms[i]);
            redraw_map(map);
        }
    }
    int ak_area = rand() % map->room_count; // drop ancient key
    Item *item = random_point(&map->rooms[ak_area]);
    item->type = IT_ANCIENT_KEY;
    item->is_taken = false;
    item->is_used = false;
    item->is_broken = false;
    map->rooms[ak_area].items[map->rooms[ak_area].item_count++] = *item;
    free(item);
    redraw_map(map);

    // make doors
    int corridor_count = 0;
    for (int i = 0; i < map->room_count - 1; i++) {
        Point *door1 =
            create_door(map, &map->rooms[i], map->rooms[i].door_count++);
        Point *door2 = create_door(map, &map->rooms[i + 1],
                                   map->rooms[i + 1].door_count++);
    }
    redraw_map(map);

    // make corridors
    for (int i = 0; i < map->room_count - 1; i++) {
        if (i == 0) {
            connect_rooms(map->rooms[i].doors[0], map->rooms[i + 1].doors[0],
                          map, i);
            redraw_map(map);
        } else {
            connect_rooms(map->rooms[i].doors[1], map->rooms[i + 1].doors[0],
                          map, i);
            redraw_map(map);
        }
    }

    // make staircase
    if (player.curr_floor == 0) {
        int sc_area;
        do {
            sc_area = rand() % (map->room_count - 1) + 1;
        } while (sc_area == player.curr_area);
        Item *point = random_point(&map->rooms[sc_area]);
        map->rooms[sc_area].staircase.x = point->pos.x;
        map->rooms[sc_area].staircase.y = point->pos.y;
        free(point);
    }

    // make treasure symbol
    if (player.curr_floor == 1) {
        int ts_area = rand() % (map->room_count - 1) + 1;
        Item *point = random_point(&map->rooms[ts_area]);
        map->rooms[ts_area].treasure_symbol.x = point->pos.x;
        map->rooms[ts_area].treasure_symbol.y = point->pos.y;
        free(point);
    }
}

Point *create_door(Map *map, Room *room, int index) {
    if (index >= MAX_DOORS)
        return NULL;
    int x = room->tlc.x, y = room->tlc.y, length = room->length,
        area = room->area;
    if (area == 2 && index == 1 || area == 5 && index == 1) // down wall
    {
        room->doors[index].x = x + (rand() % length) + 1;
        room->doors[index].y = y + length + 1;
    } else if (area == 0 && index == 0 || area == 1 && index == 1 ||
               area == 4 && index == 0 || area == 5 && index == 0 ||
               area == 6 && index == 1 || area == 7 && index == 1) // right wall
    {
        room->doors[index].x = x + length + 1;
        room->doors[index].y = y + (rand() % length) + 1;
    } else if (area == 3 && index == 0 || area == 6 && index == 0) // up wall
    {
        room->doors[index].x = x + (rand() % length) + 1;
        room->doors[index].y = y;
    } else if (area == 1 && index == 0 || area == 2 && index == 0 ||
               area == 3 && index == 1 || area == 4 && index == 1 ||
               area == 7 && index == 0 || area == 8 && index == 0) // left wall
    {
        room->doors[index].x = x;
        room->doors[index].y = y + (rand() % length) + 1;
    }
    room->doors[index].is_door = true;

    // door type
    if (area < player.curr_area && area != 0 && index != 1 ||
        area < player.curr_area && area == 0 && index != 0 ||
        area > player.curr_area && index != 0 || area == player.curr_area) {
        if (area >= player.curr_area && area + 1 < map->room_count &&
                map->rooms[area + 1].theme == RT_ENCHANT && index == 1 &&
                area != 0 ||
            area <= player.curr_area && area - 1 >= 0 &&
                map->rooms[area - 1].theme == RT_ENCHANT && index == 0 &&
                area != 0 ||
            area == 0 && index == 0 &&
                map->rooms[area + 1].theme == RT_ENCHANT) {
            room->doors[index].door_type = DT_SECRET_S;
            Point *result = &room->doors[index];
            return result;
        }
        int type_probabality = rand() % 5;
        if (type_probabality == 0) {
            room->doors[index].door_type = DT_LOCKED_P;
            Item *point = random_point(room);
            room->button.x = point->pos.x;
            room->button.y = point->pos.y;
            free(point);
            Point *result = &room->doors[index];
            return result;
        }
    }

    room->doors[index].door_type = DT_SIMPLE;
    Point *result = &room->doors[index];
    return result;
}

bool room_overlap(Room a, Room b) {
    return !(
        a.tlc.x + a.length + 6 < b.tlc.x || a.tlc.x > b.tlc.x + b.length + 6 ||
        a.tlc.y + a.length + 6 < b.tlc.y || a.tlc.y > b.tlc.y + b.length + 6);
}

bool points_neighborhood(Point a, Point b) {
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    return ((dy < 2) && (dx < 2));
}

Item *random_point(Room *room) {
    Item *point = (Item *)malloc(sizeof(Item));
    int x = room->tlc.x, y = room->tlc.y, length = room->length;
    int count = 0;
    while (true) {
        count++;
        point->pos.x = x + rand() % length + 1;
        point->pos.y = y + rand() % length + 1;
        int ch = mvinch(point->pos.y, point->pos.x) & A_CHARTEXT;
        if (ch == '.')
            break;
        if (count >= 1000)
            break;
    }
    return point;
}

void itemize_regular_room(Map *map, Room *room) {
    redraw_map(map);

    // gold
    int gold_count = rand() % 3;
    for (int i = 0; i < gold_count; i++) {
        Item *item = random_point(room);
        int bgold_probabality = rand() % 10;
        if (bgold_probabality == 0) {
            item->type = IT_BGOLD;
            item->amount = rand() % 6 + 25;
        } else {
            item->type = IT_GOLD;
            item->amount = rand() % 6 + 10;
        }
        item->is_taken = false;
        room->items[room->item_count++] = *item;
        free(item);
        redraw_map(map);
    }

    // food
    int food_count = rand() % 3;
    for (int i = 0; i < food_count; i++) {
        Item *item = random_point(room);
        item->type = IT_SIMPLE_FOOD;
        item->is_taken = false;
        item->is_used = false;
        room->items[room->item_count++] = *item;
        free(item);
        redraw_map(map);
    }

    //  spell
    int spell_count = rand() % 2;
    for (int i = 0; i < spell_count; i++) {
        Item *item = random_point(room);
        int type_probabality = rand() % 3;
        if (type_probabality == 0) {
            item->type = IT_HEALTH_SPELL;
        } else if (type_probabality == 1) {
            item->type = IT_SPEED_SPELL;
        } else if (type_probabality == 2) {
            item->type = IT_DAMAGE_SPELL;
        }
        item->is_taken = false;
        item->is_used = false;
        room->items[room->item_count++] = *item;
        free(item);
        redraw_map(map);
    }

    // trap
    int trap_count = rand() % 2;
    for (int i = 0; i < trap_count; i++) {
        Item *point = random_point(room);
        room->traps[room->trap_count].x = point->pos.x;
        room->traps[room->trap_count].y = point->pos.y;
        free(point);
        room->traps[room->trap_count++].is_triggered = false;
        redraw_map(map);
    }

    // weapon
    int weapon_count = rand() % 2;
    for (int i = 0; i < weapon_count; i++) {
        Item *point = random_point(room);
        room->weapons[room->weapon_count].pos.x = point->pos.x;
        room->weapons[room->weapon_count].pos.y = point->pos.y;
        free(point);
        int type_probabality = rand() % 4 + 1;
        if (type_probabality == 0) {
            room->weapons[room->weapon_count].type = WT_MACE;
        } else if (type_probabality == 1) {
            room->weapons[room->weapon_count].type = WT_DAGGER;
        } else if (type_probabality == 2) {
            room->weapons[room->weapon_count].type = WT_MAGIC_WAND;
        } else if (type_probabality == 3) {
            room->weapons[room->weapon_count].type = WT_NORMAL_ARROW;
        } else if (type_probabality == 4) {
            room->weapons[room->weapon_count].type = WT_SWORD;
        }
        room->weapons[room->weapon_count].is_used = false;
        room->weapons[room->weapon_count].is_taken = false;
        room->weapon_count++;
        redraw_map(map);
    }

    // monster
    int monster_probabality = rand() % 2;
    if (monster_probabality == 0) {
        Item *point = random_point(room);
        room->monster.pos.x = point->pos.x;
        room->monster.pos.y = point->pos.y;
        free(point);
        int type_probabality = rand() % 4;
        if (type_probabality == 0) {
            room->monster.type = MT_DEAMON;
            room->monster.HP = M_DEAMON_HP;
        } else if (type_probabality == 1) {
            room->monster.type = MT_FIRE_BREATHING;
            room->monster.HP = M_FIRE_BREATHING_HP;
        } else if (type_probabality == 2) {
            room->monster.type = MT_GIANT;
            room->monster.HP = M_GIANT_HP;
        } else if (type_probabality == 3) {
            room->monster.type = MT_SNAKE;
            room->monster.HP = M_SNAKE_HP;
        }
    } else {
        room->monster.type = NO_MONSTER;
    }
    redraw_map(map);
}

void itemize_enchant_room(Map *map, Room *room) {
    //  spell
    int spell_count = rand() % 3 + 2;

    for (int i = 0; i < spell_count; i++) {
        Item *item = random_point(room);
        int type_probabality = rand() % 3;
        if (type_probabality == 0) {
            item->type = IT_HEALTH_SPELL;
        } else if (type_probabality == 1) {
            item->type = IT_SPEED_SPELL;
        } else if (type_probabality == 2) {
            item->type = IT_DAMAGE_SPELL;
        }
        item->is_taken = false;
        room->items[room->item_count++] = *item;
        free(item);
        redraw_map(map);
    }
}

void itemize_treasure_room(Map *map, Room *room) {
    redraw_screen(map);

    // make traps
    int trap_count = rand() % 3 + 4;
    for (int i = 0; i < trap_count; i++) {
        Item *point = random_point(room);
        room->traps[i].x = point->pos.x;
        room->traps[i].y = point->pos.y;
        free(point);
        room->traps[i].is_triggered = false;
        redraw_screen(map);
    }

    // make monster
    Item *point = random_point(room);
    room->monster.pos.x = point->pos.x;
    room->monster.pos.y = point->pos.y;
    free(point);
    room->monster.type = MT_UNDEED;
    room->monster.HP = M_UNDEED_HP;
    redraw_screen(map);
}

Room *generate_room(int area) {
    Room *room = (Room *)malloc(sizeof(Room));
    room->length = rand() % 4 + 4;
    room->area = area;
    room->item_count = 0;
    room->trap_count = 0;
    room->weapon_count = 0;

    // room area
    switch (area) {
    case 0:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_0_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_0_Y;
        break;
    case 1:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_1_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_1_Y;
        break;
    case 2:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_2_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_2_Y;
        break;
    case 3:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_3_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_3_Y;
        break;
    case 4:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_4_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_4_Y;
        break;
    case 5:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_5_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_5_Y;
        break;
    case 6:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_6_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_6_Y;
        break;
    case 7:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_7_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_7_Y;
        break;
    case 8:
        room->tlc.x = rand() % (OT_COLS - room->length - 1) + AREA_8_X;
        room->tlc.y = rand() % (OT_LINES - room->length - 1) + AREA_8_Y;
        break;
    default:
        break;
    }
    room->door_count = 0;
    room->is_reveald = false;

    // room theme
    int theme_probabality = rand() % 10;
    if (player.curr_area != area &&
        (theme_probabality == 0 || theme_probabality == 1 ||
         theme_probabality == 2)) {
        room->theme = RT_ENCHANT;
        return room;
    }

    // room pillar
    int pillar_probabality = rand() % 5;
    if (pillar_probabality == 0) {
        Item *point = random_point(room);
        room->pillar.x = point->pos.x;
        room->pillar.y = point->pos.y;
        free(point);
    }

    room->theme = RT_REGULAR;
    return room;
}

bool is_valid(int x, int y, Map *map) {
    chtype ch = mvinch(y, x) & A_CHARTEXT;
    chtype ch1 = mvinch(y - 1, x) & A_CHARTEXT;
    chtype ch2 = mvinch(y + 1, x) & A_CHARTEXT;
    chtype ch3 = mvinch(y, x - 1) & A_CHARTEXT;
    chtype ch4 = mvinch(y, x + 1) & A_CHARTEXT;
    chtype ch5 = mvinch(y - 1, x - 1) & A_CHARTEXT;
    chtype ch6 = mvinch(y + 1, x + 1) & A_CHARTEXT;
    chtype ch7 = mvinch(y + 1, x - 1) & A_CHARTEXT;
    chtype ch8 = mvinch(y - 1, x + 1) & A_CHARTEXT;
    if (ch1 == '#' || ch2 == '#' || ch3 == '#' || ch4 == '#' || ch5 == '#' ||
        ch6 == '#' || ch7 == '#' || ch8 == '#') {
        return false;
    }
    return x >= 0 && x < COLS && y >= 3 && y < LINES - 3 &&
           (ch == ' ' || ch == '+');
}

void connect_rooms(Point door1, Point door2, Map *map, int corr_index) {
    int dx[] = {0, 0, 1, -1};
    int dy[] = {-1, 1, 0, 0};

    bool visited[MAX_MAPS_LEN][MAX_MAPS_LEN] = {{false}};
    Point queue[MAX_QUEUE_SIZE];
    Point prev[MAX_MAPS_LEN][MAX_MAPS_LEN];

    int front = 0, rear = 0;
    queue[rear++] = door1;
    visited[door1.y][door1.x] = true;

    // return;
    for (int i = 0; i < MAX_MAPS_LEN; i++) {
        for (int j = 0; j < MAX_MAPS_LEN; j++) {
            prev[i][j] = (Point){-1, -1};
        }
    }

    while (front < rear) {
        Point current = queue[front++];

        if (current.x == door2.x && current.y == door2.y) {
            break;
        }

        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];
            Point point = (Point){nx, ny};

            if (is_valid(nx, ny, map) && !visited[ny][nx]) {
                visited[ny][nx] = true;
                queue[rear++] = (Point){nx, ny};
                prev[ny][nx] = current;

                if (rear >= MAX_QUEUE_SIZE) {
                    break;
                }
            }
        }
    }

    Point p = door2;
    int count = 0;

    while (!(p.x == door1.x && p.y == door1.y)) {
        if (count >= MAX_MAPS_LEN * MAX_MAPS_LEN) {
            break;
        }

        map->corridors[corr_index].blocks[count] = p;
        map->corridors[corr_index].blocks[count].is_reveald = false;
        count++;
        p = prev[p.y][p.x];
    }

    if (count < MAX_MAPS_LEN * MAX_MAPS_LEN) {
        map->corridors[corr_index].blocks[count++] = door1;
    }

    map->corridors[corr_index].block_count = count;
    map->corridors[corr_index].is_reveald = false;
}

void draw_room(Room *room, int mode) {
    call_colors();
    int x = room->tlc.x, y = room->tlc.y, length = room->length;

    // draw walls
    for (int i = x; i < x + length + 2; i++) {
        if (room->theme == RT_REGULAR) {
            mvaddch(y, i, wall[HOR][RT_REGULAR]);
            mvaddch(y + length + 1, i, wall[HOR][RT_REGULAR]);
        } else if (room->theme == RT_ENCHANT) {
            mvaddch(y, i, wall[HOR][RT_ENCHANT]);
            mvaddch(y + length + 1, i, wall[HOR][RT_ENCHANT]);
        } else if (room->theme == RT_TREASURE) {
            mvaddch(y, i, wall[HOR][RT_TREASURE]);
            mvaddch(y + length + 1, i, wall[HOR][RT_TREASURE]);
        }
    }

    for (int i = y + 1; i < y + length + 2; i++) {
        if (room->theme == RT_REGULAR) {
            mvaddch(i, x, wall[VER][RT_REGULAR]);
            mvaddch(i, x + length + 1, wall[VER][RT_REGULAR]);
        } else if (room->theme == RT_ENCHANT) {
            mvaddch(i, x, wall[VER][RT_ENCHANT]);
            mvaddch(i, x + length + 1, wall[VER][RT_ENCHANT]);
        } else if (room->theme == RT_TREASURE) {
            mvaddch(i, x, wall[VER][RT_TREASURE]);
            mvaddch(i, x + length + 1, wall[VER][RT_TREASURE]);
        }
    }

    // draw dots
    for (int i = x + 1; i < x + length + 1; i++) {
        for (int j = y + 1; j < y + length + 1; j++) {
            mvaddch(j, i, G_DOT);
        }
    }

    // draw doors
    if (mode == 0) // for connect_rooms
    {
        for (int i = 0; i < MAX_DOORS; i++) {
            if (room->doors[i].is_door) {
                mvaddch(room->doors[i].y, room->doors[i].x, D_SIMPLE);
            }
        }
        refresh();
    } else if (mode == 1) {
        int count;
        for (int i = 0; i < MAX_DOORS; i++) {
            // debug_window(0, 0, "1", 1, );
            if (room->doors[i].door_type == DT_SIMPLE) {
                mvaddch(room->doors[i].y, room->doors[i].x, D_SIMPLE);
            } else if (room->doors[i].door_type == DT_FOUND_S) {
                mvaddch(room->doors[i].y, room->doors[i].x, D_SECRET);
            } else if (room->doors[i].door_type == DT_LOCKED_P) {
                mvaddch(room->doors[i].y, room->doors[i].x, D_LOCKED_P);
            } else if (room->doors[i].door_type == DT_OPENED_P) {
                mvaddch(room->doors[i].y, room->doors[i].x, D_OPENED_P);
            }
        }
        refresh();
    }
    // draw button
    mvaddch(room->button.y, room->button.x, G_BUTTON);

    // draw pillar
    mvaddch(room->pillar.y, room->pillar.x, G_PILLAR);

    // draw window
    // mvaddch(room->window.y, room->window.x, G_WINDOW);

    // draw traps
    for (int i = 0; i < MAX_TRAPS; i++) {
        if (room->traps[i].is_triggered) {
            mvaddch(room->traps[i].y, room->traps[i].x, G_TRAP);
        }
    }

    // draw staircase
    mvaddch(room->staircase.y, room->staircase.x, G_STAIRCASE);

    // draw treasure symbol
    mvaddch(room->treasure_symbol.y, room->treasure_symbol.x, G_TREASURE_S);

    // draw weapons
    for (int i = 0; i < room->weapon_count; i++) {
        if (!room->weapons[i].is_taken) {
            switch (room->weapons[i].type) {
            case WT_MACE:
                mvaddch(room->weapons[i].pos.y, room->weapons[i].pos.x, W_MACE);
                break;
            case WT_DAGGER:
                mvaddch(room->weapons[i].pos.y, room->weapons[i].pos.x,
                        W_DAGGER);
                break;
            case WT_MAGIC_WAND:
                mvaddch(room->weapons[i].pos.y, room->weapons[i].pos.x,
                        W_MAGIC_WAND);
                break;
            case WT_NORMAL_ARROW:
                mvaddch(room->weapons[i].pos.y, room->weapons[i].pos.x,
                        W_NORMAL_ARROW);
                break;
            case WT_SWORD:
                mvaddch(room->weapons[i].pos.y, room->weapons[i].pos.x,
                        W_SWORD);
                break;
            }
        }
    }

    // draw items
    for (int i = 0; i < room->item_count; i++) {
        switch (room->items[i].type) {
        case IT_GOLD:
            if (!room->items[i].is_taken) {
                mvaddch(room->items[i].pos.y, room->items[i].pos.x, I_GOLD);
            }
            break;
        case IT_BGOLD:
            if (!room->items[i].is_taken) {
                mvaddch(room->items[i].pos.y, room->items[i].pos.x, I_BGOLD);
            }
            break;
        case IT_SIMPLE_FOOD:
            if (!room->items[i].is_taken) {
                mvaddch(room->items[i].pos.y, room->items[i].pos.x,
                        I_SIMPLE_FOOD);
            }
            break;
        case IT_HEALTH_SPELL:
            if (!room->items[i].is_taken) {
                mvaddch(room->items[i].pos.y, room->items[i].pos.x,
                        I_HEALTH_SPELL);
            }
            break;
        case IT_SPEED_SPELL:
            if (!room->items[i].is_taken) {
                mvaddch(room->items[i].pos.y, room->items[i].pos.x,
                        I_SPEED_SPELL);
            }
            break;
        case IT_DAMAGE_SPELL:
            if (!room->items[i].is_taken) {
                mvaddch(room->items[i].pos.y, room->items[i].pos.x,
                        I_DAMAGE_SPELL);
            }
            break;
        case IT_ANCIENT_KEY:
            if (!room->items[i].is_taken) {
                mvaddch(room->items[i].pos.y, room->items[i].pos.x,
                        I_ANCIENT_KEY);
            }
            break;
        }
    }

    // draw monster
    if (room->monster.type != NO_MONSTER) {
        switch (room->monster.type) {
        case MT_DEAMON:
            mvaddch(room->monster.pos.y, room->monster.pos.x, M_DEAMON);
            break;
        case MT_FIRE_BREATHING:
            mvaddch(room->monster.pos.y, room->monster.pos.x, M_FIRE_BREATHING);
            break;
        case MT_GIANT:
            mvaddch(room->monster.pos.y, room->monster.pos.x, M_GIANT);
            break;
        case MT_SNAKE:
            mvaddch(room->monster.pos.y, room->monster.pos.x, M_SNAKE);
            break;
        case MT_UNDEED:
            mvaddch(room->monster.pos.y, room->monster.pos.x, M_UNDEED);
            break;
        default:
            break;
        }
    }

    refresh();
}

void draw_corridor(Corridor corridor, int mode) {
    for (int i = 1; i < corridor.block_count - 1; i++) {
        if (mode == 0 && !corridor.blocks[i].is_door) {
            mvaddch(corridor.blocks[i].y, corridor.blocks[i].x, '#');
        }
        if (mode == 1 && !corridor.blocks[i].is_door &&
            corridor.blocks[i].is_reveald) {
            mvaddch(corridor.blocks[i].y, corridor.blocks[i].x, '#');
        }
    }
    refresh();
}

void init_items() {
    player.item_count = 0, player.gold_count = 0, player.food_count = 0,
    player.spell_count = 0, player.ancient_key_count = 0,
    player.weapon_count = 1;
    for (int i = 1; i < MAX_WEAPONS; i++) {
        player.weapons[i].type = NO_WEAPON;
    }
}

void spawn_player(Room *room, int mode) {
    if (mode == 0) {
        init_items();
        player.HP = FULL_HP;
        player.feed = FULL_FEED;
        player.weapons[0].type = WT_MACE;
        player.active_weapon.type = WT_MACE;
    }
    int x = room->tlc.x, y = room->tlc.y, length = room->length;
    Item *point = random_point(room);
    player.pos.x = point->pos.x;
    player.pos.y = point->pos.y;
    free(point);
}

bool monster_attack(Map *map, Room *room, int game_difficulty) {
    Point p = player.pos, m = room->monster.pos;
    if (!points_neighborhood(p, m)) {
        return false;
    }
    switch (room->monster.type) {
    case MT_DEAMON:
        if (player.HP > M_DEAMON_DAMAGE_NORMAL) {
            player.HP -= M_DEAMON_DAMAGE_NORMAL;
            redraw_screen(map);
            print_umsg("Ouch! The Deamon inflicted %d points of damage on you! "
                       "Be careful.",
                       M_DEAMON_DAMAGE_NORMAL);
        } else {
            player.HP = 0;
            redraw_screen(map);
            game_over();
        }
        break;
    case MT_FIRE_BREATHING:
        if (player.HP > M_FIRE_BREATHING_DAMAGE_NORMAL) {
            player.HP -= M_FIRE_BREATHING_DAMAGE_NORMAL;
            redraw_screen(map);
            print_umsg("Ouch! The Fire Breathing Monster inflicted %d points "
                       "of damage on you! Be careful.",
                       M_FIRE_BREATHING_DAMAGE_NORMAL);
        } else {
            player.HP = 0;
            redraw_screen(map);
            game_over();
        }
        break;
    case MT_GIANT:
        if (player.HP > M_GIANT_DAMAGE_NORMAL) {
            player.HP -= M_GIANT_DAMAGE_NORMAL;
            redraw_screen(map);
            print_umsg("Ouch! The Giant inflicted %d points of damage on you! "
                       "Be careful.",
                       M_GIANT_DAMAGE_NORMAL);
        } else {
            player.HP = 0;
            redraw_screen(map);
            game_over();
        }
        break;
    case MT_SNAKE:
        if (player.HP > M_SNAKE_DAMAGE_NORMAL) {
            player.HP -= M_SNAKE_DAMAGE_NORMAL;
            redraw_screen(map);
            print_umsg("Ouch! The Snake inflicted %d points of damage on you! "
                       "Be careful.",
                       M_SNAKE_DAMAGE_NORMAL);
        } else {
            player.HP = 0;
            redraw_screen(map);
            game_over();
        }
        break;
    case MT_UNDEED:
        if (player.HP > M_UNDEED_DAMAGE_NORMAL) {
            player.HP -= M_UNDEED_DAMAGE_NORMAL;
            redraw_screen(map);
            print_umsg("Ouch! The Undeed inflicted %d points of damage on you! "
                       "Be careful.",
                       M_UNDEED_DAMAGE_NORMAL);
        } else {
            player.HP = 0;
            redraw_screen(map);
            game_over();
        }
        break;
    default:
        break;
    }
    return true;
}

void move_monster(Room *room) {
    if (room->monster.type == NO_MONSTER || room->monster.type == MT_DEAMON ||
        room->monster.type == MT_FIRE_BREATHING)
        return;
    static int chase_count[MAX_ROOMS] = {0};
    Point p = player.pos;
    Point m = room->monster.pos;

    if (points_neighborhood(m, p)) {
        return;
    }
    int dx = abs(p.x - m.x);
    int dy = abs(p.y - m.y);
    if (dx == 1 || dy == 1) {
        return;
    }

    int situation;
    if (p.x > m.x && p.y > m.y) {
        situation = DOWN_RIGHT;
    } else if (p.x < m.x && p.y > m.y) {
        situation = DOWN_LEFT;
    } else if (p.x < m.x && p.y < m.y) {
        situation = TOP_LEFT;
    } else if (p.x > m.x && p.y < m.y) {
        situation = TOP_RIGHT;
    } else if (p.x == m.x && p.y < m.y) {
        situation = TOP;
    } else if (p.x == m.x && p.y > m.y) {
        situation = DOWN;
    } else if (p.x < m.x && p.y == m.y) {
        situation = LEFT;
    } else if (p.x > m.x && p.y == m.y) {
        situation = RIGHT;
    }

    if (room->monster.type == MT_GIANT) {
        if (chase_count[room->area]++ >= 5) {
            return;
        }
    }
    switch (situation) {
    case TOP_RIGHT:
        if (player.dir == 'j' || player.dir == 'k') {
            room->monster.dir = 'j';
            if (check_collision(room->monster.pos, room->monster.dir, false)) {
                return;
            }
            room->monster.pos.y--;
        } else if (player.dir == 'h' || player.dir == 'l') {
            room->monster.dir = 'l';
            if (check_collision(room->monster.pos, room->monster.dir, false)) {
                return;
            }
            room->monster.pos.x++;
        }
        break;
    case TOP_LEFT:
        if (player.dir == 'j' || player.dir == 'k') {
            room->monster.dir = 'j';
            if (check_collision(room->monster.pos, room->monster.dir, false)) {
                return;
            }
            room->monster.pos.y--;
        } else if (player.dir == 'h' || player.dir == 'l') {
            room->monster.dir = 'h';
            if (check_collision(room->monster.pos, room->monster.dir, false)) {
                return;
            }
            room->monster.pos.x--;
        }
        break;
    case DOWN_LEFT:
        if (player.dir == 'j' || player.dir == 'k') {
            room->monster.dir = 'k';
            if (check_collision(room->monster.pos, room->monster.dir, false)) {
                return;
            }
            room->monster.pos.y++;
        } else if (player.dir == 'h' || player.dir == 'l') {
            room->monster.dir = 'h';
            if (check_collision(room->monster.pos, room->monster.dir, false)) {
                return;
            }
            room->monster.pos.x--;
        }
        break;
    case DOWN_RIGHT:
        if (player.dir == 'j' || player.dir == 'k') {
            room->monster.dir = 'k';
            if (check_collision(room->monster.pos, room->monster.dir, false)) {
                return;
            }
            room->monster.pos.y++;
        } else if (player.dir == 'h' || player.dir == 'l') {
            room->monster.dir = 'l';
            if (check_collision(room->monster.pos, room->monster.dir, false)) {
                return;
            }
            room->monster.pos.x++;
        }
        break;
    case TOP:
        room->monster.dir = 'j';
        if (check_collision(room->monster.pos, room->monster.dir, false)) {
            return;
        }
        room->monster.pos.y--;
        break;
    case DOWN:
        room->monster.dir = 'k';
        if (check_collision(room->monster.pos, room->monster.dir, false)) {
            return;
        }
        room->monster.pos.y++;
        break;
    case RIGHT:
        room->monster.dir = 'l';
        if (check_collision(room->monster.pos, room->monster.dir, false)) {
            return;
        }
        room->monster.pos.x++;
        break;
    case LEFT:
        room->monster.dir = 'h';
        if (check_collision(room->monster.pos, room->monster.dir, false)) {
            return;
        }
        room->monster.pos.x--;
        break;
    default:
        break;
    }
    // }
}

int weapon_count(int wtype) {
    int count = 0;
    for (int i = 0; i < MAX_WEAPONS; i++) {
        if (player.weapons[i].type == wtype) {
            count++;
        }
    }
    return count;
}

void remove_weapon(int wtype) {
    for (int i = 0; i < MAX_WEAPONS; i++) {
        if (player.weapons[i].type == wtype) {
            player.weapons[i].type = NO_WEAPON;
            return;
        }
    }
}

void shoot_weapon(Monster *monster, Map *map, bool damage_spell_used) {
    int factor = 1;
    if (damage_spell_used) {
        factor = 2;
    }
    Point p = player.pos;
    Point m = monster->pos;
    char *monster_name;
    switch (monster->type) { // extract monster name
    case MT_DEAMON:
        monster_name = "Deamon";
        break;
    case MT_FIRE_BREATHING:
        monster_name = "Fire Breathing";
        break;
    case MT_GIANT:
        monster_name = "Giant";
        break;
    case MT_SNAKE:
        monster_name = "Snake";
        break;
    case MT_UNDEED:
        monster_name = "Undeed";
        break;
    default:
        break;
    }
    if (player.active_weapon.type == NO_WEAPON || monster->type == NO_MONSTER) {
        return;
    }
    if (player.active_weapon.type == WT_MACE) {
        if (p.x + 1 == m.x && p.y == m.y || p.x == m.x && p.y + 1 == m.y ||
            p.x == m.x && p.y - 1 == m.y || p.x - 1 == m.x && p.y == m.y ||
            p.x + 1 == m.x && p.y + 1 == m.y ||
            p.x - 1 == m.x && p.y - 1 == m.y ||
            p.x - 1 == m.x && p.y + 1 == m.y ||
            p.x + 1 == m.x && p.y - 1 == m.y) {
            if (monster->HP > factor * W_MACE_DAMAGE) {
                monster->HP -= factor * W_MACE_DAMAGE;
                print_umsg("Well done! You hit the %s! it's HP is %d now.",
                           monster_name, monster->HP);
            } else {
                monster->HP = 0;
                monster->type = NO_MONSTER;
                redraw_screen(map);
                print_umsg("Nailed it! You killed the %s!", monster_name);
            }
        }
    } else if (player.active_weapon.type == WT_SWORD) {
        if (p.x + 1 == m.x && p.y == m.y || p.x == m.x && p.y + 1 == m.y ||
            p.x == m.x && p.y - 1 == m.y || p.x - 1 == m.x && p.y == m.y ||
            p.x + 1 == m.x && p.y + 1 == m.y ||
            p.x - 1 == m.x && p.y - 1 == m.y ||
            p.x - 1 == m.x && p.y + 1 == m.y ||
            p.x + 1 == m.x && p.y - 1 == m.y) {
            if (monster->HP > factor * W_SWORD_DAMAGE) {
                monster->HP -= factor * W_SWORD_DAMAGE;
                print_umsg("Well done! You hit the %s! it's HP is %d now.",
                           monster_name, monster->HP);
            } else {
                monster->HP = 0;
                monster->type = NO_MONSTER;
                redraw_screen(map);
                print_umsg("Nailed it! You killed the %s!", monster_name);
            }
        }
    } else if (player.active_weapon.type == WT_DAGGER) {
        remove_weapon(WT_DAGGER);
        switch (player.dir) {
        case 'j':
            bool check = true;
            int i, ch = 0;
            for (i = 1; i <= W_DAGGER_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y - i, p.x);
                if (p.x == m.x && p.y - i == m.y) {
                    check = false;
                    if (monster->HP > factor * W_DAGGER_DAMAGE) {
                        monster->HP -= factor * W_DAGGER_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_DAGGER;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y - i + 2;
            }
            break;
        case 'k':
            check = true;
            ch = 0;
            for (i = 1; i <= W_DAGGER_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y + i, p.x);
                if (p.x == m.x && p.y + i == m.y) {
                    check = false;
                    if (monster->HP > factor * W_DAGGER_DAMAGE) {
                        monster->HP -= factor * W_DAGGER_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_DAGGER;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y + i - 2;
            }
            break;
        case 'l':
            check = true;
            ch = 0;
            for (i = 1; i <= W_DAGGER_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y, p.x + i);
                if (p.x + i == m.x && p.y == m.y) {
                    check = false;
                    if (monster->HP > factor * W_DAGGER_DAMAGE) {
                        monster->HP -= factor * W_DAGGER_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_DAGGER;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x + i - 2;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y;
            }
            break;
        case 'h':
            check = true;
            ch = 0;
            for (i = 1; i <= W_DAGGER_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y, p.x - i);
                if (p.x - i == m.x && p.y == m.y &&
                    monster->type != NO_MONSTER) {
                    check = false;
                    if (monster->HP > factor * W_DAGGER_DAMAGE) {
                        monster->HP -= factor * W_DAGGER_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_DAGGER;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x - i + 2;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y;
            }
            break;
        default:
            break;
        }
    } else if (player.active_weapon.type == WT_MAGIC_WAND) {
        remove_weapon(WT_MAGIC_WAND);
        switch (player.dir) {
        case 'j':
            bool check = true;
            int i, ch = 0;
            for (i = 1; i <= W_MAGIC_WAND_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y - i, p.x);
                if (p.x == m.x && p.y - i == m.y) {
                    check = false;
                    if (monster->HP > factor * W_MAGIC_WAND_DAMAGE) {
                        monster->HP -= factor * W_MAGIC_WAND_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_MAGIC_WAND;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y - i + 2;
            }
            break;
        case 'k':
            check = true;
            ch = 0;
            for (i = 1; i <= W_MAGIC_WAND_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y + i, p.x);
                if (p.x == m.x && p.y + i == m.y) {
                    check = false;
                    if (monster->HP > factor * W_MAGIC_WAND_DAMAGE) {
                        monster->HP -= factor * W_MAGIC_WAND_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_MAGIC_WAND;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y + i - 2;
            }
            break;
        case 'l':
            check = true;
            ch = 0;
            for (i = 1; i <= W_MAGIC_WAND_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y, p.x + i);
                if (p.x + i == m.x && p.y == m.y) {
                    check = false;
                    if (monster->HP > factor * W_MAGIC_WAND_DAMAGE) {
                        monster->HP -= factor * W_MAGIC_WAND_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_MAGIC_WAND;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x + i - 2;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y;
            }
            break;
        case 'h':
            check = true;
            ch = 0;
            for (i = 1; i <= W_MAGIC_WAND_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y, p.x - i);
                if (p.x - i == m.x && p.y == m.y &&
                    monster->type != NO_MONSTER) {
                    check = false;
                    if (monster->HP > factor * W_MAGIC_WAND_DAMAGE) {
                        monster->HP -= factor * W_MAGIC_WAND_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_MAGIC_WAND;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x - i + 2;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y;
            }
            break;
        default:
            break;
        }
    } else if (player.active_weapon.type == WT_NORMAL_ARROW) {
        remove_weapon(WT_NORMAL_ARROW);
        switch (player.dir) {
        case 'j':
            bool check = true;
            int i, ch = 0;
            for (i = 1; i <= W_NORMAL_ARROW_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y - i, p.x);
                if (p.x == m.x && p.y - i == m.y) {
                    check = false;
                    if (monster->HP > factor * W_NORMAL_ARROW_DAMAGE) {
                        monster->HP -= factor * W_NORMAL_ARROW_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_NORMAL_ARROW;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y - i + 2;
            }
            break;
        case 'k':
            check = true;
            ch = 0;
            for (i = 1; i <= W_NORMAL_ARROW_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y + i, p.x);
                if (p.x == m.x && p.y + i == m.y) {
                    check = false;
                    if (monster->HP > factor * W_NORMAL_ARROW_DAMAGE) {
                        monster->HP -= factor * W_NORMAL_ARROW_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_NORMAL_ARROW;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y + i - 2;
            }
            break;
        case 'l':
            check = true;
            ch = 0;
            for (i = 1; i <= W_NORMAL_ARROW_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y, p.x + i);
                if (p.x + i == m.x && p.y == m.y) {
                    check = false;
                    if (monster->HP > factor * W_NORMAL_ARROW_DAMAGE) {
                        monster->HP -= factor * W_NORMAL_ARROW_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_NORMAL_ARROW;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x + i - 2;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y;
            }
            break;
        case 'h':
            check = true;
            ch = 0;
            for (i = 1; i <= W_NORMAL_ARROW_RANGE &&
                        ((ch & A_CHARTEXT) != '|' && (ch & A_CHARTEXT) != '_' &&
                         (ch & A_CHARTEXT) != 'O');
                 i++) {
                ch = mvinch(p.y, p.x - i);
                if (p.x - i == m.x && p.y == m.y &&
                    monster->type != NO_MONSTER) {
                    check = false;
                    if (monster->HP > factor * W_NORMAL_ARROW_DAMAGE) {
                        monster->HP -= factor * W_NORMAL_ARROW_DAMAGE;
                        print_umsg(
                            "Well done! You hit the %s! it's HP is %d now.",
                            monster_name, monster->HP);
                    } else {
                        monster->HP = 0;
                        redraw_screen(map);
                        print_umsg("Nailed it! You killed the %s!",
                                   monster_name);
                        if (monster->type == MT_UNDEED) {
                            game_won();
                        }
                        monster->type = NO_MONSTER;
                    }
                }
            }
            if (check) {
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_taken = false;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .is_used = true;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .type = WT_NORMAL_ARROW;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count]
                    .pos.x = p.x - i + 2;
                map->rooms[player.curr_area]
                    .weapons[map->rooms[player.curr_area].weapon_count++]
                    .pos.y = p.y;
            }
            break;
        default:
            break;
        }
    }
}

void treasure_room(Map *map) {
    clear();
    for (int i = 0; i < map->room_count; i++) {
        if (i < map->room_count - 1) {
            memset(&map->corridors[i], 0, sizeof(Corridor));
        }
        memset(&map->rooms[i], 0, sizeof(Room));
    }
    map->rooms[4].area = 4;
    map->rooms[4].theme = RT_TREASURE;
    map->rooms[4].is_reveald = true;
    map->rooms[4].length = 8;
    map->rooms[4].tlc.x =
        rand() % (OT_COLS - map->rooms[4].length + 10) + AREA_4_X - 5;
    map->rooms[4].tlc.y =
        rand() % (OT_LINES - map->rooms[4].length + 10) + AREA_4_Y - 5;
    // M_mode = true;
    player.curr_area = 4;
    itemize_treasure_room(map, &map->rooms[4]);
    spawn_player(&map->rooms[4], 1);
    redraw_screen(map);
}

bool check_treasure_symbol(Map *map, Room *room) {
    int x = player.pos.x, y = player.pos.y;
    if (x == room->treasure_symbol.x && y == room->treasure_symbol.y) {
        treasure_room(map);
        return true;
    }
    return false;
}

bool check_monster(Room *room) {
    if (room->monster.type != NO_MONSTER) {
        switch (room->monster.type) {
        case MT_DEAMON:

            break;

        default:
            break;
        }
    }
}

bool check_collision(Point pos, int dir, bool is_player) {
    int ch;
    char cantMoveTo[20];
    if (is_player) {
        strcpy(cantMoveTo, "|_ @ODFGSU");
    } else {
        strcpy(cantMoveTo, "#+|_ @PO");
    }
    switch (dir) {
    case 'j':
        ch = mvinch(pos.y - 1, pos.x);
        if ((ch & A_CHARTEXT) == '@' && (ch & A_COLOR) >> 8 == 3) {
            return false;
        }
        if (strchr(cantMoveTo, ch)) {
            return true;
        }
        return false;
        break;
    case 'u':
        ch = mvinch(pos.y - 1, pos.x + 1);
        if ((ch & A_CHARTEXT) == '@' && (ch & A_COLOR) >> 8 == 3) {
            return false;
        }
        if (strchr(cantMoveTo, ch)) {
            return true;
        }
        return false;
        break;
    case 'l':
        ch = mvinch(pos.y, pos.x + 1);
        if ((ch & A_CHARTEXT) == '@' && (ch & A_COLOR) >> 8 == 3) {
            return false;
        }
        if (strchr(cantMoveTo, ch)) {
            return true;
        }
        return false;
        break;
    case 'n':
        ch = mvinch(pos.y + 1, pos.x + 1);
        if ((ch & A_CHARTEXT) == '@' && (ch & A_COLOR) >> 8 == 3) {
            return false;
        }
        if (strchr(cantMoveTo, ch)) {
            return true;
        }
        return false;
        break;
    case 'k':
        ch = mvinch(pos.y + 1, pos.x);
        if ((ch & A_CHARTEXT) == '@' && (ch & A_COLOR) >> 8 == 3) {
            return false;
        }
        if (strchr(cantMoveTo, ch)) {
            return true;
        }
        return false;
        break;
    case 'b':
        ch = mvinch(pos.y + 1, pos.x - 1);
        if ((ch & A_CHARTEXT) == '@' && (ch & A_COLOR) >> 8 == 3) {
            return false;
        }
        if (strchr(cantMoveTo, ch)) {
            return true;
        }
        return false;
        break;
    case 'h':
        ch = mvinch(pos.y, pos.x - 1);
        if ((ch & A_CHARTEXT) == '@' && (ch & A_COLOR) >> 8 == 3) {
            return false;
        }
        if (strchr(cantMoveTo, ch)) {
            return true;
        }
        return false;
        break;
    case 'y':
        ch = mvinch(pos.y - 1, pos.x - 1);
        if ((ch & A_CHARTEXT) == '@' && (ch & A_COLOR) >> 8 == 3) {
            return false;
        }
        if (strchr(cantMoveTo, ch)) {
            return true;
        }
        return false;
        break;
    default:
        return false;
        break;
    }
}

bool check_traps(Room *room, bool s_pressed) {
    if (!s_pressed) {
        for (int i = 0; i < 10; i++) {
            if (room->traps[i].x == player.pos.x &&
                room->traps[i].y == player.pos.y) {
                if (!room->traps[i].is_triggered) {
                    print_umsg("Oops! That's a trap. Watch your step, buddy!");
                    room->traps[i].is_triggered = true;
                    player.HP -= 10;
                    return true;
                }
            }
        }
    } else {
        for (int i = 0; i < MAX_DOORS; i++) {
            if (player.pos.x == room->traps[i].x &&
                    player.pos.y - 1 == room->traps[i].y &&
                    !room->traps[i].is_triggered ||
                player.pos.x + 1 == room->traps[i].x &&
                    player.pos.y == room->traps[i].y &&
                    !room->traps[i].is_triggered ||
                player.pos.x == room->traps[i].x &&
                    player.pos.y + 1 == room->traps[i].y &&
                    !room->traps[i].is_triggered ||
                player.pos.x - 1 == room->traps[i].x &&
                    player.pos.y == room->traps[i].y &&
                    !room->traps[i].is_triggered ||
                player.pos.x - 1 == room->traps[i].x &&
                    player.pos.y - 1 == room->traps[i].y &&
                    !room->traps[i].is_triggered ||
                player.pos.x + 1 == room->traps[i].x &&
                    player.pos.y + 1 == room->traps[i].y &&
                    !room->traps[i].is_triggered ||
                player.pos.x - 1 == room->traps[i].x &&
                    player.pos.y + 1 == room->traps[i].y &&
                    !room->traps[i].is_triggered ||
                player.pos.x + 1 == room->traps[i].x &&
                    player.pos.y - 1 == room->traps[i].y &&
                    !room->traps[i].is_triggered) {
                print_umsg("Oops! That's a trap. That was close!");
                room->traps[i].is_triggered = true;
                return false;
            }
        }
    }
    return false;
}

bool check_password_doors(Map *map, Room *room) {
    switch (player.dir) {
    case 'j':
        for (int i = 0; i < MAX_DOORS; i++) {
            if (player.pos.x == room->doors[i].x &&
                player.pos.y - 1 == room->doors[i].y &&
                room->doors[i].door_type == DT_LOCKED_P) {
                print_umsg(
                    "ACCESS DENIED! You need a password to unlock this door.");
                unlock_door(map, room, i);
                return true;
            }
        }
        return false;
        break;
    case 'l':
        for (int i = 0; i < MAX_DOORS; i++) {
            if (player.pos.x + 1 == room->doors[i].x &&
                player.pos.y == room->doors[i].y &&
                room->doors[i].door_type == DT_LOCKED_P) {
                print_umsg(
                    "ACCESS DENIED! You need a password to unlock this door.");
                unlock_door(map, room, i);
                return true;
            }
        }
        return false;
        break;
    case 'k':
        for (int i = 0; i < MAX_DOORS; i++) {
            // DEBUG(0, 0, "%c", mvinch(player.pos.y + 1, player.pos.x));
            if (player.pos.x == room->doors[i].x &&
                player.pos.y + 1 == room->doors[i].y &&
                room->doors[i].door_type == DT_LOCKED_P) {
                print_umsg(
                    "ACCESS DENIED! You need a password to unlock this door.");
                unlock_door(map, room, i);
                return true;
            }
        }
        return false;
        break;
    case 'h':
        for (int i = 0; i < MAX_DOORS; i++) {
            if (player.pos.x - 1 == room->doors[i].x &&
                player.pos.y == room->doors[i].y &&
                room->doors[i].door_type == DT_LOCKED_P) {
                print_umsg(
                    "ACCESS DENIED! You need a password to unlock this door.");
                unlock_door(map, room, i);
                return true;
            }
        }
        return false;
        break;
    default:
        return false;
        break;
    }
}

bool check_button(Map *map, Room *room) {
    if (player.pos.x == room->button.x && player.pos.y == room->button.y) {
        if (!room->button.is_pressed) {
            room->button.is_pressed = true;
            print_umsg(
                "You've pressed the button! A new password is generated.");
            srand(time(NULL));
            generate_password(&player, map);
        }
    }
}

Point *check_secret_doors(Room *room, bool s_pressed) {
    if (!s_pressed) {
        switch (player.dir) {
        case 'j':
            for (int i = 0; i < MAX_DOORS; i++) {
                if (player.pos.x == room->doors[i].x &&
                    player.pos.y - 1 == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S) {
                    mvaddch(room->doors[i].y, room->doors[i].x, D_SECRET);
                    print_umsg("Well done! You just discovered a secret door! "
                               "Let's see "
                               "what's hiding behind it...");
                    room->doors[i].door_type = DT_FOUND_S;
                    return &room->doors[i];
                }
            }
            break;
        case 'l':
            for (int i = 0; i < MAX_DOORS; i++) {
                if (player.pos.x + 1 == room->doors[i].x &&
                    player.pos.y == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S) {
                    mvaddch(room->doors[i].y, room->doors[i].x, D_SECRET);
                    print_umsg("Well done! You just discovered a secret door! "
                               "Let's see "
                               "what's hiding behind it...");
                    room->doors[i].door_type = DT_FOUND_S;
                    return &room->doors[i];
                }
            }
            break;
        case 'k':
            for (int i = 0; i < MAX_DOORS; i++) {
                if (player.pos.x == room->doors[i].x &&
                    player.pos.y + 1 == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S) {
                    mvaddch(room->doors[i].y, room->doors[i].x, D_SECRET);
                    print_umsg("Well done! You just discovered a secret door! "
                               "Let's see "
                               "what's hiding behind it...");
                    room->doors[i].door_type = DT_FOUND_S;
                    return &room->doors[i];
                }
            }
            break;
        case 'h':
            for (int i = 0; i < MAX_DOORS; i++) {
                if (player.pos.x - 1 == room->doors[i].x &&
                    player.pos.y == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S) {
                    mvaddch(room->doors[i].y, room->doors[i].x, D_SECRET);
                    print_umsg("Well done! You just discovered a secret door! "
                               "Let's see "
                               "what's hiding behind it...");
                    room->doors[i].door_type = DT_FOUND_S;
                    return &room->doors[i];
                }
            }
            break;
        default:
            break;
        }
    } else {
        for (int i = 0; i < MAX_DOORS; i++) {
            if (player.pos.x == room->doors[i].x &&
                    player.pos.y - 1 == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S ||
                player.pos.x + 1 == room->doors[i].x &&
                    player.pos.y == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S ||
                player.pos.x == room->doors[i].x &&
                    player.pos.y + 1 == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S ||
                player.pos.x - 1 == room->doors[i].x &&
                    player.pos.y == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S ||
                player.pos.x - 1 == room->doors[i].x &&
                    player.pos.y - 1 == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S ||
                player.pos.x + 1 == room->doors[i].x &&
                    player.pos.y + 1 == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S ||
                player.pos.x - 1 == room->doors[i].x &&
                    player.pos.y + 1 == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S ||
                player.pos.x + 1 == room->doors[i].x &&
                    player.pos.y - 1 == room->doors[i].y &&
                    room->doors[i].door_type == DT_SECRET_S) {
                mvaddch(room->doors[i].y, room->doors[i].x, D_SECRET);
                print_umsg(
                    "Well done! You just discovered a secret door! Let's see "
                    "what's hiding behind it...");
                room->doors[i].door_type = DT_FOUND_S;
                return &room->doors[i];
            }
        }
    }
    return NULL;
}

bool check_items(Room *room) {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (player.pos.y == room->items[i].pos.y &&
            player.pos.x == room->items[i].pos.x && !room->items[i].is_taken) {
            Item *item = &room->items[i];
            switch (item->type) {
            case IT_GOLD:
                print_umsg("Jackpot! %d shiny gold coins are now yours!",
                           item->amount);
                player.items[player.item_count++] = *item;
                player.gold_count += item->amount;
                item->is_taken = true;
                break;
            case IT_BGOLD:
                print_umsg("Lucky! %d shiny Black gold coins are now yours!",
                           item->amount);
                player.items[player.item_count++] = *item;
                player.gold_count += item->amount;
                item->is_taken = true;
                break;
            case IT_SIMPLE_FOOD:
                if (player.food_count == 5) {
                    print_umsg("Sorry! You have already 5 units of food.");
                    break;
                }
                print_umsg("Yum! You found a delicious meal! Enjoy it!");
                player.food_count++;
                player.items[player.item_count++] = *item;
                item->is_taken = true;
                break;
            case IT_HEALTH_SPELL:
                print_umsg("Health Spell found! You feel renewed.");
                player.items[player.item_count++] = *item;
                player.spell_count++;
                item->is_taken = true;
                break;
            case IT_SPEED_SPELL:
                print_umsg("Speed Spell found! You're faster now.");
                player.items[player.item_count++] = *item;
                player.spell_count++;
                item->is_taken = true;
                break;
            case IT_DAMAGE_SPELL:
                print_umsg("Damage Spell found! Increased attack power.");
                player.items[player.item_count++] = *item;
                player.spell_count++;
                item->is_taken = true;
                break;
            case IT_ANCIENT_KEY:
                print_umsg(
                    "You found an Ancient Key! A mysterious energy surrounds "
                    "it... Maybe it can unlock a password door!");
                item->is_taken = true;
                item->is_broken = false;
                player.items[player.item_count++] = *item;
                player.ancient_key_count++;
                break;
            }
        }
    }
}

bool check_weapons(Room *room) {
    for (int i = 0; i < MAX_WEAPONS; i++) {
        if (player.pos.y == room->weapons[i].pos.y &&
            player.pos.x == room->weapons[i].pos.x &&
            !room->weapons[i].is_taken) {
            Weapon *weapon = &room->weapons[i];
            switch (weapon->type) {
            case WT_MACE:
                if (weapon_count(WT_MACE) > 0) {
                    print_umsg("You already have a Mace.");
                    break;
                }
                print_umsg("You found a Mace. Ready your arms for battle!");
                player.weapons[player.weapon_count++] = *weapon;
                weapon->is_taken = true;
                break;
            case WT_DAGGER:
                print_umsg("You found a Dagger. Ready your arms for battle!");
                if (!weapon->is_used) {
                    for (int j = 0; j < W_DAGGER_CA; j++) {
                        player.weapons[player.weapon_count++] = *weapon;
                        weapon->is_taken = true;
                    }
                } else {
                    player.weapons[player.weapon_count++] = *weapon;
                    weapon->is_taken = true;
                    weapon->is_used = false;
                }
                break;
            case WT_MAGIC_WAND:
                print_umsg(
                    "You found a Magic Wand. Ready your arms for battle!");
                if (!weapon->is_used) {
                    for (int j = 0; j < W_MAGIC_WAND_CA; j++) {
                        player.weapons[player.weapon_count++] = *weapon;
                        weapon->is_taken = true;
                    }
                } else {
                    player.weapons[player.weapon_count++] = *weapon;
                    weapon->is_taken = true;
                    weapon->is_used = false;
                }
                break;
            case WT_NORMAL_ARROW:
                print_umsg(
                    "You found a Normal Arrow. Ready your arms for battle!");
                if (!weapon->is_used) {
                    for (int j = 0; j < W_NORMAL_ARROW_CA; j++) {
                        player.weapons[player.weapon_count++] = *weapon;
                        weapon->is_taken = true;
                    }
                } else {
                    player.weapons[player.weapon_count++] = *weapon;
                    weapon->is_taken = true;
                    weapon->is_used = false;
                }
                break;
            case WT_SWORD:
                if (weapon_count(WT_SWORD) > 0) {
                    print_umsg("You already have a Sword.");
                    break;
                }
                print_umsg("You found a Sword. Ready your arms for battle!");
                player.weapons[player.weapon_count++] = *weapon;
                weapon->is_taken = true;
                break;
            }
        }
    }
}

bool check_staircase(Map *map, Room *room) {
    int x = player.pos.x, y = player.pos.y;
    if (x == room->staircase.x && y == room->staircase.y &&
        player.curr_floor == 0) {
        create_map(&maps[++player.curr_floor]);
        spawn_player(&maps[player.curr_floor].rooms[player.curr_area], 1);
        maps[player.curr_floor].rooms[player.curr_area].is_reveald = true;
        redraw_screen(&maps[player.curr_floor]);
        return true;
    }
    return false;
}

bool use_ancient_key() {
    int count = player.ancient_key_count;

    if (count > 0) {
        int bk_index[player.ancient_key_count + 1];
        int broken_keys_count = 0;
        for (int i = 0; i < count; i++) {
            if (player.items[i].type == IT_ANCIENT_KEY &&
                player.items[i].is_broken && !player.items[i].is_used) {
                bk_index[broken_keys_count++] = i;
            }
            if (player.items[i].type == IT_ANCIENT_KEY &&
                !player.items[i].is_broken && !player.items[i].is_used) {
                srand(time(NULL));
                int broke_probabality = rand() % 10;
                if (broke_probabality == 0) {
                    player.items[i].is_broken = true;
                    print_lmsg(RED, "Sorry Buddy! Your key broke!");
                    return false;
                }
                player.items[i].is_used = true;
                return true;
            }
        }
        if (broken_keys_count >= 2) {
            player.items[bk_index[0]].is_used = true;
            player.items[bk_index[1]].is_used = true;
            return true;
        } else if (broken_keys_count == 1) {
            print_lmsg(RED, "Sorry! There is just one broken key!");
            return false;
        }
    }
    print_lmsg(RED, "Sorry Dude! There is no Ancient Key!");
    return false;
}

bool unlock_door(Map *map, Room *room, int index) {
    WINDOW *pwin = newwin(10, 50, LINES / 2 - 5, COLS / 2 - 25);
    keypad(pwin, TRUE);
    box(pwin, 0, 0);
    char input_password[PASSWORD_SIZE];
    attron(A_ITALIC);
    mvwprintw(pwin, 3, 2, "Please enter the password to unlock this door:");
    mvwprintw(pwin, 4, 10, "(Enter 'K' to use Ancient Key)");
    attroff(A_ITALIC);
    refresh();
    int count = 0;
    while (true) {
        int ch;
        timeout(-1);
        wmove(pwin, 7, 23);
        bool check = false, key = false;
        int input_count = 0;
        while (true) {
            ch = wgetch(pwin);
            if (ch != ERR) {
                if (ch == 'K') {
                    check = true;
                    key = use_ancient_key();
                    if (key == false) {
                        return false;
                    }
                    break;
                } else if (ch == '\n') {
                    if (input_count < 4)
                        continue;
                    else
                        break;
                } else if (ch == 27) {
                    input_password[0] = '\0';
                    input_password[1] = '\0';
                    input_password[2] = '\0';
                    input_password[3] = '\0';
                    input_password[4] = '\0';
                    curs_set(0);
                    return false;
                } else if (ch == KEY_BACKSPACE) {
                    int y, x;
                    getyx(pwin, y, x);
                    if (x > 23) {
                        curs_set(0);
                        wmove(pwin, y, x - 1);
                        waddch(pwin, ' ');
                        wmove(pwin, y, x - 1);
                        curs_set(1);
                        input_password[--input_count] = '\0';
                    }
                } else {
                    if (input_count < 4) {
                        waddch(pwin, ch);
                        input_password[input_count++] = ch;
                        // if (input_count == 3)
                        // {
                        //     break;
                        // }
                    } else {
                        continue;
                    }
                }
            }
        }
        curs_set(0);
        count++;
        if (!strcmp(map->rooms[player.curr_area].password, input_password) ||
            key) {
            wclear(pwin);
            delwin(pwin);
            redraw_screen(map);
            room->doors[index].door_type = DT_OPENED_P;
            redraw_screen(map);
            print_umsg("Success! The door unlocks and creaks open... What lies "
                       "ahead?");
            return true;
        } else {
            switch (count) {
            case 1:
                print_lmsg(
                    1, "Oops! The password is incorrect. Please try again.");
                wmove(pwin, 7, 23);
                wclrtoeol(pwin);
                box(pwin, 0, 0);
                break;
            case 2:
                print_lmsg(6,
                           "Access denied! Wrong password again. Be careful!");
                wmove(pwin, 7, 23);
                wclrtoeol(pwin);
                box(pwin, 0, 0);
                break;
            default:
                return false;
                break;
            }
        }
    }
}

void reverse_password(char str1[], char str2[]) {
    for (int i = 0; i < 4; i++) {
        str1[i] = str2[3 - i];
    }
}

void *generate_password_thread(void *arg) {
    GamePthread *game = (GamePthread *)arg;
    char *password = game->map->rooms[player.curr_area].password;
    snprintf(password, sizeof(password) + 1, "%04d", rand() % 10000);
    time_t start_time = time(NULL);
    char show_password[5] = {0};
    srand(time(NULL));
    int reverse_probabality = rand() % 4;
    if (reverse_probabality == 0) {
        reverse_password(show_password, password);
    } else {
        strcpy(show_password, password);
    }
    while (time(NULL) - start_time <= 30) {
        usleep(100000);
        mvprintw(UMSG_Y, COLS - 20, "Password: %s", show_password);
        refresh();
    }
    move(UMSG_Y, COLS - 20);
    clrtoeol();
    refresh();
    game->map->rooms[player.curr_area].button.is_pressed = false;
    return NULL;
}

void generate_password(Player *player, Map *map) {
    pthread_t password_thread;
    GamePthread *game = malloc(sizeof(GamePthread));
    game->map = map;
    game->player = player;
    pthread_create(&password_thread, NULL, generate_password_thread,
                   (void *)game);
    pthread_detach(password_thread);
}

void M_mode_draw(Map *map) {
    clear();
    for (int i = 0; i < MAX_ROOMS; i++) {
        draw_player();
        draw_room(&map->rooms[i], 1);
        draw_corridor(map->corridors[i], 0);
        mvprintw(0, 0, "  ");
        mvprintw(1, 0, "  ");
        refresh();
    }
    refresh();
    timeout(-1);
    while (true) {
        int chh = getch();
        if (chh != ERR) {
            if (chh == 'M') {
                redraw_screen(map);
                return;
            } else {
                continue;
            }
        }
    }
}

WINDOW *list_window(const char *title) {
    int size = strlen(title);
    WINDOW *lwin = newwin(LIST_HEIGHT, LIST_WIDTH, LIST_Y, LIST_X);
    box(lwin, 0, 0);
    mvwprintw(lwin, LIST_MSG_Y, LIST_WIDTH / 2 - size / 2, title);
    wrefresh(lwin);
    return lwin;
}

void food_list() {
    WINDOW *lwin = list_window("Food List");
    while (true) {
        int simple_food_count = 0, use_index = -1;
        for (int i = 0; i < player.item_count; i++) {
            if (player.items[i].type == IT_SIMPLE_FOOD &&
                !player.items[i].is_used) {
                use_index = i;
                simple_food_count++;
            }
        }
        mvwprintw(lwin, LIST_FI_Y, LIST_FI_X, "1. Simple Food (%d)",
                  simple_food_count);
        wrefresh(lwin);
        timeout(-1);
        int ch = wgetch(lwin);
        if (ch != ERR) {
            if (ch == '1') {
                if (player.food_count >= 0) {
                    player.food_count--;
                }
                if (use_index != -1) {
                    player.items[use_index].is_used = true;
                }
                if (player.food_count >= 0) {
                    if (player.feed <= FULL_FEED - 5) {
                        player.feed += 10;
                    } else {
                        player.feed = FULL_FEED;
                    }
                    if (player.HP <= FULL_HP - 5) {
                        player.HP += 10;
                    } else {
                        player.HP = FULL_HP;
                    }
                }
            } else if (ch == 27) {
                timeout(0);
                delwin(lwin);
                return;
            }
        }
    }
}

int spell_list() {
    WINDOW *lwin = list_window("Spell List");

    int result;
    while (true) {
        int health_spell_count = 0, speed_spell_count = 0,
            damage_spell_count = 0;
        int use_index1 = -1, use_index2 = -1, use_index3 = -1;
        for (int i = 0; i < player.item_count; i++) {
            if (player.items[i].type == IT_HEALTH_SPELL &&
                !player.items[i].is_used) {
                use_index1 = i;
                health_spell_count++;
            }
            if (player.items[i].type == IT_SPEED_SPELL &&
                !player.items[i].is_used) {
                use_index2 = i;
                speed_spell_count++;
            }
            if (player.items[i].type == IT_DAMAGE_SPELL &&
                !player.items[i].is_used) {
                use_index3 = i;
                damage_spell_count++;
            }
        }
        mvwprintw(lwin, LIST_FI_Y, LIST_FI_X, "1. Health Spell (%d)",
                  health_spell_count);
        mvwprintw(lwin, LIST_FI_Y + 1, LIST_FI_X, "2. Speed Spell (%d)",
                  speed_spell_count);
        mvwprintw(lwin, LIST_FI_Y + 2, LIST_FI_X, "3. Damage Spell (%d)",
                  damage_spell_count);
        wrefresh(lwin);
        timeout(-1);
        int ch = wgetch(lwin);
        if (ch != ERR) {
            if (ch == '1') {
                if (player.spell_count > 0) {
                    player.spell_count--;
                }
                if (use_index1 != -1) {
                    player.items[use_index1].is_used = true;
                    result = IT_HEALTH_SPELL;
                    break;
                }
            }
            if (ch == '2') {
                if (player.spell_count != 0) {
                    player.spell_count--;
                }
                if (use_index2 != -1) {
                    player.items[use_index2].is_used = true;
                    result = IT_SPEED_SPELL;
                    break;
                }
            }
            if (ch == '3') {
                if (player.spell_count != 0) {
                    player.spell_count--;
                }
                if (use_index3 != -1) {
                    player.items[use_index3].is_used = true;
                    result = IT_DAMAGE_SPELL;
                    break;
                }
            } else if (ch == 27) {
                result = -1;
                break;
            }
        }
    }
    timeout(0);
    delwin(lwin);
    refresh();
    return result;
}

void weapon_list() {
    WINDOW *lwin = list_window("Weapon List");
    while (true) {
        int count[5] = {0};
        int use_index[5] = {-1, -1, -1, -1, -1};
        for (int i = 0; i < player.weapon_count; i++) {
            if (player.weapons[i].type == NO_WEAPON) {
                continue;
            }
            if (player.weapons[i].type == WT_MACE) {
                use_index[WT_MACE] = i;
                count[WT_MACE]++;
            } else if (player.weapons[i].type == WT_DAGGER) {
                use_index[WT_DAGGER] = i;
                count[WT_DAGGER]++;
            } else if (player.weapons[i].type == WT_MAGIC_WAND) {
                use_index[WT_MAGIC_WAND] = i;
                count[WT_MAGIC_WAND]++;
            } else if (player.weapons[i].type == WT_NORMAL_ARROW) {
                use_index[WT_NORMAL_ARROW] = i;
                count[WT_NORMAL_ARROW]++;
            } else if (player.weapons[i].type == WT_SWORD) {
                use_index[WT_SWORD] = i;
                count[WT_SWORD]++;
            }
        }
        mvwprintw(lwin, LIST_FI_Y, LIST_FI_X, "short-range weapons:");
        mvwprintw(lwin, LIST_FI_Y + 1, LIST_FI_X, "1. Mace");
        if (count[WT_MACE] > 0) {
            waddwstr(lwin, L"(✓)");
        } else {
            waddwstr(lwin, L"(×)");
        }
        if (player.active_weapon.type == WT_MACE) {
            wprintw(lwin, " *");
        }
        mvwprintw(lwin, LIST_FI_Y + 2, LIST_FI_X, "2. Sword");
        if (count[WT_SWORD] > 0) {
            waddwstr(lwin, L"(✓)");
        } else {
            waddwstr(lwin, L"(×)");
        }
        if (player.active_weapon.type == WT_SWORD) {
            wprintw(lwin, " *");
        }
        mvwprintw(lwin, LIST_FI_Y + 4, LIST_FI_X, "long-range weapons:");
        mvwprintw(lwin, LIST_FI_Y + 5, LIST_FI_X, "3. Dagger (%d)",
                  count[WT_DAGGER]);
        if (player.active_weapon.type == WT_DAGGER) {
            wprintw(lwin, " *");
        }
        mvwprintw(lwin, LIST_FI_Y + 6, LIST_FI_X, "4. Magic Wand (%d)",
                  count[WT_MAGIC_WAND]);
        if (player.active_weapon.type == WT_MAGIC_WAND) {
            wprintw(lwin, " *");
        }
        mvwprintw(lwin, LIST_FI_Y + 7, LIST_FI_X, "5. Normal Arrow (%d)",
                  count[WT_NORMAL_ARROW]);
        if (player.active_weapon.type == WT_NORMAL_ARROW) {
            wprintw(lwin, " *");
        }
        wrefresh(lwin);
        timeout(-1);
        int ch = wgetch(lwin);
        if (ch != ERR) {
            if (ch == '1') {
                if (player.active_weapon.type == NO_WEAPON &&
                    count[WT_MACE] > 0) {
                    player.active_weapon.type = WT_MACE;
                    print_umsg("Active weapon changed to Mace.");
                    return;
                } else if (player.active_weapon.type == NO_WEAPON &&
                           count[WT_MACE] == 0) {
                    print_lmsg(RED, "You don't have this weapon!");
                } else if (player.active_weapon.type != NO_WEAPON &&
                           player.active_weapon.type != WT_MACE) {
                    print_lmsg(ORANGE, "Put your weapon on backpack first.");
                    return;
                }
            }
            if (ch == '2') {
                if (player.active_weapon.type == NO_WEAPON &&
                    count[WT_SWORD] > 0) {
                    player.active_weapon.type = WT_SWORD;
                    print_umsg("Active weapon changed to Sword.");
                    return;
                } else if (player.active_weapon.type == NO_WEAPON &&
                           count[WT_SWORD] == 0) {
                    print_lmsg(RED, "You don't have this weapon!");
                } else if (player.active_weapon.type != NO_WEAPON &&
                           player.active_weapon.type != WT_SWORD) {
                    print_lmsg(ORANGE, "Put your weapon on backpack first.");
                    return;
                }
            }
            if (ch == '3') {
                if (player.active_weapon.type == NO_WEAPON &&
                    count[WT_DAGGER] > 0) {
                    player.active_weapon.type = WT_DAGGER;
                    print_umsg("Active weapon changed to Dagger.");
                    return;
                } else if (player.active_weapon.type == NO_WEAPON &&
                           count[WT_DAGGER] == 0) {
                    print_lmsg(RED, "You don't have this weapon!");
                } else if (player.active_weapon.type != NO_WEAPON &&
                           player.active_weapon.type != WT_DAGGER) {
                    print_lmsg(ORANGE, "Put your weapon on backpack first.");
                    return;
                }
            }
            if (ch == '4') {
                if (player.active_weapon.type == NO_WEAPON &&
                    count[WT_MAGIC_WAND] > 0) {
                    player.active_weapon.type = WT_MAGIC_WAND;
                    print_umsg("Active weapon changed to Magic Wand.");
                    return;
                } else if (player.active_weapon.type == NO_WEAPON &&
                           count[WT_MAGIC_WAND] == 0) {
                    print_lmsg(RED, "You don't have this weapon!");
                } else if (player.active_weapon.type != NO_WEAPON &&
                           player.active_weapon.type != WT_MAGIC_WAND) {
                    print_lmsg(ORANGE, "Put your weapon on backpack first.");
                    return;
                }
            }
            if (ch == '5') {
                if (player.active_weapon.type == NO_WEAPON &&
                    count[WT_NORMAL_ARROW] > 0) {
                    player.active_weapon.type = WT_NORMAL_ARROW;
                    print_umsg("Active weapon changed to Normal Arrow.");
                    return;
                } else if (player.active_weapon.type == NO_WEAPON &&
                           count[WT_NORMAL_ARROW] == 0) {
                    print_lmsg(RED, "You don't have this weapon!");
                } else if (player.active_weapon.type != NO_WEAPON &&
                           player.active_weapon.type != WT_NORMAL_ARROW) {
                    print_lmsg(ORANGE, "Put your weapon on backpack first.");
                    return;
                }
            } else if (ch == 27) {
                timeout(0);
                delwin(lwin);
                return;
            } else {
                continue;
            }
        }
    }
}

int block_index(Corridor corridor) {
    int x = player.pos.x, y = player.pos.y;
    int count = corridor.block_count;
    for (int j = 0; j < count; j++) {
        if (x == corridor.blocks[j].x && y == corridor.blocks[j].y) {
            return j;
        }
    }
    return -1;
}

void reveal_blocks(Corridor *corridor, int index) {
    if (index >= 0) {
        for (int j = 0; j < 5; j++) {
            if (index - j >= 0 && index - j < corridor->block_count ||
                index + j >= 0 && index + j < corridor->block_count) {
                corridor->blocks[index - j].is_reveald = true;
                corridor->blocks[index + j].is_reveald = true;
            }
        }
    }
}

void check_position(Map *map) {
    int x = player.pos.x, y = player.pos.y;
    for (int i = 0; i < MAX_CORRS; i++) {
        int index = block_index(map->corridors[i]);
        // debug_window(0, 0, "1", 1, index);
        if (index >= 0) {
            if (index == 1) {
                map->rooms[i + 1].is_reveald = true;
            } else if (index == 0) {
                player.curr_area = i + 1;
            } else if (index == map->corridors[i].block_count - 2) {
                map->rooms[i].is_reveald = true;
            } else if (index == map->corridors[i].block_count - 1) {
                player.curr_area = i;
            }

            map->corridors[i].is_reveald = true;
            reveal_blocks(&map->corridors[i], index);
            return;
        }
    }
}

void redraw_map(Map *map) {
    clear();
    for (int i = 0; i < MAX_ROOMS; i++) {
        draw_room(&map->rooms[i], 0);
        draw_corridor(map->corridors[i], 0);
    }
    refresh();
}

void redraw_screen(Map *map) {
    clear();
    if (M_mode == false) {
        int count = map->room_count;
        for (int i = 0; i < MAX_CORRS; i++) {
            if (map->rooms[i].is_reveald) {
                if ((i >= 0 && i < map->room_count) &&
                    (i == player.curr_area || i - 1 == player.curr_area ||
                     i + 1 == player.curr_area)) {
                    draw_room(&map->rooms[i], 1);
                }
            }
            if (map->corridors[i].is_reveald) {
                if ((i >= 0 && i < map->room_count - 1) &&
                    (i == player.curr_area || i == player.curr_area - 1)) {
                    draw_corridor(map->corridors[i], 1);
                }
            }
        }
    } else {
        for (int i = 0; i < MAX_CORRS; i++) {
            draw_room(&map->rooms[i], 1);
            draw_corridor(map->corridors[i], 0);
        }
    }
    mvprintw(0, 0, "  ");
    mvprintw(1, 0, "  ");
    draw_player();
}

void draw_player() {
    // draw player's character
    mvprintw(0, 0, " ");
    mvaddch(player.pos.y, player.pos.x, G_PLAYER);

    // print user's name
    mvprintw(LINES - 1, COLS - 10, "%s", logged_in_user.username);

    // print current weapon
    switch (player.active_weapon.type) {
    case WT_MACE:
        mvaddch(LINES - 1, COLS - 1, W_MACE);
        break;
    case WT_DAGGER:
        mvaddch(LINES - 1, COLS - 1, W_DAGGER);
        break;
    case WT_MAGIC_WAND:
        mvaddch(LINES - 1, COLS - 1, W_MAGIC_WAND);
        break;
    case WT_NORMAL_ARROW:
        mvaddch(LINES - 1, COLS - 1, W_NORMAL_ARROW);
        break;
    case WT_SWORD:
        mvaddch(LINES - 1, COLS - 1, W_SWORD);
        break;
    default:
        break;
    };

    // draw player's HP
    attron(COLOR_PAIR(BLUE));
    attron(A_ITALIC);
    mvprintw(HP_MSG_Y, HP_MSG_X, "HP(%3d): ", player.HP);
    attroff(A_ITALIC);
    attroff(COLOR_PAIR(BLUE));
    for (int i = 0; i < player.HP; i += 10) {
        addwstr(G_HP_UNIT);
    }

    // draw player's feed
    attron(COLOR_PAIR(ORANGE));
    attron(A_ITALIC);
    mvprintw(HP_MSG_Y, HP_MSG_X + 35, "FEED(%3d): ", player.feed);
    attroff(A_ITALIC);
    attroff(COLOR_PAIR(ORANGE));
    for (int i = 0; i < player.feed; i += 10) {
        addwstr(G_FEED_UNIT);
    }

    // draw player's gold
    attron(COLOR_PAIR(GOLD));
    attron(A_ITALIC);
    mvprintw(HP_MSG_Y, HP_MSG_X + 70, "GOLD(%3d): ", player.gold_count);
    attroff(A_ITALIC);
    attroff(COLOR_PAIR(GOLD));
    for (int i = 0; i < player.gold_count; i += 10) {
        addwstr(G_GOLD_UNIT);
    }

    refresh();
}

void print_umsg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    move(UMSG_Y, UMSG_X);
    call_colors();
    attron(A_ITALIC);
    attron(A_BOLD);
    attron(COLOR_PAIR(BLUE));
    vw_printw(stdscr, format, args);
    va_end(args);
    attroff(A_ITALIC);
    attroff(A_BOLD);
    attroff(COLOR_PAIR(BLUE));
    refresh();
    timeout(-1);
    keypad(stdscr, TRUE);
    while (true) {
        int ch = getch();
        if (ch != ERR) {
            if (ch == 27) // esc button
            {
                move(UMSG_Y, UMSG_X);
                clrtoeol();
                break;
            }
        }
    }
}

void debug_window(int y, int x, const char *title, int count, ...) {
    int height = 10;
    int width = 50;
    WINDOW *debug_win = newwin(height, width, y, x);
    box(debug_win, 0, 0);

    mvwprintw(debug_win, 0, 2, "[ %s ]", title);

    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        int value = va_arg(args, int);
        mvwprintw(debug_win, i + 1, 2, "Value %d: %d", i + 1, value);
    }
    va_end(args);

    wrefresh(debug_win);

    mvwprintw(debug_win, height - 2, 2, "Press any key to close...");
    wrefresh(debug_win);
    timeout(-1);
    getch();

    delwin(debug_win);
    refresh();
}

bool save_game_menu(Map *map) {
    WINDOW *swin = list_window("Save Game");
    char save_title[MAX_TITLE_LEN];
    noecho();
    keypad(swin, TRUE);
    mvwprintw(swin, LIST_FI_Y + 1, LIST_FI_X, "Enter your save title:");
    wrefresh(swin);
    int input_count = 0;
    wmove(swin, LIST_FI_Y + 3, LIST_FI_X);
    while (true) {
        if (input_count >= 20) {
            return true;
        }
        int ch = wgetch(swin);
        if (ch != ERR) {
            if (ch == 27) {
                delwin(swin);
                return false;
            } else if (ch == '\n') {
                break;
            } else if (ch == KEY_BACKSPACE) {
                int y1, x1;
                getyx(swin, y1, x1);
                curs_set(0);
                if (x1 > LIST_FI_X) {
                    wmove(swin, y1, x1 - 1);
                    waddch(swin, ' ');
                    wmove(swin, y1, x1 - 1);
                    curs_set(1);
                    save_title[--input_count] = '\0';
                }
            } else {
                waddch(swin, ch);
                save_title[input_count++] = ch;
            }
        }
    }
    GameSave game;
    game.maps[0] = maps[0];
    game.maps[1] = maps[1];
    game.player = player;
    strcpy(game.save_title, save_title);
    save_game(&game);
    return true;
}

void save_game(GameSave *gameSave) {
    FILE *file = fopen("gameinfo.bin", "ab");
    if (!file) {
        return;
    }

    fwrite(gameSave, sizeof(GameSave), 1, file);
    fclose(file);
    return;
}

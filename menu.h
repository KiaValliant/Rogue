#include <stdbool.h>
#include "game.h"
#ifndef MENU_H
#define MENU_H
#define MAX_LEN 100
#define MAX_SAVES 2

typedef struct
{
    char username[MAX_LEN];
    char password[MAX_LEN];
    char email[MAX_LEN];
    GameSave game_save;
} UserInfo;

void main_menu();
void sign_up_menu();
void sign_in_menu();
void pregame_menu();
void login_menu();
void scoreboard_menu();
void settings_menu();
void draw_base_form(int color);
bool get_input(int y, int x, char *str);
void print_main_msgs();
void print_sign_up_msgs();
void print_sign_in_msgs();
void print_pregame_msgs();
void print_login_msgs();
void handle_sign_up_input();
void handle_login_input();
bool is_username_valid();
bool is_password_valid();
bool is_password_correct();
bool is_email_valid();
void print_lmsg(int, const char *);
void draw_margin();
void draw_rogue();
bool is_username_used();
void save_userinfo();

#endif
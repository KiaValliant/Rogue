#include "menu.h"
#include "game.h"
#include "mainh.h"
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define USERNAME_FIELD_Y LINES / 2 - 2
#define PASSWORD_FIELD_Y LINES / 2 + 1
#define EMAIL_FIELD_Y LINES / 2 + 4
#define ALL_FIELD_X COLS / 2 - 14
#define ERROR_FIELD_Y LINES - 3
#define ERROR_FIELD_X COLS / 2 - 9

UserInfo user_info;
UserInfo logged_in_user;

void main_menu() {
    initscr();
    clear();
    curs_set(0);
    keypad(stdscr, TRUE);
    draw_base_form(CYAN);
    print_main_msgs(CYAN);
    noecho();
    timeout(-1);
    while (true) {
        int ch = getch();
        echo();
        if (ch != ERR) {
            if (ch == '1') {
                clear();
                sign_up_menu();
                curs_set(0);
                draw_base_form(CYAN);
                print_main_msgs(CYAN);
            } else if (ch == '2') {
                clear();
                sign_in_menu();
                curs_set(0);
                draw_base_form(CYAN);
                print_main_msgs(CYAN);
            } else if (ch == '3') {
                clear();
                pregame_menu();
                curs_set(0);
                draw_base_form(CYAN);
                print_main_msgs(CYAN);
            }
        }
    }
}

void sign_in_menu() {
    initscr();
    curs_set(0);
    draw_base_form(RED);
    print_sign_in_msgs(RED);
    noecho();
    timeout(-1);
    int ch = getch();
    echo();
    if (ch != ERR) {
        if (ch == '1') {
            clear();
            login_menu();
        } else if (ch == '2') {
            clear();
            memset(&logged_in_user, 0, sizeof(UserInfo));
            main_menu();
        }
    }
    timeout(-1);
    getch();
    clear();
    endwin();
}

void sign_up_menu() {
    initscr();
    curs_set(1);
    draw_base_form(YELLOW);
    print_sign_up_msgs(YELLOW);
    timeout(-1);
    handle_sign_up_input();
    clear();
    endwin();
    return;
}

void pregame_menu() {
    initscr();
    curs_set(1);
    draw_base_form(MAGENTA);
    print_pregame_msgs(MAGENTA);
    timeout(-1);
    timeout(-1);
    int ch = getch();
    echo();
    if (ch != ERR) {
        if (ch == '1') {
            clear();
            start_game();
        } else if (ch == '2') {
            clear();
            resume_game();
        } else if (ch == '3') {
            clear();
            scoreboard_menu();
        } else if (ch == '4') {
            clear();
            settings_menu();
        }
    }
}

void login_menu() {
    initscr();
    curs_set(1);
    draw_base_form(MAGENTA);
    print_login_msgs(MAGENTA);
    handle_login_input();
}

void scoreboard_menu() {}

void settings_menu() {}

void print_login_msgs(int color) {
    call_colors();
    attron(COLOR_PAIR(color));
    attron(A_BOLD);
    mvprintw(9, COLS / 2 - 3, "LOGIN");
    mvprintw(USERNAME_FIELD_Y - 1, ALL_FIELD_X, "Please enter your username:");
    mvprintw(PASSWORD_FIELD_Y - 1, ALL_FIELD_X, "Please enter your password:");
    attroff(COLOR_PAIR(color));
    attroff(A_BOLD);
    refresh();
    move(USERNAME_FIELD_Y, ALL_FIELD_X);
}

bool is_username_used() {
    FILE *file = fopen("userinfo.bin", "rb");
    if (file == NULL) {
        return false;
    }

    UserInfo temp;
    while (fread(&temp, sizeof(UserInfo), 1, file) == 1) {
        if (!strcmp(user_info.username, temp.username)) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

void save_userinfo() {
    FILE *file = fopen("userinfo.bin", "ab");
    if (file == NULL) {
        return;
    }

    if (fwrite(&user_info, sizeof(UserInfo), 1, file) != 1) {
        fclose(file);
        return;
    }
    fclose(file);
}

void draw_base_form(int color) {
    call_colors();
    attron(COLOR_PAIR(color));
    attron(A_BOLD);
    draw_rogue();
    attron(A_BLINK);
    draw_margin();
    attroff(A_BLINK);
    attroff(COLOR_PAIR(color));
    attroff(A_BOLD);
    refresh();
}

void draw_margin() {
    for (int i = 1; i < LINES; i++) {
        mvprintw(i, 0, "|");
        mvprintw(i, COLS - 1, "|");
    }
    for (int i = 1; i < COLS - 1; i++) {
        mvprintw(0, i, "_");
        mvprintw(LINES - 1, i, "_");
    }
}

void draw_rogue() {
    mvprintw(1, COLS / 2 - 17, "    ____  ____  ________  ________");
    mvprintw(2, COLS / 2 - 17, "   / __ \\/ __ \\/ ____/ / / / ____/");
    mvprintw(3, COLS / 2 - 17, "  / /_/ / / / / / __/ / / / __/   ");
    mvprintw(4, COLS / 2 - 17, " / _, _/ /_/ / /_/ / /_/ / /___   ");
    mvprintw(5, COLS / 2 - 17, "/_/ |_|\\____/\\____/\\____/_____/   ");
}

void print_sign_up_msgs(int color) {
    call_colors();
    attron(COLOR_PAIR(color));
    attron(A_BOLD);
    mvprintw(9, COLS / 2 - 4, "SIGN UP");
    mvprintw(USERNAME_FIELD_Y - 1, ALL_FIELD_X, "Please enter your username:");
    mvprintw(PASSWORD_FIELD_Y - 1, ALL_FIELD_X, "Please enter your password:");
    mvprintw(EMAIL_FIELD_Y - 1, ALL_FIELD_X, "Please enter your email:");
    attroff(COLOR_PAIR(color));
    attroff(A_BOLD);
    refresh();
    move(USERNAME_FIELD_Y, ALL_FIELD_X);
}

void print_main_msgs(int color) {
    call_colors();
    attron(COLOR_PAIR(color));
    attron(A_BOLD);
    mvprintw(4, 4, "%s", logged_in_user.username);
    mvprintw(9, COLS / 2 - 4, "MAIN MENU");
    mvprintw(LINES / 2 - 2, COLS / 2 - 5, "1. SIGN UP");
    mvprintw(LINES / 2, COLS / 2 - 5, "2. SIGN IN");
    mvprintw(LINES / 2 + 2, COLS / 2 - 5, "3. PRE-GAME");
    attroff(COLOR_PAIR(color));
    attroff(A_BOLD);
    refresh();
}

void print_pregame_msgs(int color) {
    call_colors();
    attron(COLOR_PAIR(color));
    attron(A_BOLD);
    mvprintw(9, COLS / 2 - 4, "START GAME");
    mvprintw(LINES / 2 - 3, COLS / 2 - 5, "1. New Game");
    mvprintw(LINES / 2 - 1, COLS / 2 - 5, "2. Resume Game");
    mvprintw(LINES / 2 + 1, COLS / 2 - 5, "3. Scoreboard");
    mvprintw(LINES / 2 + 3, COLS / 2 - 5, "4. Settings");
    attroff(COLOR_PAIR(color));
    attroff(A_BOLD);
    refresh();
}

void print_sign_in_msgs(int color) {
    call_colors();
    attron(COLOR_PAIR(color));
    attron(A_BOLD);
    mvprintw(9, COLS / 2 - 4, "SIGN UP");
    mvprintw(LINES / 2 - 1, COLS / 2 - 11, "1. Login with Username");
    mvprintw(LINES / 2 + 1, COLS / 2 - 11, "2. Continue as Guest");
    attroff(COLOR_PAIR(color));
    attroff(A_BOLD);
    refresh();
}

bool get_input(int y, int x, char *str) {
    int size = strlen(str);
    int ch;
    timeout(-1);
    move(y, x);
    curs_set(1);
    int input_count = 0;
    noecho();
    while (true) {
        if (input_count >= 20) {
            return true;
        }
        ch = getch();
        if (ch != ERR) {
            if (ch == 27) {
                for (int i = 0; i < 100; i++) {
                    str[i] = 0;
                }
                return false;
            } else if (ch == '\n') {
                return true;
            } else if (ch == KEY_BACKSPACE) {
                int y1, x1;
                getyx(stdscr, y1, x1);
                curs_set(0);
                if (x1 > x) {
                    move(y1, x1 - 1);
                    addch(' ');
                    move(y1, x1 - 1);
                    curs_set(1);
                    str[--input_count] = '\0';
                }
            } else {
                addch(ch);
                str[input_count++] = ch;
            }
        }
    }
}

void handle_sign_up_input() {
    while (true) {
        mvgetstr(USERNAME_FIELD_Y, ALL_FIELD_X, user_info.username);
        // if (!get_input(USERNAME_FIELD_Y, ALL_FIELD_X, user_info.username))
        // {
        // main_menu();
        // }
        if (!is_username_valid()) {
            curs_set(0);
            move(USERNAME_FIELD_Y, 1);
            clrtoeol();
            draw_base_form(YELLOW);
            move(USERNAME_FIELD_Y, ALL_FIELD_X);
            curs_set(1);
        } else {
            if (is_username_used()) {
                print_lmsg(2, "Your username is used!");
                curs_set(0);
                move(USERNAME_FIELD_Y, 1);
                clrtoeol();
                draw_base_form(YELLOW);
                move(USERNAME_FIELD_Y, ALL_FIELD_X);
                curs_set(1);
            } else {
                attron(COLOR_PAIR(3));
                mvprintw(USERNAME_FIELD_Y, ALL_FIELD_X, "%s",
                         user_info.username);
                attroff(COLOR_PAIR(3));
                refresh();
                break;
            }
        }
    }
    while (true) {
        mvgetstr(PASSWORD_FIELD_Y, ALL_FIELD_X, user_info.password);
        // if (!get_input(PASSWORD_FIELD_Y, ALL_FIELD_X, user_info.password))
        // {
        //     main_menu();
        // }
        if (!is_password_valid()) {
            curs_set(0);
            move(PASSWORD_FIELD_Y, ALL_FIELD_X);
            clrtoeol();
            draw_base_form(YELLOW);
            move(PASSWORD_FIELD_Y, ALL_FIELD_X);
            curs_set(1);
        } else {
            attron(COLOR_PAIR(3));
            int sizeStar = strlen(user_info.password);
            char stars[sizeStar + 1];
            memset(stars, '*', sizeStar + 1);
            stars[sizeStar] = '\0';
            mvprintw(PASSWORD_FIELD_Y, ALL_FIELD_X, "%s", stars);
            attroff(COLOR_PAIR(3));
            refresh();
            break;
        }
    }
    while (true) {
        mvgetstr(EMAIL_FIELD_Y, ALL_FIELD_X, user_info.email);
        // if (!get_input(EMAIL_FIELD_Y, ALL_FIELD_X, user_info.email))
        // {
        //     main_menu();
        // }
        if (!is_email_valid()) {
            curs_set(0);
            move(EMAIL_FIELD_Y, ALL_FIELD_X);
            clrtoeol();
            draw_base_form(YELLOW);
            move(EMAIL_FIELD_Y, ALL_FIELD_X);
            curs_set(1);
        } else {
            save_userinfo();
            attron(COLOR_PAIR(3));
            mvprintw(EMAIL_FIELD_Y, ALL_FIELD_X, "%s", user_info.email);
            attroff(COLOR_PAIR(3));
            refresh();
            break;
        }
    }
}

void handle_login_input() {
    while (true) {
        mvgetstr(USERNAME_FIELD_Y, ALL_FIELD_X, user_info.username);
        // get_input(USERNAME_FIELD_Y, ALL_FIELD_X, user_info.username);
        if (!is_username_used()) {
            print_lmsg(2, "Username not found!");
            curs_set(0);
            move(USERNAME_FIELD_Y, 1);
            clrtoeol();
            draw_base_form(MAGENTA);
            move(USERNAME_FIELD_Y, ALL_FIELD_X);
            curs_set(1);
        } else {
            attron(COLOR_PAIR(3));
            mvprintw(USERNAME_FIELD_Y, ALL_FIELD_X, "%s", user_info.username);
            attroff(COLOR_PAIR(3));
            refresh();
            break;
        }
    }
    while (true) {
        mvgetstr(PASSWORD_FIELD_Y, ALL_FIELD_X, user_info.password);
        // get_input(PASSWORD_FIELD_Y, ALL_FIELD_X, user_info.password);
        if (is_password_correct()) {
            print_lmsg(3, "Login successfull!");
            logged_in_user = user_info;
            attron(COLOR_PAIR(3));
            int sizeStar = strlen(user_info.password);
            char stars[sizeStar + 1];
            memset(stars, '*', sizeStar + 1);
            stars[sizeStar] = '\0';
            mvprintw(PASSWORD_FIELD_Y, ALL_FIELD_X, "%s", stars);
            attroff(COLOR_PAIR(3));
            refresh();
            break;
        } else {
            print_lmsg(2, "Incorrect password!");
            curs_set(0);
            move(PASSWORD_FIELD_Y, ALL_FIELD_X);
            clrtoeol();
            draw_base_form(MAGENTA);
            move(PASSWORD_FIELD_Y, ALL_FIELD_X);
            curs_set(1);
        }
    }
}

bool is_username_valid() {
    int size = strlen(user_info.username);
    if (size == 0) {
        print_lmsg(2, "Invalid username!");
        return false;
    }
    return true;
}

void print_lmsg(int color_pair, const char *errmsg) {
    int size = strlen(errmsg);
    curs_set(0);
    attron(COLOR_PAIR(color_pair));
    attron(A_BOLD);
    attron(A_ITALIC);
    mvprintw(ERROR_FIELD_Y, COLS / 2 - size / 2, "%s", errmsg);
    attroff(COLOR_PAIR(color_pair));
    attroff(A_BOLD);
    attroff(A_ITALIC);
    refresh();
    sleep(2);
    move(ERROR_FIELD_Y, COLS / 2 - size / 2);
    clrtoeol();
    // draw_base_form();
    move(PASSWORD_FIELD_Y, ALL_FIELD_X);
    // curs_set(1);
}

bool is_password_valid() {
    int size = strlen(user_info.password);
    char u_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char l_alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char digits[] = "0123456789";
    if (size < 7 || strpbrk(user_info.password, u_alphabet) == NULL ||
        strpbrk(user_info.password, l_alphabet) == NULL ||
        strpbrk(user_info.password, digits) == NULL) {
        print_lmsg(2, "Invalid password!");
        return false;
    }
    return true;
}

bool is_password_correct() {
    FILE *file = fopen("userinfo.bin", "rb");
    if (file == NULL) {
        return false;
    }

    UserInfo search;
    while (fread(&search, sizeof(UserInfo), 1, file) == 1) {
        if (!strcmp(user_info.username, search.username)) {
            if (!strcmp(user_info.password, search.password)) {
                fclose(file);
                return true;
            }
        }
    }

    fclose(file);
    return false;
}

bool is_email_valid() {
    int check = 0;
    char *pos1 = strchr(user_info.email, '@');
    if (pos1) {
        int len1 = pos1 - user_info.email;
        if (len1 != 0)
            check++;
        char *pos2 = strchr(user_info.email + len1 + 1, '.');
        if (pos2) {
            int len2 = pos2 - user_info.email - len1 - 1;
            if (len2 != 0)
                check++;
            int lastPartSize = strlen(user_info.email + len1 + len2 + 2);
            if (lastPartSize != 0)
                check++;
        }
    }
    if (check != 3) {
        print_lmsg(2, "Invalid email!");
        return false;
    }
    return true;
}
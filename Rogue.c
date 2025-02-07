#include <stdio.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include "game.h"
#include "mainh.h"
#include "menu.h"

int main()
{
    main_menu();
    start_game();
    return 0;
}
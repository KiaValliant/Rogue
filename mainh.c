#include <ncurses.h>
#include "mainh.h"
#define COLOR_ORANGE 8
#define COLOR_GOLD 9
#define COLOR_GRAY 10
#define COLOR_BROWN 11

void call_colors()
{
    if (has_colors())
    {
        start_color();
        init_color(COLOR_BROWN, 600, 300, 0);
        init_color(COLOR_GRAY, 500, 500, 500);
        init_color(COLOR_ORANGE, 1000, 647, 0);
        init_color(COLOR_GOLD, 1000, 843, 0);
        // init_color(COLOR_YELLOW, 1000, 1000, 1000);
        init_pair(1, COLOR_YELLOW, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_ORANGE, COLOR_BLACK);
        init_pair(7, COLOR_GOLD, COLOR_BLACK);
        init_pair(8, COLOR_WHITE, COLOR_BLACK);
        init_pair(9, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(10, COLOR_GRAY, COLOR_BLACK);
        init_pair(11, COLOR_BROWN, COLOR_BLACK);
    }
}
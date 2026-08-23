#include "acorn/gui.h"
#include "acorn/framebuffer.h"
#include "acorn/mouse.h"
#include "acorn/serial.h"

#define COLOR_BG 0x101820
#define COLOR_BAR 0x183A5A
#define COLOR_PANEL 0x244052
#define COLOR_WINDOW 0x1D303A
#define COLOR_TITLE 0x2A5B63
#define COLOR_TEXT 0xD8F3DC
#define COLOR_CURSOR 0xF4D35E

static unsigned int terminal_x;
static char terminal_line[80];
static unsigned int terminal_length;
static unsigned long last_mouse_packets;
static unsigned long gui_ticks;
static char terminal_output[80];
static unsigned int terminal_output_length;
static unsigned int gui_mode;

enum { GUI_DESKTOP, GUI_FILES, GUI_CALCULATOR, GUI_SNAKE };
enum { KEY_F1 = 0x80, KEY_F2, KEY_F3 };

static int text_equals(const char *left, const char *right)
{
    unsigned int index = 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        ++index;
    }
    return left[index] == right[index];
}

static void text_copy(char *destination, const char *source)
{
    unsigned int index = 0;
    while (source[index] != '\0' && index + 1 < sizeof(terminal_output)) {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
    terminal_output_length = index;
}

static int begins_with(const char *text, const char *prefix)
{
    unsigned int index = 0;
    while (prefix[index] != '\0') {
        if (text[index] != prefix[index]) return 0;
        ++index;
    }
    return 1;
}

static long calculate(const char *expression)
{
    long result = 0;
    long value = 0;
    char operation = '+';
    unsigned int index = 0;
    for (;;) {
        while (expression[index] >= '0' && expression[index] <= '9') {
            value = value * 10 + expression[index] - '0';
            ++index;
        }
        if (operation == '+') result += value;
        else if (operation == '-') result -= value;
        else if (operation == '*') result *= value;
        else if (operation == '/' && value != 0) result /= value;
        value = 0;
        if (expression[index] == '\0') return result;
        operation = expression[index++];
    }
}

static void number_text(char *destination, long value)
{
    char reverse[24];
    unsigned int length = 0;
    int negative = value < 0;
    unsigned long magnitude = negative ? (unsigned long)(-value) : (unsigned long)value;
    if (magnitude == 0) reverse[length++] = '0';
    while (magnitude != 0) {
        reverse[length++] = (char)('0' + magnitude % 10);
        magnitude /= 10;
    }
    unsigned int output = 0;
    if (negative) destination[output++] = '-';
    while (length != 0) destination[output++] = reverse[--length];
    destination[output] = '\0';
}

static void draw_cursor(void)
{
    unsigned int x = (unsigned int)mouse_x();
    unsigned int y = (unsigned int)mouse_y();
    static const unsigned short shape[16] = {
        0x8000, 0xC000, 0xE000, 0xF000,
        0xF800, 0xFC00, 0xFE00, 0xFF00,
        0xFF80, 0xFFC0, 0xF0C0, 0x60C0,
        0x60C0, 0x30C0, 0x30C0, 0x0000
    };
    for (unsigned int row = 0; row < 16; ++row) {
        for (unsigned int column = 0; column < 16; ++column) {
            if ((shape[row] & (1u << (15 - column))) == 0) continue;
            framebuffer_fill_rect(x + column + 1, y + row + 1, 1, 1, 0x050505);
        }
    }
    for (unsigned int row = 0; row < 16; ++row) {
        for (unsigned int column = 0; column < 16; ++column) {
            if ((shape[row] & (1u << (15 - column))) == 0) continue;
            framebuffer_fill_rect(x + column, y + row, 1, 1, 0xF7F7F2);
        }
    }
}

static void draw_calculator(void)
{
    framebuffer_fill_rect(318, 180, 410, 300, COLOR_WINDOW);
    framebuffer_fill_rect(318, 180, 410, 30, COLOR_TITLE);
    framebuffer_draw_text(334, 190, "CALCULATOR", COLOR_TEXT, 2);
    framebuffer_draw_text(336, 236, "TYPE: calc 12+30", COLOR_TEXT, 1);
    framebuffer_draw_text(336, 268, "RESULT", COLOR_CURSOR, 1);
    framebuffer_draw_text(336, 286, terminal_output, COLOR_TEXT, 2);
}

static void draw_files(void)
{
    framebuffer_fill_rect(318, 180, 410, 300, COLOR_WINDOW);
    framebuffer_fill_rect(318, 180, 410, 30, COLOR_TITLE);
    framebuffer_draw_text(334, 190, "FILES", COLOR_TEXT, 2);
    framebuffer_draw_text(338, 236, "PERSISTENT FILES", COLOR_TEXT, 1);
    framebuffer_draw_text(338, 264, "/tmp/hello", COLOR_TEXT, 2);
    framebuffer_draw_text(338, 296, "/persistent", COLOR_TEXT, 2);
    framebuffer_draw_text(338, 328, "/persistent-dir", COLOR_TEXT, 2);
}

static unsigned int snake_x[32] = { 12, 11, 10, 9 };
static unsigned int snake_y[32] = { 8, 8, 8, 8 };
static unsigned int snake_length = 4;
static int snake_dx = 1;
static int snake_dy;

static void draw_snake(void)
{
    framebuffer_fill_rect(318, 180, 410, 300, COLOR_WINDOW);
    framebuffer_fill_rect(318, 180, 410, 30, COLOR_TITLE);
    framebuffer_draw_text(334, 190, "SNAKE", COLOR_TEXT, 2);
    framebuffer_draw_text(540, 190, "WASD MOVE", COLOR_TEXT, 1);
    for (unsigned int index = 0; index < snake_length; ++index)
        framebuffer_fill_rect(340 + snake_x[index] * 12,
            230 + snake_y[index] * 12, 10, 10,
            index == 0 ? COLOR_CURSOR : 0x4CC9A4);
}

static void snake_step(void)
{
    for (unsigned int index = snake_length - 1; index != 0; --index) {
        snake_x[index] = snake_x[index - 1];
        snake_y[index] = snake_y[index - 1];
    }
    snake_x[0] = (unsigned int)((int)snake_x[0] + snake_dx + 30) % 30;
    snake_y[0] = (unsigned int)((int)snake_y[0] + snake_dy + 18) % 18;
}

static void draw_terminal(void)
{
    framebuffer_fill_rect(318, 180, 410, 300, COLOR_WINDOW);
    framebuffer_fill_rect(318, 180, 410, 30, COLOR_TITLE);
    framebuffer_draw_text(334, 190, "TERMINAL", COLOR_TEXT, 2);
    framebuffer_draw_text(334, 230, "OakOS graphical shell", COLOR_TEXT, 1);
    framebuffer_draw_text(334, 250, ">", COLOR_CURSOR, 1);
    framebuffer_draw_text(346, 250, terminal_line, COLOR_TEXT, 1);
    framebuffer_draw_text(334, 282, terminal_output, COLOR_CURSOR, 1);
    framebuffer_fill_rect(334 + terminal_x, 266, 6, 2, COLOR_CURSOR);
}

static void draw_desktop(void)
{
    unsigned int screen_width = framebuffer_width();
    unsigned int screen_height = framebuffer_height();
    framebuffer_clear(COLOR_BG);
    framebuffer_fill_rect(0, 0, screen_width, 56, COLOR_BAR);
    framebuffer_draw_text(24, 18, "OAKOS DESKTOP", COLOR_TEXT, 2);
    framebuffer_fill_rect(screen_width - 120, 16, 88, 24, 0x4CC9A4);
    framebuffer_fill_rect(26, 78, 270, screen_height - 110, COLOR_PANEL);
    framebuffer_fill_rect(26, 78, 270, 32, COLOR_TITLE);
    framebuffer_draw_text(40, 88, "WORKSPACE", COLOR_TEXT, 1);
    framebuffer_draw_text(48, 140, "FILES", COLOR_TEXT, 2);
    framebuffer_draw_text(48, 174, "TERMINAL", COLOR_TEXT, 2);
    framebuffer_draw_text(48, 208, "SETTINGS", COLOR_TEXT, 2);
    framebuffer_fill_rect(42, 246, 56, 42, COLOR_TITLE);
    framebuffer_fill_rect(112, 246, 56, 42, COLOR_TITLE);
    framebuffer_fill_rect(182, 246, 56, 42, COLOR_TITLE);
    framebuffer_draw_text(50, 258, "F1", COLOR_TEXT, 1);
    framebuffer_draw_text(120, 258, "F2", COLOR_TEXT, 1);
    framebuffer_draw_text(190, 258, "F3", COLOR_TEXT, 1);
    framebuffer_draw_text(40, 300, "APPS", COLOR_CURSOR, 1);
    framebuffer_fill_rect(318, 78, 410, 76, COLOR_TITLE);
    framebuffer_draw_text(336, 96, "WELCOME TO OAKOS", COLOR_TEXT, 2);
    framebuffer_draw_text(336, 124, "A small graphical kernel", COLOR_TEXT, 1);
    draw_terminal();
    if (gui_mode == GUI_FILES) draw_files();
    if (gui_mode == GUI_CALCULATOR) draw_calculator();
    if (gui_mode == GUI_SNAKE) draw_snake();
    draw_cursor();
}

void gui_init(void)
{
    terminal_x = 0;
    terminal_length = 0;
    terminal_line[0] = '\0';
    terminal_output[0] = '\0';
    terminal_output_length = 0;
    gui_mode = GUI_DESKTOP;
    gui_ticks = 0;
    last_mouse_packets = 0;
    if (framebuffer_available()) draw_desktop();
}

void gui_keyboard_input(int value)
{
    if (value == KEY_F1) {
        gui_mode = GUI_FILES;
        text_copy(terminal_output, "files app");
        draw_desktop();
        return;
    }
    if (value == KEY_F2) {
        gui_mode = GUI_CALCULATOR;
        text_copy(terminal_output, "0");
        draw_desktop();
        return;
    }
    if (value == KEY_F3) {
        gui_mode = GUI_SNAKE;
        text_copy(terminal_output, "snake: WASD move, Q quit");
        draw_desktop();
        return;
    }
    if (value == 0x1B) {
        gui_mode = GUI_DESKTOP;
        text_copy(terminal_output, "desktop ready");
        draw_desktop();
        return;
    }
    if (gui_mode == GUI_SNAKE) {
        if (value == 'w') { snake_dx = 0; snake_dy = -1; }
        if (value == 's') { snake_dx = 0; snake_dy = 1; }
        if (value == 'a') { snake_dx = -1; snake_dy = 0; }
        if (value == 'd') { snake_dx = 1; snake_dy = 0; }
        if (value == 'q') { gui_mode = GUI_DESKTOP; text_copy(terminal_output, "snake closed"); }
        draw_desktop();
        return;
    }
    if (value == '\b') {
        if (terminal_length != 0) --terminal_length;
    } else if (value == '\n') {
        if (text_equals(terminal_line, "help"))
            text_copy(terminal_output, "help calc snake files clear");
        else if (text_equals(terminal_line, "clear"))
            text_copy(terminal_output, "");
        else if (text_equals(terminal_line, "files"))
            text_copy(terminal_output, "FILES: /tmp/hello /persistent");
        else if (text_equals(terminal_line, "desktop")) {
            gui_mode = GUI_DESKTOP;
            text_copy(terminal_output, "desktop ready");
        } else if (text_equals(terminal_line, "snake")) {
            gui_mode = GUI_SNAKE;
            snake_x[0] = 12; snake_y[0] = 8;
            snake_dx = 1; snake_dy = 0;
            text_copy(terminal_output, "snake: WASD move, Q quit");
        } else if (begins_with(terminal_line, "calc ")) {
            char result[24];
            number_text(result, calculate(terminal_line + 5));
            text_copy(terminal_output, result);
            gui_mode = GUI_CALCULATOR;
        } else text_copy(terminal_output, "unknown command: help");
        terminal_length = 0;
    } else if (value >= 32 && value <= 126 && terminal_length + 1 < sizeof(terminal_line)) {
        terminal_line[terminal_length++] = (char)value;
    }
    terminal_line[terminal_length] = '\0';
    terminal_x = terminal_length * 6;
    draw_desktop();
}

void gui_mouse_input(int x, int y, unsigned char buttons)
{
    if ((buttons & 1) == 0) return;
    if (gui_mode != GUI_DESKTOP || y < 246 || y > 288) return;
    if (x >= 42 && x < 98) gui_mode = GUI_FILES;
    else if (x >= 112 && x < 168) gui_mode = GUI_CALCULATOR;
    else if (x >= 182 && x < 238) gui_mode = GUI_SNAKE;
    else return;
    text_copy(terminal_output, gui_mode == GUI_SNAKE ?
        "snake: WASD move, Q quit" : (gui_mode == GUI_CALCULATOR ? "0" : "files app"));
    draw_desktop();
}

void gui_tick(void)
{
    if (!framebuffer_available()) return;
    ++gui_ticks;
    if (gui_mode == GUI_SNAKE && gui_ticks % 20 == 0) {
        snake_step();
        draw_desktop();
    }
    if (mouse_moved()) {
        draw_desktop();
        if (mouse_packet_count() != last_mouse_packets) {
            last_mouse_packets = mouse_packet_count();
            serial_write("mouse event: x=");
            serial_write_hex((unsigned long)mouse_x());
            serial_write(" y=");
            serial_write_hex((unsigned long)mouse_y());
            serial_write(" packets=");
            serial_write_hex(last_mouse_packets);
            serial_write("\n");
        }
    }
}

int gui_self_test(void)
{
    return framebuffer_available() && framebuffer_width() >= 320 &&
        framebuffer_height() >= 200;
}

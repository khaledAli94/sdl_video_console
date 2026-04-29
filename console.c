#include "console.h"
#include "font8x8.h"
#include <string.h>
#include <SDL2/SDL.h>

static unsigned char screen[CONSOLE_ROWS][CONSOLE_COLS];
static int line_len[CONSOLE_ROWS];   // FIXED: missing array

static int cursor_x = 0;
static int cursor_y = 0;
static int insert_mode = 0;

static int esc_state = 0;
static char esc_buf[16];
static int esc_len = 0;

static int cursor_visible = 1;
static Uint32 last_blink = 0;
static const Uint32 BLINK_INTERVAL = 500;

/* ---------- FORWARD DECLARATIONS (fix implicit use) ---------- */
static void update_line_length(int row);
static void delete_prev_word(void);
static void delete_next_word(void);
static void cursor_word_left(void);
static void cursor_word_right(void);

/* ---------- LINE LENGTH ---------- */

static int line_length(int row) {
    int len = CONSOLE_COLS;
    while (len > 0 && screen[row][len - 1] == ' ')
        len--;
    return len;
}

static void update_line_length(int row) {
    int len = CONSOLE_COLS;
    while (len > 0 && screen[row][len - 1] == ' ')
        len--;
    line_len[row] = len;
}

/* ---------- INIT ---------- */

void console_init(void) {
    memset(screen, ' ', sizeof(screen));
    memset(line_len, 0, sizeof(line_len));   // FIXED
    cursor_x = cursor_y = 0;
}

/* ---------- SCROLL ---------- */

static void scroll_up(void) {
    for (int y = 1; y < CONSOLE_ROWS; y++) {
        memcpy(screen[y - 1], screen[y], CONSOLE_COLS);
        line_len[y - 1] = line_len[y];       // FIXED
    }
    memset(screen[CONSOLE_ROWS - 1], ' ', CONSOLE_COLS);
    line_len[CONSOLE_ROWS - 1] = 0;          // FIXED
    cursor_y = CONSOLE_ROWS - 1;
}

/* ---------- DELETE ---------- */

static void delete_at_cursor(void) {
    int len = line_length(cursor_y);
    if (cursor_x >= len) return;

    for (int x = cursor_x; x < len - 1; x++)
        screen[cursor_y][x] = screen[cursor_y][x + 1];

    screen[cursor_y][len - 1] = ' ';
    update_line_length(cursor_y);            // FIXED
}

static void backspace_char(void) {
    if (cursor_x == 0) return;
    cursor_x--;
    delete_at_cursor();
}

/* ---------- WORD DELETE ---------- */

static void delete_prev_word(void) {
    int start = cursor_x;

    /* skip spaces */
    while (cursor_x > 0 && screen[cursor_y][cursor_x - 1] == ' ')
        cursor_x--;

    /* skip word */
    while (cursor_x > 0 && screen[cursor_y][cursor_x - 1] != ' ')
        cursor_x--;

    int delta = start - cursor_x;

    for (int i = 0; i < delta; i++)
        delete_at_cursor();

    update_line_length(cursor_y);
}



static void delete_next_word(void) {
    int len = line_len[cursor_y];

    while (cursor_x < len && screen[cursor_y][cursor_x] == ' ')
        delete_at_cursor();

    while (cursor_x < len && screen[cursor_y][cursor_x] != ' ')
        delete_at_cursor();

    update_line_length(cursor_y);
}

/* ---------- CLEAR ---------- */

static void clear_line(void) {
    memset(screen[cursor_y], ' ', CONSOLE_COLS);
    cursor_x = 0;
    line_len[cursor_y] = 0;                  // FIXED
}

/* ---------- WORD MOVEMENT ---------- */

static void cursor_word_left(void) {
    int x = cursor_x;
    if (x == 0) return;

    x--;
    while (x > 0 && screen[cursor_y][x] == ' ') x--;
    while (x > 0 && screen[cursor_y][x - 1] != ' ') x--;

    cursor_x = x;
}

static void cursor_word_right(void) {
    int len = line_len[cursor_y];
    int x = cursor_x;

    while (x < len && screen[cursor_y][x] != ' ')
        x++;

    while (x < len && screen[cursor_y][x] == ' ')
        x++;

    cursor_x = x;
}

/* ---------- INSERT ---------- */

static void insert_char(unsigned char c) {
    int len = line_length(cursor_y);

    if (!insert_mode || cursor_x >= len) {
        if (cursor_x < CONSOLE_COLS)
            screen[cursor_y][cursor_x] = c;

        cursor_x++;

        if (cursor_x >= CONSOLE_COLS) {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= CONSOLE_ROWS)
                scroll_up();
        }

        update_line_length(cursor_y);        // FIXED
        return;
    }

    if (len >= CONSOLE_COLS) len = CONSOLE_COLS - 1;

    for (int x = len; x > cursor_x; x--)
        screen[cursor_y][x] = screen[cursor_y][x - 1];

    screen[cursor_y][cursor_x] = c;
    cursor_x++;

    update_line_length(cursor_y);            // FIXED
}

/* ---------- NORMAL CHAR ---------- */

static void process_normal_char(unsigned char c) {
    switch (c) {

    case '\n':      /* LF – Line Feed: move cursor down one line */
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= CONSOLE_ROWS)
            scroll_up();
        else
            line_len[cursor_y] = 0;
        return;

    case '\r':      /* CR – Carriage Return: move cursor to column 0 */
        cursor_x = 0;
        return;

    case 0x08:      /* BS – Backspace: delete character before cursor */
        delete_prev_word();   /* your custom behavior */
        return;

    case 0x7F:      /* DEL – Delete: erase previous character (common Backspace) */
        backspace_char();
        return;

    case 0x01:      /* SOH – Ctrl+A: move cursor to start of line */
        cursor_x = 0;
        return;

    case 0x05:      /* ENQ – Ctrl+E: move cursor to end of line */
        cursor_x = line_length(cursor_y);
        return;

    case 0x17:      /* ETB – Ctrl+W: delete previous word */
        delete_prev_word();
        return;

    case 0x15:      /* NAK – Ctrl+U: clear entire line */
        clear_line();
        return;

    case 0x09:      /* HT – Horizontal Tab: insert 4 spaces */
        for (int i = 0; i < 4; i++)
            insert_char(' ');
        return;

    case 0x16:      /* SYN – Ctrl+V: toggle insert mode (your custom behavior) */
        insert_mode = !insert_mode;
        return;

    default:
        if (c < 32)
            return; /* ignore other control codes */

        /* Printable ASCII */
        insert_char(c);
        return;
    }
}


/* ---------- CURSOR MOVEMENT ---------- */

static void cursor_left(void) {
    if (cursor_x > 0)
        cursor_x--;
}

static void cursor_right(void) {
    if (cursor_x < line_len[cursor_y])
        cursor_x++;
}

static void cursor_up(void) {
    if (cursor_y > 0) {
        cursor_y--;
        int len = line_length(cursor_y);
        if (cursor_x > len)
            cursor_x = len;
    }
}

static void cursor_down(void) {
    if (cursor_y < CONSOLE_ROWS - 1) {
        cursor_y++;
        int len = line_length(cursor_y);
        if (cursor_x > len)
            cursor_x = len;
    }
}

static void cursor_home(void) {
    cursor_x = 0;
}

static void cursor_end(void) {
    cursor_x = line_length(cursor_y);
}

/* ---------- ESC PARSER ---------- */

static void handle_esc(unsigned char c) {
    esc_buf[esc_len++] = c;

    if (esc_len >= 15) {
        esc_state = 0;
        esc_len = 0;
        return;
    }

    /* ESC [ */
    if (esc_state == 1) {
        if (c == '[') {
            esc_state = 2;
            return;
        }
        esc_state = 0;
        esc_len = 0;
        return;
    }

    /* ESC [ ... */
    if (esc_state == 2) {

        /* --- Ctrl + Arrow: ESC [ 1 ; 5 D/C/A/B --- */
        if (esc_len >= 5 &&
            esc_buf[1] == '1' &&
            esc_buf[2] == ';' &&
            esc_buf[3] == '5') {

            if (c == 'D') { cursor_word_left();  esc_state = 0; esc_len = 0; return; }
            if (c == 'C') { cursor_word_right(); esc_state = 0; esc_len = 0; return; }
            if (c == 'A') { cursor_up();         esc_state = 0; esc_len = 0; return; }
            if (c == 'B') { cursor_down();       esc_state = 0; esc_len = 0; return; }
        }

        /* --- Simple arrows --- */
        if (c == 'A') { cursor_up();    esc_state = 0; esc_len = 0; return; }
        if (c == 'B') { cursor_down();  esc_state = 0; esc_len = 0; return; }
        if (c == 'C') { cursor_right(); esc_state = 0; esc_len = 0; return; }
        if (c == 'D') { cursor_left();  esc_state = 0; esc_len = 0; return; }

        /* --- Home / End --- */
        if (c == 'H') { cursor_home(); esc_state = 0; esc_len = 0; return; }
        if (c == 'F') { cursor_end();  esc_state = 0; esc_len = 0; return; }

        /* --- Delete / Insert / Ctrl+Delete --- */
        if (c == '~') {
            /* esc_buf[0] = '[', esc_buf[1] = '3' or '2', ... */

            /* Delete: ESC [ 3 ~ */
            if (esc_len == 3 && esc_buf[1] == '3' && esc_buf[2] == '~') {
                delete_at_cursor();
                esc_state = 0;
                esc_len = 0;
                return;
            }

            /* Ctrl+Delete: ESC [ 3 ; 5 ~ */
            if (esc_len >= 4 &&
                esc_buf[1] == '3' &&
                esc_buf[2] == ';' &&
                esc_buf[3] == '5') {
                delete_next_word();
                esc_state = 0;
                esc_len = 0;
                return;
            }

            /* Ctrl+Backspace: ESC [ 3 ; 2/3/4 ~ */
            if (esc_len >= 4 &&
                esc_buf[1] == '3' &&
                esc_buf[2] == ';' &&
                (esc_buf[3] == '2' || esc_buf[3] == '3' || esc_buf[3] == '4')) {
                delete_prev_word();
                esc_state = 0;
                esc_len = 0;
                return;
            }

            /* Insert: ESC [ 2 ~ */
            if (esc_len >= 2 && esc_buf[1] == '2') {
                insert_mode = !insert_mode;
                esc_state = 0;
                esc_len = 0;
                return;
            }

            esc_state = 0;
            esc_len = 0;
            return;
        }
    }
}

/* ---------- PUBLIC API ---------- */

void console_putc(unsigned char c) {
    if (esc_state) {
        handle_esc(c);
        return;
    }

    if (c == 0x1B) {
        esc_state = 1;
        esc_len = 0;
        return;
    }

    process_normal_char(c);
}

void console_tick_blink(void) {
    Uint32 now = SDL_GetTicks();
    if (now - last_blink >= BLINK_INTERVAL) {
        cursor_visible = !cursor_visible;
        last_blink = now;
    }
}

void console_render(void *rend, uint32_t fg, uint32_t bg) {
    SDL_Renderer *renderer = (SDL_Renderer*)rend;

    SDL_SetRenderDrawColor(renderer, bg>>16, (bg>>8)&255, bg&255, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < CONSOLE_ROWS; y++) {
        for (int x = 0; x < CONSOLE_COLS; x++) {
            unsigned char c = screen[y][x];

            // clamp to valid font range
            if (c >= 128)
                c = '?';

            for (int row = 0; row < FONT_H; row++) {
                uint8_t bits = font8x8_basic[c][row];
                for (int col = 0; col < FONT_W; col++) {
                    uint32_t color = (bits & (1u<<col)) ? fg : bg;
                    SDL_SetRenderDrawColor(renderer,
                        color>>16, (color>>8)&255, color&255, 255);
                    SDL_RenderDrawPoint(renderer,
                        x*FONT_W + col, y*FONT_H + row);
                }
            }
        }
    }

    if (cursor_visible) {
        SDL_Rect r = {
            cursor_x * FONT_W,
            cursor_y * FONT_H + FONT_H - 2,
            FONT_W,
            2
        };
        SDL_SetRenderDrawColor(renderer, fg>>16, (fg>>8)&255, fg&255, 255);
        SDL_RenderFillRect(renderer, &r);
    }
}

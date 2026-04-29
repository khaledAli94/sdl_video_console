#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define FONT_W (8)
#define FONT_H (8)

#define SCR_W (480)
#define SCR_H (272)

#define CONSOLE_COLS  (SCR_W / FONT_W)
#define CONSOLE_ROWS  (SCR_H / FONT_H)

void console_init(void);
void console_putc(unsigned char c);
void console_render(void *renderer, uint32_t fg, uint32_t bg);
void console_tick_blink(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_H__ */

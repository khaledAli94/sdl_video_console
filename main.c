#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "console.h"
#include "input_raw.h"

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR, "0");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    
    SDL_Window *win = SDL_CreateWindow("SDL Console",SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_W, SCR_H, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    input_raw_init();
    console_init();

    int running = 1;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT)
                running = 0;

        unsigned char buf[256];
        int n = input_raw_read(buf, sizeof(buf));
        for (int i = 0; i < n; i++)
            console_putc(buf[i]);

        console_tick_blink();
        console_render(renderer, 0xFFFFFF, 0x000000);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    input_raw_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

/* SPDX-License-Identifier: Zlib
 *
 * sdl2-gltest.c — minimal fullscreen OpenGL smoke test for the Phoenix-RTOS
 * SDL2 port (video+input driver over /dev/fb0 + Mesa V3D). Exercises the exact
 * milestone path: SDL_Init(VIDEO) -> fullscreen GL window -> GL context ->
 * clear + swap -> input pump loop. This is a link/smoke target; the on-Pi run
 * is a later turn.
 */
#include "SDL.h"
#include "SDL_opengl.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_GLContext ctx;
    int running = 1;
    int frames = 0;

    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init(VIDEO) failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow("phoenix-sdl2-gltest",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              1920, 1080,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, ctx);
    SDL_GL_SetSwapInterval(1);

    printf("GL_VERSION:  %s\n", (const char *)glGetString(GL_VERSION));
    printf("GL_RENDERER: %s\n", (const char *)glGetString(GL_RENDERER));

    while (running) {
        SDL_Event ev;
        SDL_PumpEvents();
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                running = 0;
            }
        }

        /* Animate the clear colour so a real run shows the app is alive. */
        {
            float t = (float)(frames % 120) / 120.0f;
            glClearColor(0.1f, t, 0.3f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapWindow(window);

        if (++frames >= 600) { /* ~10 s at 60 Hz, then exit cleanly */
            running = 0;
        }
    }

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("sdl2-gltest: %d frames, clean exit\n", frames);
    return 0;
}

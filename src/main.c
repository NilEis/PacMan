#include "raylib.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#include <stdlib.h>

#define WIDTH (800.0)
#define HEIGHT (600.0)

#define TEXTURE_WIDTH (WIDTH / 2.0)
#define TEXTURE_HEIGHT (HEIGHT / 2.0)

void main_loop (void *arg);

int main (int argc, char const **argv)
{
    InitWindow (WIDTH, HEIGHT, "PacMan");
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg (main_loop, NULL, 0, 1);
#else
    SetTargetFPS (60);
    while (!WindowShouldClose ())
    {
        main_loop (NULL);
    }
#endif
    CloseWindow ();
    return 0;
}

void main_loop (void *arg)
{
    BeginDrawing ();
    EndDrawing ();
#ifdef __EMSCRIPTEN__
    if (WindowShouldClose ())
    {
        emscripten_cancel_main_loop ();
    }
#endif
}
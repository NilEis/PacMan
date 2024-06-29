//
// Created by NilEis on 29.06.2024.
//

#ifndef PACMAN_NUKLEAR_DEFAULT_H
#define PACMAN_NUKLEAR_DEFAULT_H

// #define NK_ASSERT(x)                                                          \
//     do                                                                        \
//     {                                                                         \
//         if (!(x))                                                             \
//         {                                                                     \
//             *(int *)0 = 0;                                                    \
//         }                                                                     \
//     }                                                                         \
//     while (0)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_BUTTON_TRIGGER_ON_RELEASE

#endif // PACMAN_NUKLEAR_DEFAULT_H
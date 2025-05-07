#ifndef __AGL_EXAMPLES_TRIANGLE_H
#define __AGL_EXAMPLES_TRIANGLE_H

#include "MacWindows.h"
#include <Quickdraw.h>
#include <Threads.h>
#include <Windows.h>
#include <agl.h>
#include <aglMacro.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  GLenum aglErr;
  GLenum glErr;
  Rect winRect;
  WindowPtr window;
  AGLPixelFormat pixFormat;
  AGLContext context;

} triangle_context;

// Execute an AGL function and then exit, printing the message, if it fails.
#define AGL_ERROR(aglErr, x)                                                   \
  x;                                                                           \
  aglErr = aglGetError();                                                      \
  if (aglErr != AGL_NO_ERROR) {                                                \
    printf("AGL Error on %s: %s\n", #x, aglErrorString(aglErr));               \
    getchar();                                                                 \
    exit(1);                                                                   \
  }

// Execute an AGL function and then exit, printing the message, if it fails.
#define GL_ERROR(glErr, x)                                                     \
  x;                                                                           \
  glErr = glGetError();                                                        \
  if (glErr != GL_NO_ERROR) {                                                  \
    printf("GL Error on %s: %ld\n", #x, glErr);                                \
    getchar();                                                                 \
    exit(1);                                                                   \
  }
void setup_window(triangle_context *ctx);
void setup_agl(triangle_context *ctx);
void destroy_agl(triangle_context *ctx);
void destroy_window(triangle_context *ctx);

#endif
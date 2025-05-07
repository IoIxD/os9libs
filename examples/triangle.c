#include "triangle.h"
#include "MacMemory.h"
#include "MacWindows.h"
#include "Quickdraw.h"
#include "QuickdrawText.h"
#include <Dialogs.h>
#include <stdio.h>
#include <stdlib.h>

const ConstStr255Param test = "\ptest\0";

#define agl_ctx ctx.context

int main() {
  triangle_context ctx;

  setup_window(&ctx);
  setup_agl(&ctx);

  bool what = false;
  while (!Button()) {
    BeginUpdate(ctx.window);

    // MoveTo(10, 10);
    // DrawString(test);

    GL_ERROR(ctx.glErr, glClearColor(0, 0, 0, 1));
    GL_ERROR(ctx.glErr, glClear(GL_COLOR_BUFFER_BIT));

    glBegin(GL_TRIANGLES);
    glColor3f(1, 0, 0); // red
    glVertex2f(-0.8, -0.8);
    glColor3f(0, 1, 0); // green
    glVertex2f(0.8, -0.8);
    glColor3f(0, 0, 1); // blue
    glVertex2f(0, 0.9);
    glEnd();

    AGL_ERROR(ctx.aglErr, aglSwapBuffers(ctx.context));

    EndUpdate(ctx.window);

    YieldToAnyThread();
  }

  destroy_window(&ctx);
  destroy_agl(&ctx);
}

void setup_window(triangle_context *ctx) {
  unsigned char *title = (unsigned char *)"\pHello, world!";

  // Setup the Window
  InitGraf(&qd.thePort);
  InitFonts();
  FlushEvents(everyEvent, 0);
  InitWindows();
  InitMenus();
  TEInit();
  InitDialogs(0L);
  InitCursor();

  SetRect(&ctx->winRect, 100, 100, 640, 480);

  ctx->window = NewCWindow(nil, &ctx->winRect, title, true, documentProc,
                           (WindowPtr)(-1), false, 0);
  SetPort(ctx->window);
}

void setup_agl(triangle_context *ctx) {
  aglConfigure(AGL_RETAIN_RENDERERS, GL_TRUE);

  // Choose a pixel format
  GLint aglAttributes[] = {AGL_RGBA, AGL_DOUBLEBUFFER, AGL_NONE};

  AGL_ERROR(ctx->aglErr,
            ctx->pixFormat = aglChoosePixelFormat(NULL, 0, aglAttributes));

  if (ctx->pixFormat == NULL) {
    printf(
        "The system cannot find a pixel format and virtual screen that "
        "satisfies the constraints of the buffer and renderer attributes.\n");
    getchar();
    exit(1);
  }

  // Create a context
  AGL_ERROR(ctx->aglErr, ctx->context = aglCreateContext(ctx->pixFormat, NULL));

  // Set the drawable region to our window
  CGrafPtr port = GetWindowPort(ctx->window);
  AGL_ERROR(ctx->aglErr, aglSetDrawable(ctx->context, port));

  // Set it to be current
  AGL_ERROR(ctx->aglErr, aglSetCurrentContext(ctx->context));
}

void destroy_agl(triangle_context *ctx) {
  aglDestroyPixelFormat(ctx->pixFormat);
  aglSetCurrentContext(NULL);
  aglSetDrawable(ctx->context, NULL);
  aglDestroyContext(ctx->context);
};
void destroy_window(triangle_context *ctx) {
  CloseWindow(ctx->window);
  DisposePtr((Ptr)ctx->window);
};

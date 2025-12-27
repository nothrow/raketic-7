#include "platform.h"

#include <Windows.h>
#include <gl/GL.h>

static uint64_t prev_time_;
static double ticks_;

static HWND hwnd_;

int _fltused = 0;

void _memory_initialize(void);
void _gl_initialize(void);

static void _platform_create_window(void) {
  WNDCLASS wc = { 0 };
  wc.lpfnWndProc = DefWindowProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = "w";
  ATOM a = RegisterClass(&wc);
  _ASSERT(a != 0);

  hwnd_ = CreateWindowA(wc.lpszClassName, "Raketic", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, WINDOW_WIDTH,
                      WINDOW_HEIGHT, 0, 0, wc.hInstance, 0);

  _ASSERT(hwnd_ != NULL);

  HDC hdc = GetDC(hwnd_);
  _ASSERT(hdc != NULL);

  PIXELFORMATDESCRIPTOR pfd = { 0 };
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cAlphaBits = 8;

  int pf = ChoosePixelFormat(hdc, &pfd);
  _ASSERT(pf != 0);

  BOOL b = SetPixelFormat(hdc, pf, &pfd);
  _ASSERT(b != 0);

  HGLRC hglrc = wglCreateContext(hdc);
  _ASSERT(hglrc != NULL);

  b = wglMakeCurrent(hdc, hglrc);
  _ASSERT(b != 0);
}

void platform_initialize(void) {
  _memory_initialize();
  _platform_create_window();
  _gl_initialize();
}


void platform_frame_start(void) {
  glClear(GL_COLOR_BUFFER_BIT);

  uint64_t now = GetTickCount64();
  ticks_ += now - prev_time_;
  prev_time_ = now;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void platform_frame_end(void) {
  SwapBuffers(GetDC(hwnd_));
  Sleep(0);
}

bool platform_tick_pending(void) {
  if (ticks_ >= TICK_MS) {
    ticks_ -= TICK_MS;
    return true;
  }
  return false;
}

bool platform_loop(void) {
  MSG msg;
  while (PeekMessage(&msg, hwnd_, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT || msg.message == WM_CLOSE) {
      return false;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  Sleep(0);
  return true;
}

// declared in main.c
int run(void);

extern void WinMainCRTStartup(void) {
  int ret = run();
  ExitProcess(ret);
}

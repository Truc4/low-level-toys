#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable BOOL Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;

internal void RenderWeirdGradient(int XOffset, int YOffset) {
  int BytesPerPixel = 4;
  int Pitch = BitmapWidth * BytesPerPixel;
  uint8_t *Row = (uint8_t *)BitmapMemory;
  for (int Y = 0; Y < BitmapHeight; ++Y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < BitmapWidth; ++X) {
      uint8_t Blue = (X + XOffset);
      uint8_t Green = (Y + YOffset);
      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Pitch;
  }
}

internal void Win32ResizeDIBSection(int Width, int Height) {
  if (BitmapMemory) {
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int BytesPerPixel = 4;
  int BitmapMemorySize = BytesPerPixel * Width * Height;
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X,
                                int Y, int Width, int Height) {
  int WindowWidth = ClientRect->right - ClientRect->left;
  int WindowHeight = ClientRect->bottom - ClientRect->top;
  StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, 0, 0,
                WindowWidth, WindowHeight, BitmapMemory, &BitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam,
                                    LPARAM LParam) {
  LRESULT Result = 0;
  switch (Message) {
  case WM_SIZE: {
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    int Width = ClientRect.right - ClientRect.left;
    int Height = ClientRect.bottom - ClientRect.top;
    Win32ResizeDIBSection(Width, Height);
    OutputDebugStringA("WM_SIZE\n");
  } break;

  case WM_DESTROY: {
    // TODO Handle this as an error, recreate window
    Running = FALSE;
    OutputDebugStringA("WM_DESTROY\n");
  } break;

  case WM_CLOSE: {
    // TODO handle with a message to the user
    Running = FALSE;
    OutputDebugStringA("WM_CLOSE\n");
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;
  case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(Window, &Paint);
    int X = Paint.rcPaint.left;
    int Y = Paint.rcPaint.top;
    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
    // PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS);
    EndPaint(Window, &Paint);
  } break;
  default: {
    Result = DefWindowProc(Window, Message, WParam, LParam);
  } break;
  }
  return (Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {
  WNDCLASSA WindowClass = {0};

  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if (RegisterClassA(&WindowClass)) {
    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "HandmadeHero",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 0, 0, Instance, 0);
    if (Window) {
      Running = TRUE;
      int XOffset = 0;
      int YOffset = 0;
      while (Running) {
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          if (Message.message == WM_QUIT) {
            Running = FALSE;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }
        RenderWeirdGradient(XOffset, YOffset);
        HDC DeviceContext = GetDC(Window);
        RECT ClientRect;
        GetClientRect(Window, &ClientRect);
        int WindowWidth = ClientRect.right - ClientRect.left;
        int WindowHeight = ClientRect.bottom - ClientRect.top;
        Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth,
                          WindowHeight);
        ReleaseDC(Window, DeviceContext);
        XOffset++;
        YOffset++;
      }
    } else {
    }
  } else {
  }
  return (0);
}

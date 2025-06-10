#include <assert.h>
#include <stdint.h>
#include <windows.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

typedef struct {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int BytesPerPixel;
} win32_offscreen_buffer;

global_variable BOOL Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

typedef struct {
  int Width;
  int Height;
} win32_window_dimension;

#define X_INPUT_GET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_GET_STATE(XInputGetStateStub) { return 0; }
X_INPUT_SET_STATE(XInputSetStateStub) { return 0; }

global_variable x_input_get_state *DyXInputGetState = XInputGetStateStub;
global_variable x_input_set_state *DyXInputSetState = XInputSetStateStub;
#define XInputGetState DyXInputGetState
#define XInputSetState DyXInputSetState

internal void Win32LoadXInput(void) {
  HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
  if (XInputLibrary) {
    XInputGetState =
        (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState =
        (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

internal win32_window_dimension GetWindowDimension(HWND Window) {
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;
  return Result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset,
                                  int YOffset) {
  int Pitch = Buffer->Width * Buffer->BytesPerPixel;
  uint8_t *Row = (uint8_t *)Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; ++Y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < Buffer->Width; ++X) {
      uint8_t Blue = (X + XOffset);
      uint8_t Green = (Y + YOffset);
      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Pitch;
  }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer,
                                    win32_window_dimension WindowDimension) {
  if (Buffer->Memory) {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = WindowDimension.Width;
  Buffer->Height = WindowDimension.Height;
  Buffer->BytesPerPixel = 4;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int MemorySize = Buffer->BytesPerPixel * Buffer->Width * Buffer->Height;
  Buffer->Memory = VirtualAlloc(0, MemorySize, MEM_COMMIT, PAGE_READWRITE);
  assert(Buffer->Memory > 0);
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, win32_offscreen_buffer *Buffer,
                           win32_window_dimension WindowDimension) {
  StretchDIBits(DeviceContext, 0, 0, WindowDimension.Width,
                WindowDimension.Height, 0, 0, Buffer->Width, Buffer->Height,
                Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message,
                                             WPARAM WParam, LPARAM LParam) {
  LRESULT Result = 0;
  switch (Message) {
  case WM_SIZE: {
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
  case WM_SYSKEYDOWN: {

  } break;
  case WM_SYSKEYUP: {
    uint32_t VKCode = WParam;
    BOOL WasDown = ((LParam & (1 << 30)) != 0);
    BOOL IsDown = ((LParam & (1 << 31)) == 0);
    if (WasDown != IsDown) {
      if (VKCode == 'W') {
      } else if (VKCode == 'A') {
      } else if (VKCode == 'S') {
      } else if (VKCode == 'D') {
      } else if (VKCode == 'Q') {
      } else if (VKCode == 'E') {
      } else if (VKCode == VK_ESCAPE) {
        if (IsDown) {
          OutputDebugStringA("Is Down!");
        }
        if (WasDown) {
          OutputDebugStringA("Was Down!");
        }
      } else if (VKCode == VK_SPACE) {
      }
    }
  } break;
  case WM_KEYDOWN: {

    uint32_t VKCode = WParam;
    if (VKCode == 'W') {
    }
    // LParam & (1 << 30);
  } break;
  case WM_KEYUP: {

  } break;
  case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(Window, &Paint);
    win32_window_dimension WindowDimension;
    WindowDimension.Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
    WindowDimension.Width = Paint.rcPaint.right - Paint.rcPaint.left;
    Win32DisplayBufferInWindow(DeviceContext, &GlobalBackbuffer,
                               WindowDimension);
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
  Win32LoadXInput();

  WNDCLASSA WindowClass = {0};

  win32_window_dimension WindowDimension = {1280, 720};
  Win32ResizeDIBSection(&GlobalBackbuffer, WindowDimension);

  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "TruceEngineWindowClass";

  if (RegisterClassA(&WindowClass)) {
    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "TruceEngine",
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

        for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT;
             ++ControllerIndex) {
          XINPUT_STATE ControllerState;
          if (XInputGetState(ControllerIndex, &ControllerState) ==
              ERROR_SUCCESS) {
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
            BOOL Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            BOOL Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            BOOL Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            BOOL Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            BOOL Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            BOOL Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            BOOL LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            BOOL RightShoulder =
                (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            BOOL AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
            BOOL BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
            BOOL XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
            BOOL YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
            int16_t StickX = Pad->sThumbLX;
            int16_t StickY = Pad->sThumbLY;
          } else {
          }
        }

        RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
        HDC DeviceContext = GetDC(Window);
        win32_window_dimension WindowDimension = GetWindowDimension(Window);
        Win32DisplayBufferInWindow(DeviceContext, &GlobalBackbuffer,
                                   WindowDimension);
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

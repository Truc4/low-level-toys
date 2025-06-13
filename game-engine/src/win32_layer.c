#include "platform_layer.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <xaudio2.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

// Sound constants
#define BITSPERSSAMPLE 16
#define SAMPLESPERSEC 44100
#define CYCLESPERSEC 220
#define VOLUME 0.5
#define AUDIOBUFFERSIZEINCYCLES 10
#define PI 3.14159265358979323846

#define SAMPLESPERCYCLE ((int)(SAMPLESPERSEC / CYCLESPERSEC)) // 200
#define AUDIOBUFFERSIZEINSAMPLES                                               \
  (SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES) // 2000
#define AUDIOBUFFERSIZEINBYTES                                                 \
  ((AUDIOBUFFERSIZEINSAMPLES * BITSPERSSAMPLE) / 8) // 4000
// End sound

typedef struct {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int BytesPerPixel;
} win32_offscreen_buffer;

global_variable BOOL GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable byte GlobalSoundBuffer[AUDIOBUFFERSIZEINBYTES] = {0};

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

X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }

global_variable x_input_get_state *DyXInputGetState = XInputGetStateStub;
global_variable x_input_set_state *DyXInputSetState = XInputSetStateStub;
#define XInputGetState DyXInputGetState
#define XInputSetState DyXInputSetState

#define X_AUDIO2_CREATE(name)                                                  \
  HRESULT name(IXAudio2 **ppXAudio2, UINT32 Flags,                             \
               XAUDIO2_PROCESSOR XAudio2Processor)
// X_AUDIO2_CREATE(XAudioCreateStub) { return XAUDIO2_E_INVALID_CALL; }
typedef X_AUDIO2_CREATE(x_audio2_create);

loopCallbackFn platformLoopCallback = 0;

internal void Win32LoadXInput(void) {
  HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
  if (!XInputLibrary) {
    XInputLibrary = LoadLibrary("xinput1_3.dll");
  }
  if (XInputLibrary) {
    XInputGetState =
        (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState =
        (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

XAUDIO2_BUFFER xAudio2Buffer = {
    XAUDIO2_END_OF_STREAM,
    AUDIOBUFFERSIZEINBYTES,
    GlobalSoundBuffer,
    0,
    0,
    0,
    0,
    XAUDIO2_LOOP_INFINITE,
};

internal void Win32InitXAudio(void) {
  HMODULE XAudioLibrary = LoadLibraryA("xaudio2_9.dll");
  if (!XAudioLibrary) {
    printf("ERROR: Failed to load xaudio2_9.dll\n");
    return;
  }
  printf("INFO: Successfully loaded xaudio2_9.dll\n");

  x_audio2_create *XAudio2Create =
      (x_audio2_create *)GetProcAddress(XAudioLibrary, "XAudio2Create");
  if (!XAudio2Create) {
    printf("ERROR: Failed to get address of XAudio2Create\n");
    return;
  }
  printf("INFO: Successfully got XAudio2Create function\n");

  IXAudio2 *XAudio = 0;
  if (!SUCCEEDED(XAudio2Create(&XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
    printf("ERROR: Failed to create XAudio2 interface\n");
    return;
  }
  printf("INFO: XAudio2 interface created\n");

  IXAudio2MasteringVoice *MasteringVoice;
  if (!SUCCEEDED(XAudio->lpVtbl->CreateMasteringVoice(XAudio, &MasteringVoice,
                                                      0, 0, 0, 0, 0, 0))) {
    printf("ERROR: Failed to create mastering voice\n");
    return;
  }
  printf("INFO: Mastering voice created\n");

  // Define a format.
  WAVEFORMATEX waveFormatEx = {0};
  waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
  waveFormatEx.nChannels = 1; // 1 channel
  waveFormatEx.nSamplesPerSec = SAMPLESPERSEC;
  waveFormatEx.nBlockAlign = waveFormatEx.nChannels * BITSPERSSAMPLE / 8;
  waveFormatEx.nAvgBytesPerSec =
      waveFormatEx.nSamplesPerSec * waveFormatEx.nBlockAlign;
  waveFormatEx.wBitsPerSample = BITSPERSSAMPLE;
  waveFormatEx.cbSize = 0;

  // Create a source voice with that format.
  IXAudio2SourceVoice *XAudio2SourceVoice;
  HRESULT hr = XAudio->lpVtbl->CreateSourceVoice(
      XAudio, &XAudio2SourceVoice, &waveFormatEx, 0, 1.0, 0, 0, 0);
  if (!SUCCEEDED(hr)) {
    printf("ERROR: Failed to create source voice. HRESULT = 0x%08lX\n", hr);
    return;
  }
  printf("INFO: Source voice created\n");

  // Submit the buffer to the source voice.
  if (!SUCCEEDED(XAudio2SourceVoice->lpVtbl->SubmitSourceBuffer(
          XAudio2SourceVoice, &xAudio2Buffer, 0))) {
    printf("ERROR: Failed to submit source buffer\n");
    return;
  }
  printf("INFO: Source buffer submitted\n");

  // Start the voice.
  if (!SUCCEEDED(XAudio2SourceVoice->lpVtbl->Start(XAudio2SourceVoice, 0, 0))) {
    printf("ERROR: Failed to start source voice\n");
    return;
  }
  printf("INFO: Source voice started successfully\n");
}

internal win32_window_dimension GetWindowDimension(HWND Window) {
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;
  return Result;
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
    GlobalRunning = FALSE;
    OutputDebugStringA("WM_DESTROY\n");
  } break;

  case WM_CLOSE: {
    // TODO handle with a message to the user
    GlobalRunning = FALSE;
    OutputDebugStringA("WM_CLOSE\n");
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP: {
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
    BOOL AltKeyWasDown = ((LParam & (1 << 29)) != 0);
    if (VKCode == VK_F4 && AltKeyWasDown) {
      GlobalRunning = FALSE;
    }
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

void platformMain(loopCallbackFn loopCallback) {
  platformLoopCallback = loopCallback;

  HINSTANCE instance = GetModuleHandle(NULL);
  LPSTR commandLine = GetCommandLineA();
  int showCode = SW_SHOW;

  WinMain(instance, NULL, commandLine, showCode);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {
  LARGE_INTEGER PerfCountFrequency;
  QueryPerformanceFrequency(&PerfCountFrequency);

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
      Win32InitXAudio();
      BOOL SoundIsPlaying = FALSE;
      GlobalRunning = TRUE;

      LARGE_INTEGER LastCounter;
      QueryPerformanceCounter(&LastCounter);

      int64_t LastCycleCount = __rdtsc();

      while (GlobalRunning) {
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          if (Message.message == WM_QUIT) {
            GlobalRunning = FALSE;
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

        HDC DeviceContext = GetDC(Window);
        win32_window_dimension WindowDimension = GetWindowDimension(Window);
        Win32DisplayBufferInWindow(DeviceContext, &GlobalBackbuffer,
                                   WindowDimension);
        ReleaseDC(Window, DeviceContext);

        int64_t EndCycleCount = __rdtsc();

        LARGE_INTEGER EndCounter;
        QueryPerformanceCounter(&EndCounter);

        assert(platformLoopCallback);
        engine_offscreen_buffer Buffer = {
            GlobalBackbuffer.Memory,
            GlobalBackbuffer.Width,
            GlobalBackbuffer.Height,
            GlobalBackbuffer.BytesPerPixel,
        };

        engine_sound_buffer SoundBuffer = {GlobalSoundBuffer, 0};
        platformLoopCallback(&Buffer, &SoundBuffer);
        SoundIsPlaying = SoundBuffer.SoundPlaying;

        int64_t CyclesElapsed = EndCycleCount - LastCycleCount;

        int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
        int64_t MSPerFrame =
            ((1000 * CounterElapsed) / PerfCountFrequency.QuadPart);
        int32_t MCPerFrame = (int32_t)(CyclesElapsed / (1000 * 1000));
        // printf("ms elapsed: %lli, %i\n", MSPerFrame, MCPerFrame);
        LastCounter = EndCounter;
        LastCycleCount = EndCycleCount;
      }
    } else {
    }
  } else {
  }
  return (0);
}

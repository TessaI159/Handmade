#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer
{
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct win32_window_dimension
{
  int Width;
  int Height;
};


// TODO (Tess): This is a global for now.
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;


// NOTE (Tess): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
  return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE (Tess): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
  return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void)
{
  // TODO (Tess): Test on Windows Eight
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");

  if(!XInputLibrary)
    {
      HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
  if(XInputLibrary)
    {
      XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
      if(!XInputGetState) { XInputGetState = XInputGetStateStub; }
      XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
      if(!XInputSetState) { XInputSetState = XInputSetStateStub; }
    }
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID pcGuiDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void Win32InitDSound(HWND Window, int32 BufferSize, int32 SamplesPerSecond)
{
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
  if(DSoundLibrary)
    {
      direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

      // TODO (Tess): Double check if this works on xp - DirectSound 8 or 7??
      LPDIRECTSOUND DirectSound;
      if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
	{
	  WAVEFORMATEX WaveFormat = {};

	  WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	  WaveFormat.nChannels = 2;
	  WaveFormat.nSamplesPerSec = SamplesPerSecond;
	  WaveFormat.wBitsPerSample = 16;
	  WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
	  WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign*WaveFormat.nSamplesPerSec;
	  WaveFormat.cbSize = 0;

	  if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
	    {
	      DSBUFFERDESC BufferDescription = {};
	      BufferDescription.dwSize = sizeof(BufferDescription);
	      BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

	      // TODO (Tess): DSBCAPS_GLOBALFOCUS?
	      LPDIRECTSOUNDBUFFER PrimaryBuffer;
	      if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
		{
		  if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
		    {
		      OutputDebugStringA("Primary buffer format was set.\n");
		      // NOTE (Tess): We finally set the format of the buffer!
		    }
		  else
		    {
		      // TODO (Tess): Diagnostic
		    }
		}
	      else
		{
		  // TODO (Tess): Diagnostic
		}
	    }
	  else
	    {
	      // TODO (Tess): Diagnostic
	    }

	  // NOTE (Tess): "Create" a secondary buffer

	  // TODO (Tess): DSBCAPS_GETCURRENTPOSITION2
	  DSBUFFERDESC BufferDescription = {};
	  BufferDescription.dwSize = sizeof(BufferDescription);
	  BufferDescription.dwFlags = 0;
	  BufferDescription.dwBufferBytes = BufferSize;
	  BufferDescription.lpwfxFormat = &WaveFormat;
	  LPDIRECTSOUNDBUFFER SecondaryBuffer;

	  if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0)))
	    {
	      OutputDebugStringA("Secondary buffer created successfully!\n");
	      // NOTE (Tess): Start it playing!
	    }
	  else
	    {
	      // TODO (Tess): Diagnostics
	    }
	}
      else
	{
	  // TODO (Tess): Diagnostic
	}
    }

}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
  win32_window_dimension Result;

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return (Result);
}

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
  // TODO (Tess): Let's see what the optimizer does if passed by pointer

  uint8 *Row = (uint8 *) Buffer.Memory;
  for (int Y = 0; Y < Buffer.Height; ++Y)
    {
      uint32 *Pixel = (uint32 *) Row;
      for (int X = 0; X < Buffer.Width; ++X)
	{
	  uint8 Blue = (X + XOffset);
	  uint8 Red = (Y + YOffset);

	  *Pixel++ = ((Red << 16) | Blue);
	}
      Row += Buffer.Pitch;
    }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
  // TODO (Tess): Bulletproof this.
  // Maybe don't free first, free after then free first if that fails.

  if (Buffer->Memory)
    {
      VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

  Buffer->Width = Width;
  Buffer->Height = Height;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  // NOTE(Me): BiHeight being negative tells Windows to render top down
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  Buffer->BytesPerPixel = 4;
  int BitmapMemorySize = (Buffer->Width*Buffer->Height)*Buffer->BytesPerPixel;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

  // TODO (Tess): Probably clear this to black.
  Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, win32_offscreen_buffer *Buffer,
					 int WindowWidth, int WindowHeight) {
  // TODO (Tess): Aspect ratio correction
  StretchDIBits(DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;

  switch (Message)
    {
    case WM_DESTROY:
      {
	// TODO (Tess): Handle this as an error - Recreate window?
	GlobalRunning = false;
      } break;
    case WM_CLOSE:
      {
	// TODO (Tess): Handle this with a message to the user?
	GlobalRunning = false;
      } break;
    case WM_ACTIVATEAPP:
      {
	OutputDebugStringA("WM_ACTIVATEAPP\n");
      } break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
      {
	uint32 VKCode = WParam;
	bool WasDown = ((LParam & (1 << 30)) != 0);
	bool IsDown = ((LParam & (1 << 31)) == 0);

	if(WasDown != IsDown)
	  {
	    if(VKCode == 'W')
	      {
	      }
	    else if(VKCode == 'A')
	      {
	      }
	    else if(VKCode == 'S')
	      {
	      }
	    else if(VKCode == 'D')
	      {
	      }
	    else if(VKCode == 'Q')
	      {
	      }
	    else if(VKCode == 'E')
	      {
	      }
	    else if(VKCode == VK_UP)
	      {
	      }
	    else if(VKCode == VK_DOWN)
	      {
	      }
	    else if(VKCode == VK_LEFT)
	      {
	      }
	    else if(VKCode == VK_RIGHT)
	      {
	      }
	    else if(VKCode == VK_ESCAPE)
	      {
		OutputDebugStringA("ESCAPE: ");
		if(IsDown)
		  {
		    OutputDebugStringA("IsDown\n");
		  }
		if(WasDown)
		  {
		    OutputDebugStringA("WasDown\n");
		  }
	      }
	    else if(VKCode == VK_SPACE)
	      {
	      }
	  }
	bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);
	if((VKCode = VK_F4) && AltKeyWasDown)
	  {
	    GlobalRunning = false;
	  }}
      break;
    case WM_PAINT:
      {
	PAINTSTRUCT Paint;
	HDC DeviceContext = BeginPaint(Window, &Paint);
	int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
	int Width = Paint.rcPaint.right - Paint.rcPaint.left;

	win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32DisplayBufferInWindow(DeviceContext, &GlobalBackbuffer,
				   Dimension.Width, Dimension.Height);
	EndPaint(Window, &Paint);
      } break;
    default:
      {
	Result = DefWindowProc(Window, Message, WParam, LParam);
      } break;
    }
  return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine,
		     int ShowCode)
{

  WNDCLASSA WindowClass = {};

  Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if (RegisterClass(&WindowClass))
    {
      HWND Window =
	CreateWindowExA(0, WindowClass.lpszClassName, "Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
      if (Window)
	{
	  Win32InitDSound(Window, 48000, 48000*sizeof(int16)*2);
	  // NOTE(Me): Since we specified CS_OWNDC, we can just get the DC once since we are not sharing it.
	  HDC DeviceContext = GetDC(Window);

	  int XOffset = 0;
	  int YOffset = 0;

	  GlobalRunning = true;
	  while (GlobalRunning)
	    {
	      MSG Message;
	      while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
		  if (Message.message == WM_QUIT)
		    {
		      GlobalRunning = false;
		    }
		  TranslateMessage(&Message);
		  DispatchMessage(&Message);
		}

	      // TODO (Tess): Should we poll this more frequently?
	      for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
		{
		  XINPUT_STATE ControllerState;
		  if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		    {
		      // NOTE (Tess): Controller is connected
		      // TODO (Tess): See if ControllerState.dwPacketNumber increments too rapidly
		      XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

		      bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
		      bool DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
		      bool DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
		      bool DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
		      bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
		      bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
		      bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
		      bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
		      bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
		      bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
		      bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
		      bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

		      int16 StickX = Pad->sThumbLX;
		      int16 StickY = Pad->sThumbLY;


		      XOffset += StickX >> 12;
		      YOffset += StickY >> 12;
		    }
		  else
		    {
		      // NOTE (Tess): Controller is unavailable
		    }
		}


	      RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

	      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	      Win32DisplayBufferInWindow(DeviceContext, &GlobalBackbuffer, Dimension.Width, Dimension.Height);

	    }
	} else
	{
	  // TODO (Tess): Logging
	}
    } else
    {
      // TODO (Tess): Logging
    }

  return (0);
}


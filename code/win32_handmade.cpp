#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

//TODO(Me): This is a global for now.
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

internal void
Win32ResizeDIBSection(int Width, int Height)
{

  //TODO (Me): Bulletproof this.
  // Maybe don't free first, free after then free first if that fails.

  if(BitmapHandle)
  {
    DeleteObject(BitmapHandle);
  }
  if(!BitmapDeviceContext)
  {
    //TODO(Me): Should we recreate these under special circumstances?
    BitmapDeviceContext = CreateCompatibleDC(0);
  }

  BITMAPINFO BitmapInfo;
  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;
  BitmapInfo.bmiHeader.biSizeImage = 0;
  BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
  BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
  BitmapInfo.bmiHeader.biClrUsed = 0;
  BitmapInfo.bmiHeader.biClrImportant = 0;

  HBITMAP BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
  StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height,
    BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;

  switch(Message)
  {
    case WM_SIZE:
    {
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(Width, Height);
    } break;
    case WM_DESTROY:
    {
      //TODO(Me): Handle this as an error - Recreate window?
      Running = false;
    } break;
    case WM_CLOSE:
    {
      //TODO(Me): Handle this with a message to the user?
      Running = false;
    } break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);
        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;
        Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
      EndPaint(Window, &Paint);
    }
    default:
    {
      OutputDebugStringA("DEFAULT\n");
      Result = DefWindowProc(Window, Message, WParam, LParam);
    } break;
  }
  return Result;
}


int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine,
  int ShowCode)
  {
    WNDCLASS WindowClass = {};
      //TODO(Me): Check if HREDRAW AND VREDRAW are needed
      WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
      WindowClass.lpfnWndProc = Win32MainWindowCallback;
      WindowClass.hInstance = Instance;
      // WindowClass.hIcon;
      WindowClass.lpszClassName = "HandmadeHeroWindowClass";

      if(RegisterClass(&WindowClass))
      {
        HWND WindowHandle =
        CreateWindowExA(0, WindowClass.lpszClassName, "Handmade Hero",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
        if(WindowHandle)
        {
          Running = true;
          while(Running)
          {
            MSG Message;
            BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
            if(MessageResult > 0)
            {
              TranslateMessage(&Message);
              DispatchMessage(&Message);
            }
            else
            {
              break;
            }
          }
        }
        else
        {
          //TODO(Me): Logging
        }
      }
      else
      {
        //TODO(Me): Logging
      }

    return(0);
  }

#include "framework.h"
#include "Mouse_Hook.h"

/*
  * 
  * The code is a simple Windows application that intercepts and handles mouse events at a low level. 
  * It creates the main application window with "MS LL start" and "MS LL stop" buttons to activate and deactivate mouse capture. 
  * When mouse interception is activated, the application responds to various mouse events such as movement, clicks, and 
  * wheel scrolling and displays relevant information on the screen.
  *
  *    Main components of the code:
  * 1. Definition of constants and global variables.
  * 2. Registering a custom window class.
  * 3. Initialization and creation of the main application window.
  * 4. Message handler of the main window.
  * 5. Functions for activating and deactivating mouse interception.
  * 6. Function-interceptor of mouse events at a low level.
  *
  * The code also contains functions for displaying information on the screen, creating and
  * managing a Device Context, drawing on a Device Context (eg, lines, ellipses, rectangles),
  * and manipulating memory for storing images. 
  *
*/

#define MAX_LOADSTRING 100
#define CMD_MS_LL_START 1005
#define CMD_MS_LL_STOP 1006
#define MS_OFFSET_X 5
#define MS_OFFSET_Y 5
#define MS_SCALE_X 5
#define MS_SCALE_Y 5
#define BUTTON_CLICK_SIZE 10

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND list;
HHOOK kbLL;
HHOOK msLL;
WCHAR str[MAX_LOADSTRING];
HWND msTrace;
HDC msDC, msTraceDC, memoryDC;
HBITMAP memoryPicture;
HPEN pen, fatPen, leftClick_pen, rightClick_pen;
HBRUSH leftClick_brush, rightClick_brush;
BOOL firstMove;
BOOL isLeftHold;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
DWORD CALLBACK StartMsHookLL(LPVOID);
DWORD CALLBACK StopMsHookLL(LPVOID);
LRESULT CALLBACK MsHookProcLL(int, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{              // The entry point to a Windows program.
              // Provides the core structure of a Windows application, including window creation and
             // handling, resource loading, and the main message loop.
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MOUSEHOOK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MOUSEHOOK));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)  // Registering a custom window class, including window handler, icons, cursors.
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_MOUSEHOOK));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(1000, 1000, 1000));
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MOUSEHOOK);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)  // Initializes an instance of the application and creates the main window.
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), 50, 50, 750, 310, nullptr, nullptr, hInstance, nullptr);
    RECT rect;
    msDC = GetDC(hWnd);
    memoryDC = CreateCompatibleDC(msDC);
    GetClientRect(hWnd, &rect);

    memoryPicture = CreateCompatibleBitmap(memoryDC, rect.right - rect.left, rect.bottom - rect.top);

    SelectObject(memoryDC, memoryPicture);

    HBRUSH bg = CreateSolidBrush(GetBkColor(msDC));

    FillRect(memoryDC, &rect, bg);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)  // Message handler for the application's main window and 
{                                                                               // defines the application's behavior in response to various messages and 
                                                                               // commands associated with the window and its interface components.
    switch (message)
    {
    case WM_CREATE:
    {
        kbLL = 0;
        pen = CreatePen(PS_SOLID, 2, RGB(140, 250, 250));
        fatPen = CreatePen(PS_SOLID, 10, RGB(200, 100, 250));
        leftClick_pen = CreatePen(PS_ALTERNATE, 5, RGB(250, 200, 100));
        rightClick_pen = CreatePen(PS_ALTERNATE, 5, RGB(350, 220, 240));
        leftClick_brush = CreateSolidBrush(RGB(250, 200, 600));
        rightClick_brush = CreateSolidBrush(RGB(150, 210, 290));
        list = CreateWindowW(L"Listbox", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL, 105, 10, 200, 226, hWnd, 0, hInst, NULL);


        CreateWindowW(L"Button", L"MS LL start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 90, 90, 23, hWnd, (HMENU)CMD_MS_LL_START, hInst, NULL);
        CreateWindowW(L"Button", L"MS LL stop", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 120, 90, 23, hWnd, (HMENU)CMD_MS_LL_STOP, hInst, NULL);

        msTrace = CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME, 320, 10, 394, 226, hWnd, (HMENU)CMD_MS_LL_STOP, hInst, NULL);
        msLL = 0;
        msTraceDC = GetDC(msTrace);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        switch (wmId)
        {
        case CMD_MS_LL_START:
            StartMsHookLL(NULL);
            break;
        case CMD_MS_LL_STOP:
            StopMsHookLL(NULL);
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT r;

        GetClientRect(msTrace, &r);
        BitBlt(msTraceDC, 0, 0, r.right - r.left, r.bottom - r.top, memoryDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        DeleteObject(pen);
        DeleteObject(fatPen);
        DeleteObject(leftClick_brush);
        DeleteObject(rightClick_brush);
        ReleaseDC(hWnd, msDC);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)  // Allows you to process messages related to initialization and 
{                                                                             // commands that appear in this dialog box.
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

DWORD CALLBACK StartMsHookLL(LPVOID)  // Run a global interception of a low-level mouse hook, check its status, and 
{                                    // update the corresponding information in the user interface.
    if (msLL != 0)
        _snwprintf_s(str, MAX_LOADSTRING, L"MS hook works");
    else
    {
        msLL = SetWindowsHookExW(WH_MOUSE_LL, MsHookProcLL, GetModuleHandle(NULL), 0);

        if (msLL != 0)
        {
            firstMove = TRUE;

            _snwprintf_s(str, MAX_LOADSTRING, L"MS hook activated");
        }
        else
            _snwprintf_s(str, MAX_LOADSTRING, L"MS hook activation failed");
    }
    SendMessageW(list, LB_ADDSTRING, 100, (LPARAM)str);
    return 0;
}

DWORD CALLBACK StopMsHookLL(LPVOID)  // Stop the global interception of the low-level mouse hook and 
{                                   // update the corresponding information in the user interface.
    if (msLL != 0)
    {
        UnhookWindowsHookEx(msLL);

        msLL = 0;

        _snwprintf_s(str, MAX_LOADSTRING, L"MS hook released");
    }
    else
        _snwprintf_s(str, MAX_LOADSTRING, L"MS hook is not active");

    SendMessageW(list, LB_ADDSTRING, 100, (LPARAM)str);
    return 0;
}

LRESULT CALLBACK MsHookProcLL(int nCode, WPARAM wParam, LPARAM lParam)  // The MsHookProcLL function intercepts and processes mouse messages 
{                                                                      // and renders user actions on the msTraceDC and memoryDC device context.
    if (nCode == HC_ACTION)
    {
        MOUSEHOOKSTRUCT msInfo = *((MOUSEHOOKSTRUCT*)lParam);
        MSLLHOOKSTRUCT msLLInfo = *((MSLLHOOKSTRUCT*)lParam);

        switch (wParam)
        {
        case WM_MOUSEWHEEL:
        {
            _snwprintf_s(str, MAX_LOADSTRING, L"wheel1 %d", (short)HIWORD(msLLInfo.mouseData));
            SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)str);

            if ((short)HIWORD(msLLInfo.mouseData) > 0)
            {
                AngleArc(msTraceDC, msInfo.pt.x / MS_SCALE_X, msInfo.pt.y / MS_SCALE_Y, 12, 0, 180);
                AngleArc(memoryDC, msInfo.pt.x / MS_SCALE_X, msInfo.pt.y / MS_SCALE_Y, 12, 0, 180);
            }
            else
            {
                AngleArc(msTraceDC, msInfo.pt.x / MS_SCALE_X, msInfo.pt.y / MS_SCALE_Y, 12, 0, -180);
                AngleArc(memoryDC, msInfo.pt.x / MS_SCALE_X, msInfo.pt.y / MS_SCALE_Y, 12, 0, -180);
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            isLeftHold = TRUE;

            SelectObject(msTraceDC, leftClick_brush);
            SelectObject(memoryDC, leftClick_brush);
            _snwprintf_s(str, MAX_LOADSTRING, L"x:%d y:%d", msInfo.pt.x, msInfo.pt.y);

            SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)str);

            Ellipse(msTraceDC, MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X - BUTTON_CLICK_SIZE / 2, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y - BUTTON_CLICK_SIZE / 2,
                MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X + BUTTON_CLICK_SIZE / 2, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y + BUTTON_CLICK_SIZE / 2);
            Ellipse(memoryDC, MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X - BUTTON_CLICK_SIZE / 2, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y - BUTTON_CLICK_SIZE / 2,
                MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X + BUTTON_CLICK_SIZE / 2, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y + BUTTON_CLICK_SIZE / 2);
            break;
        }
        case WM_LBUTTONUP:
            isLeftHold = FALSE;
            break;
        case WM_RBUTTONDOWN:
            SelectObject(msTraceDC, rightClick_brush);
            SelectObject(memoryDC, rightClick_brush);
            _snwprintf_s(str, MAX_LOADSTRING, L"x:%d y:%d", msInfo.pt.x, msInfo.pt.y);

            SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)str);

            Rectangle(msTraceDC, MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X - BUTTON_CLICK_SIZE / 2, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y - BUTTON_CLICK_SIZE / 2,
                MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X + BUTTON_CLICK_SIZE / 2, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y + BUTTON_CLICK_SIZE / 2);
            Rectangle(memoryDC, MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X - BUTTON_CLICK_SIZE / 2, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y - BUTTON_CLICK_SIZE / 2,
                MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X + BUTTON_CLICK_SIZE / 2, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y + BUTTON_CLICK_SIZE / 2);
            break;
        case WM_MOUSEMOVE:
            if (isLeftHold)
            {
                SelectObject(msTraceDC, fatPen);
                SelectObject(memoryDC, fatPen);
            }
            else
            {
                SelectObject(msTraceDC, pen);
                SelectObject(memoryDC, pen);
            }
            if (firstMove)
            {
                MoveToEx(msTraceDC, MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y, NULL);
                MoveToEx(memoryDC, MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y, NULL);

                firstMove = FALSE;
            }
            else
            {
                LineTo(msTraceDC, MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y);
                LineTo(memoryDC, MS_OFFSET_X + msInfo.pt.x / MS_SCALE_X, MS_OFFSET_Y + msInfo.pt.y / MS_SCALE_Y);
            }
            break;
        default:
            break;
        }
    }
    return CallNextHookEx(kbLL, nCode, wParam, lParam);
}


            //_________________¶¶111111¶11111111111111¶¶111¶____
            //_________________¶¶11111¶111111111111111¶¶111¶____
            //________________¶¶111111¶11111111¶¶11¶¶¶¶1111¶____
            //_______________¶¶1111111111111111¶¶¶¶¶¶¶11111¶____
            //_____________1¶¶11111111111111111¶¶___¶¶11111¶____
            //___________¶¶¶¶11111¶1111111¶¶1111¶¶¶11¶11111¶____
            //_________¶¶¶111111111111111111111111¶¶¶¶11111¶____
            //_______1¶¶11111111111111111111111111111¶¶¶111¶____
            //______¶¶111111111111111111111111111111111¶¶¶1¶____
            //_____¶¶1111111111111111¶11¶¶111111111111111¶¶¶____
            //____¶¶11111111111111111¶¶¶¶11111111111111111¶¶____
            //___¶¶1111111111111111111¶¶1111111111111111111¶¶___
            //__¶¶11111111111111111111¶¶11111111111111111111¶¶__
            //__¶111111111111111111111¶1111111111111111111111¶__
            //_¶¶11111111111111111111¶¶1111111111111111111111¶¶_
            //_¶111111111111111111111¶¶1111111111111111111111¶¶_
            //_¶111111111111111111111¶¶1111111111111111111111¶¶_
            //1¶111111111111111111111¶¶1111111111111111111111¶¶_
            //1¶111111111111111111111¶¶1111111111111111111111¶¶_
            //1¶111111111111111111111¶¶1111111111111111111111¶1_
            //_¶1111111111111111111111¶¶11111111111111111111¶¶__
            //_¶1111111111111111111111¶¶11111111111111111111¶1__
            //_¶111111111111111111111¶¶¶¶111111111111111111¶¶___
            //_¶¶1111111111111111111¶¶1¶¶¶¶111111111111111¶¶____
            //_1¶1111111111111111¶¶¶¶¶¶¶¶¶¶¶¶111111111111¶¶1____
            //__¶11111111111¶¶¶¶¶¶¶¶¶¶¶_¶¶¶¶¶¶¶¶¶¶¶11111¶¶¶1____
            //__¶¶1111111111¶¶¶¶11111¶¶_¶¶1111¶¶¶¶1111¶¶¶1¶¶____
            //__1¶¶¶11111111111111111¶¶_¶1¶1111111111¶¶¶11¶¶____
            //___¶¶¶¶¶1111111111111111¶¶¶¶1111111111¶¶_¶_1¶¶____
            //___1¶111¶¶¶11111111111111¶¶¶¶111111¶1¶¶_1¶_11¶____
            //____¶¶1111¶¶¶111111111111¶¶¶1111111¶¶¶_¶¶1111¶____
            //____1¶111111¶¶¶1111111111¶1¶¶1111111¶¶¶11¶11¶¶____
            //_____¶¶1111111¶¶¶¶1111111¶¶¶¶111111111¶1¶¶_1¶¶____
            //_____1¶1111111111¶¶¶¶1111¶¶1¶¶11111111¶¶¶¶_1¶1____
            //______¶¶11111111111¶¶¶¶111¶1¶¶111111111¶¶¶__¶1____
            //______1¶11111111111111¶¶¶¶¶_1¶1111111111¶¶111¶____
            //_______¶¶111111111111111¶¶¶1¶¶1111111111¶¶¶¶¶¶1___
            //________¶111111111111111¶¶__1¶¶¶11111111¶¶¶1¶¶¶___
            //________¶¶111111111111111¶1__1¶1¶¶¶¶¶1111¶¶¶¶¶¶1__
            //_________¶111111111111111¶¶___¶111¶¶¶¶¶¶¶¶¶¶_1¶___
            //_________¶¶11111111111111¶¶___¶¶1111111¶¶¶____¶___
            //__________¶1111111111111¶¶_____¶111111111¶________
            //__________¶¶111111111111¶¶_____¶¶1111111¶¶________
            //___________¶111111111111¶1______¶1111111¶¶________
            //___________¶¶11111111111¶_______¶¶111111¶¶________
            //____________¶1111111111¶¶________¶111111¶¶________
            //____________¶¶111111111¶¶________¶¶11111¶¶________
            //____________1¶111111111¶¶_________¶11111¶¶________
            //_____________¶¶11111111¶1_________¶¶1111¶¶________
            //_____________¶¶111¶1111¶__________1¶1111¶¶________
            //______________¶111¶¶111¶¶__________¶¶1111¶________
            //______________¶111¶¶111¶¶__________¶¶1111¶________
            //______________¶11111111¶1__________1¶1¶¶1¶¶_______
            //_____________¶¶11111111¶____________¶1¶¶11¶_______
            //____________1¶111111111¶____________¶11__1¶_______
            //____________¶¶11111111¶¶____________¶¶¶¶¶¶¶_______
            //____________¶111111111¶¶____________¶¶¶¶¶¶¶_______
            //____________¶1111111111¶____________¶¶¶¶¶¶¶¶______
            //___________1¶1111111111¶____________¶¶¶¶¶¶¶¶______
            //___________1¶1111111111¶1___________¶¶¶¶¶¶¶¶1_____
            //____________¶1111111111¶¶___________1¶¶¶¶¶¶¶______
            //____________¶1111111111¶¶____________¶¶¶¶¶¶¶______
            //____________¶¶111111111¶¶____________¶¶¶¶¶¶¶¶_____
            //____________1¶1111111111¶_____________¶¶¶¶¶¶¶_____
            //_____________¶1111111111¶_____________¶¶¶¶¶¶¶¶____
            //_____________¶¶111111111¶_____________1¶¶¶¶¶¶¶¶___
            //______________¶111111111¶1____________¶¶¶¶¶¶¶¶¶¶__
            //______________¶¶11111111¶1____________¶¶¶¶¶¶¶¶¶¶__
            //_______________¶11111111¶¶___________¶¶¶¶¶¶¶¶¶¶1__
            //_______________¶¶1111111¶¶___________¶¶¶¶¶¶¶¶¶¶___
            //________________¶¶111111¶¶____________¶¶¶¶¶¶¶¶____
            //_________________¶111111¶¶_____________¶¶¶¶¶¶_____

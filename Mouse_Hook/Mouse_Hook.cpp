#include "framework.h"
#include "Mouse_Hook.h"
#include <fstream>

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
{
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

ATOM MyRegisterClass(HINSTANCE hInstance)
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

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
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

DWORD CALLBACK StartMsHookLL(LPVOID)
{
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

DWORD CALLBACK StopMsHookLL(LPVOID)
{
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

LRESULT CALLBACK MsHookProcLL(int nCode, WPARAM wParam, LPARAM lParam)
{
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
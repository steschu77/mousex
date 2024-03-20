#include "mousex.h"
#include <math.h>
#include <inttypes.h>
#include <string>
#include <windowsx.h>

// ----------------------------------------------------------------------------
static HINSTANCE ghInstance = 0;
static const char* gWinTitle = "Keyboard & Mouse Xtensions";
static const char* gWinClassName = "KeyMouseXStats";
static const char* szRegKey = "Software\\MouseX";
static const UINT IDI_ICON = 3001;

HHOOK WinStats::_hookMouse = nullptr;
__int64 WinStats::_mouseDistance = 0;
__int64 WinStats::_mouseWheel = 0;
HHOOK WinStats::_hookKb = nullptr;
__int64 WinStats::_keyStrokes = 0;
POINT WinStats::_mouseGlobalPos = { 0, 0 };
POINT WinStats::_mouseClientPos = { 0, 0 };

// ----------------------------------------------------------------------------
void TaskBarAddIcon(HWND hwnd, HICON hicon, const char* pszText)
{
  NOTIFYICONDATA nid = { 0 };
  nid.cbSize = sizeof(NOTIFYICONDATA);
  nid.hWnd = hwnd;
  nid.uID = IDI_ICON;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_USER;
  nid.hIcon = hicon;
  strncpy_s(nid.szTip, sizeof(nid.szTip), pszText, _TRUNCATE);

  Shell_NotifyIcon(NIM_ADD, &nid);
}

// ----------------------------------------------------------------------------
void TaskBarDeleteIcon(HWND hwnd)
{
  NOTIFYICONDATA nid = { 0 };
  nid.cbSize = sizeof(NOTIFYICONDATA);
  nid.hWnd = hwnd;
  nid.uID = IDI_ICON;

  Shell_NotifyIcon(NIM_DELETE, &nid);
}

// ----------------------------------------------------------------------------
LRESULT SendKey(KBDLLHOOKSTRUCT* pHook, WORD vkCode)
{
  INPUT input = { 0 };
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = vkCode;
  input.ki.wScan = static_cast<WORD>(pHook->scanCode);
  input.ki.dwFlags = pHook->flags;
  input.ki.time = pHook->time;
  input.ki.dwExtraInfo = pHook->dwExtraInfo;
  SendInput(1, &input, sizeof(input));
  return 1;
}

// ----------------------------------------------------------------------------
void RegWriteInt64(HKEY hKey, const char* Name, __int64 Value)
{
  RegSetValueEx(hKey, Name, 0, REG_QWORD, (PBYTE)&Value, 8);
}

// ----------------------------------------------------------------------------
__int64 RegReadInt64(HKEY hKey, const char* Name)
{
  DWORD dwSize = 8;
  __int64 qwData = 0;
  int res = RegQueryValueEx(hKey, Name, 0, NULL, (LPBYTE)&qwData, &dwSize);

  return (res == ERROR_SUCCESS) ? qwData : 0;
}

// ----------------------------------------------------------------------------
static const char* gNameMouseDistance = "MouseDistance";
static const char* gNameMouseWheel = "MouseWheelRotations";
static const char* gNameKeyStrokes = "KeyStrokes";

// ----------------------------------------------------------------------------
int WinStats::saveState() const
{
  HKEY hKey = nullptr;
  ULONG rc = 0;
  RegCreateKeyEx(HKEY_CURRENT_USER, szRegKey, 0, "", 0,
    KEY_ALL_ACCESS, NULL, &hKey, &rc);

  if  (rc != REG_OPENED_EXISTING_KEY
    && rc != REG_CREATED_NEW_KEY
    && rc != ERROR_SUCCESS)
  {
    MessageBox(0, "Creating registry key failed.", "Error", MB_OK);
    return -1;
  }

  RegWriteInt64(hKey, gNameMouseDistance, _mouseDistance);
  RegWriteInt64(hKey, gNameMouseWheel, _mouseWheel);
  RegWriteInt64(hKey, gNameKeyStrokes, _keyStrokes);
  return 0;
}

// ----------------------------------------------------------------------------
int WinStats::restoreState()
{
  HKEY hKey = nullptr;
  int nResult = RegOpenKeyEx(HKEY_CURRENT_USER, szRegKey, 0, KEY_READ, &hKey);
  if (nResult != ERROR_SUCCESS)
  {
    MessageBox(0, "Opening registry key failed.", "Error", MB_OK);
    return -1;
  }

  _mouseDistance = RegReadInt64(hKey, gNameMouseDistance);
  _mouseWheel = RegReadInt64(hKey, gNameMouseWheel);
  _keyStrokes = RegReadInt64(hKey, gNameKeyStrokes);
  return 0;
}

// ----------------------------------------------------------------------------
WinStatsFactory::WinStatsFactory()
{
  _icon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_MOUSE));
  _menu = LoadMenu(ghInstance, MAKEINTRESOURCE(IDM_MOUSE));
  _registerClass(gWinClassName, _icon);
}

// ----------------------------------------------------------------------------
constexpr int to_dpi(int val, int dpi) {
  return val * dpi / 96;
}

// ----------------------------------------------------------------------------
int WinStatsFactory::createWindow(WinStats** ppWin)
{
  WinStats* pWin = new WinStats(this);

  HDC hdc = GetDC(NULL);
  int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
  ReleaseDC(NULL, hdc);

  // Create a main window for this application instance.
  HWND hwnd = CreateWindowEx(0L, gWinClassName, gWinTitle,
    WS_MINIMIZEBOX | WS_SYSMENU, CW_USEDEFAULT, 0,
    to_dpi(400, dpi), to_dpi(150, dpi), nullptr, nullptr, ghInstance, pWin);

  if (hwnd == nullptr) {
    return -1;
  }

  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  *ppWin = pWin;
  return 0;
}

// ----------------------------------------------------------------------------
void WinStatsFactory::_registerClass(const char* ClassName, HICON hIcon)
{
  WNDCLASSEX wcex = { 0 };
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.lpfnWndProc = (WNDPROC)__procStatic1;
  wcex.hInstance = ghInstance;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hIcon = hIcon;
  wcex.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
  wcex.lpszClassName = ClassName;

  RegisterClassEx(&wcex);
}

// ----------------------------------------------------------------------------
// Window Proc handler until WM_CREATE with create params is received
LRESULT CALLBACK WinStatsFactory::__procStatic1(
  HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_CREATE) {
    CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
    WinStats* pHandler = (cs != nullptr) ? static_cast<WinStats*>(cs->lpCreateParams) : nullptr;

    if (pHandler == nullptr) {
      return 0;
    }

    pHandler->_hwnd = hwnd;
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(__procStaticN));
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pHandler));
    return pHandler->proc(hwnd, msg, wParam, lParam);
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ----------------------------------------------------------------------------
// Window Proc handler with initialized user data
LRESULT CALLBACK WinStatsFactory::__procStaticN(
  HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  WinStats* pWin = reinterpret_cast<WinStats*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  return pWin->proc(hwnd, msg, wParam, lParam);
}


// ----------------------------------------------------------------------------
WinStats::WinStats(WinStatsFactory* fab)
: _fab(fab)
{
}

// ----------------------------------------------------------------------------
LRESULT WinStats::proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
  case WM_CREATE:
    return onCreate();

  case WM_DESTROY:
    return onDestroy();

  case WM_CLOSE:
    return onClose();

  case WM_QUERYENDSESSION:
    return onQueryEndSession();

  case WM_TIMER:
    return onTimer();

  case WM_SHOWWINDOW:
    return onShowWindow(wParam != 0);

  case WM_COMMAND:
    return onCommand((HWND)lParam, HIWORD(wParam), LOWORD(wParam));

  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    LRESULT res = onPaint(hdc, &ps);
    EndPaint(hwnd, &ps);
    return res;
  }

  case WM_USER:
    return onTaskbarNotify(lParam, wParam);
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ----------------------------------------------------------------------------
LRESULT WinStats::onCreate()
{
  TaskBarAddIcon(_hwnd, _fab->appIcon(), gWinTitle);

  restoreState();

  GetCursorPos(&_mouseGlobalPos);

  _hookMouse = SetWindowsHookEx(WH_MOUSE_LL, procMouse, ghInstance, 0);
  _hookKb = SetWindowsHookEx(WH_KEYBOARD_LL, procKeyboard, ghInstance, 0);

  return 0;
}

// ----------------------------------------------------------------------------
LRESULT WinStats::onDestroy()
{
  UnhookWindowsHookEx(_hookMouse);
  UnhookWindowsHookEx(_hookKb);

  saveState();

  TaskBarDeleteIcon(_hwnd);
  PostQuitMessage(1);
  return 0;
}

// --------------------------------------------------------------------------
LRESULT WinStats::onClose()
{
  ShowWindow(_hwnd, SW_HIDE);
  return 0;
}

// --------------------------------------------------------------------------
LRESULT WinStats::onQueryEndSession()
{
  DestroyWindow(_hwnd);
  return TRUE;
}

// --------------------------------------------------------------------------
LRESULT WinStats::onTimer()
{
  InvalidateRect(_hwnd, nullptr, FALSE);
  return 0;
}

// --------------------------------------------------------------------------
LRESULT WinStats::onShowWindow(bool show)
{
  if (show) {
    SetTimer(_hwnd, 0, 100, 0);
  } else {
    KillTimer(_hwnd, 0);
  }
  return 0;
}

// ----------------------------------------------------------------------------
LRESULT WinStats::onCommand(HWND hwndCtrl, WORD wCode, WORD wID)
{
  switch (wID)
  {
  case IDM_QUIT:
    DestroyWindow(_hwnd);
    break;

  case IDM_SHOW:
    ShowWindow(_hwnd, SW_SHOW);
    break;

  case IDM_ABOUT:
    MessageBox(_hwnd, "Keystroke Counter,\nMouse-Distance Counter,\nMouse Wheel Counter,\nWheel Controls Window under Mouse,\nLeft-Double-Click Simulation using Middle Mouse Button,\nMouse Button Anti-Contact Bounce,\nESC+Up/Down translates to Page Up/Down\n"
      "\n© Copyright 2001-2024\n", gWinTitle,
      MB_ICONINFORMATION | MB_OK);
    break;
  }
  return 0;
}

// --------------------------------------------------------------------------
LRESULT WinStats::onPaint(HDC hdc, const PAINTSTRUCT* pPS)
{
  FillRect(hdc, &pPS->rcPaint, GetSysColorBrush(COLOR_WINDOW));

  // Set font size based on the scaling factor
  int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
  HFONT hfnt = CreateFont(
    -to_dpi(12, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial")
  );

  HGDIOBJ hgdiFont = SelectObject(hdc, hfnt);

  constexpr int stat_count = 5;
  char szText[stat_count][128];
  const char* pszTitle[stat_count] = {
    "Distance:",
    "Wheel Rotations:",
    "Key Strokes:",
    "Global Pos:",
    "Client Pos:",
  };

  sprintf_s(szText[0], sizeof(szText[0]), "%" PRId64, _mouseDistance / 256);
  sprintf_s(szText[1], "%" PRId64, _mouseWheel / WHEEL_DELTA);
  sprintf_s(szText[2], "%" PRId64, _keyStrokes);
  sprintf_s(szText[3], "%d, %4d", (int)_mouseGlobalPos.x, (int)_mouseGlobalPos.y);
  sprintf_s(szText[4], "%d, %4d", (int)_mouseClientPos.x, (int)_mouseClientPos.y);

  RECT rcClient;
  GetClientRect(_hwnd, &rcClient);
  const int h = rcClient.bottom / stat_count;

  for (int i = 0; i < stat_count; i++)
  {
    RECT rcText = { 10, i * h, rcClient.right - 10, (i + 1) * h };
    DrawText(hdc, pszTitle[i], -1, &rcText, DT_LEFT);
    DrawText(hdc, szText[i], -1, &rcText, DT_RIGHT);
  }

  SelectObject(hdc, hgdiFont);
  DeleteObject(hfnt);
  return 0;
}

// ----------------------------------------------------------------------------
LRESULT WinStats::onTaskbarNotify(LPARAM lParam, WPARAM wParam)
{
  if (lParam == WM_LBUTTONDBLCLK) {
    onCommand(nullptr, 0, IDM_SHOW);
  }

  if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP) {
    POINT ptCursor;
    GetCursorPos(&ptCursor);

    HMENU hMenu = GetSubMenu(_fab->appMenu(), 0);
    TrackPopupMenu(hMenu, 0, ptCursor.x, ptCursor.y, 0, _hwnd, NULL);
  }
  return 0;
}

// ----------------------------------------------------------------------------
LRESULT CALLBACK WinStats::procMouse(int nCode, WPARAM wParam, LPARAM lParam)
{
  LPMSLLHOOKSTRUCT pHook = (LPMSLLHOOKSTRUCT)lParam;

  if (nCode != HC_ACTION) {
    return CallNextHookEx(_hookMouse, nCode, wParam, lParam);
  }

  const HWND hwndFg = GetForegroundWindow();
  const HWND hwndMouse = WindowFromPoint(pHook->pt);
  
  POINT ptClient = pHook->pt;
  ScreenToClient(hwndMouse, &ptClient);

  const long dx = pHook->pt.x - _mouseGlobalPos.x;
  const long dy = pHook->pt.y - _mouseGlobalPos.y;
  const __int64 dist = (__int64)(sqrt(dx * dx + dy * dy) * 256.0);

  _mouseDistance += dist;
  _mouseGlobalPos = pHook->pt;
  _mouseClientPos = ptClient;

  if (wParam == WM_MOUSEWHEEL) {
    short nWheel = (short)(pHook->mouseData >> 16);
    _mouseWheel += abs(nWheel);
  }

  // Fix for broken mouse buttons (left): filter clicks coming in too quickly
  if (wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP || wParam == WM_LBUTTONDBLCLK)
  {
    // Left Mouse Button anti contact bounce
    static DWORD tLastLButtonUp = 0;
    DWORD tNow = GetTickCount();

    if (wParam == WM_LBUTTONUP) {
      tLastLButtonUp = tNow;
    }

    if (wParam == WM_LBUTTONDOWN && tNow - tLastLButtonUp < 60) {
      return 1;
    }
  }

  // Simulate left-button double click on middle button press
  if (wParam == WM_MBUTTONUP || wParam == WM_MBUTTONDBLCLK) {
    return 1;
  }

  if (wParam == WM_MBUTTONDOWN)
  {
    // Activate the Window the user clicked on
    ShowWindowAsync(hwndMouse, SW_SHOW);

    // Generate a left-button double click sequence
    const LPARAM lParamPos = MAKELONG(ptClient.x, ptClient.y);

    const std::initializer_list<UINT> seq = {
      WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK, WM_LBUTTONUP };
    for (const auto& message : seq) {
      PostMessage(hwndMouse, message, 0, lParamPos);
    }

    return 1;
  }

  return CallNextHookEx(_hookMouse, nCode, wParam, lParam);
}

// ----------------------------------------------------------------------------
LRESULT CALLBACK WinStats::procKeyboard(int code, WPARAM wParam, LPARAM lParam)
{
  KBDLLHOOKSTRUCT* pHook = (KBDLLHOOKSTRUCT*)lParam;

  if (code == HC_ACTION && wParam == WM_KEYUP) {
    _keyStrokes++;
  }

  // got used to Fn + Up/Down is Page Up/Down on your notebook?
  // Have it here with ESC + Up/Down
  if ((GetKeyState(VK_ESCAPE) & 0x8000) != 0)
  {
    switch (pHook->vkCode)
    {
    case VK_UP:
      return SendKey(pHook, VK_PRIOR);

    case VK_DOWN:
      return SendKey(pHook, VK_NEXT);
    }
  }

  return CallNextHookEx(WinStats::_hookKb, code, wParam, lParam);
}

// ----------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  ghInstance = hInstance;

  WinStats* pWin = nullptr;
  WinStatsFactory fab;
  fab.createWindow(&pWin);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  delete pWin;
  return static_cast<int>(msg.wParam);
}

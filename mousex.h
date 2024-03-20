#pragma once

#define _WIN32_WINNT 0x0500
#define WINVER 0x0500

#include <windows.h>
#undef min
#undef max

#include "resource.h"

// ----------------------------------------------------------------------------
class WinStats
{
  // Allow WinStatsFactory to call the window "proc"
  friend class WinStatsFactory;

public:
  WinStats(WinStatsFactory* fab);

  HWND hwnd() const;

protected:
  LRESULT proc(HWND hwnd, UINT cmd, WPARAM wParam, LPARAM lParam);

  HWND _hwnd = nullptr;
  
  const WinStatsFactory* _fab = nullptr;

private:
  LRESULT onCreate();
  LRESULT onDestroy();
  LRESULT onClose();
  LRESULT onQueryEndSession();
  LRESULT onTimer();
  LRESULT onCommand(HWND hwndCtrl, WORD wCode, WORD wID);
  LRESULT onShowWindow(bool show);
  LRESULT onPaint(HDC hdc, const PAINTSTRUCT* pPS);

  // User defined messages
  LRESULT onTaskbarNotify(LPARAM lParam, WPARAM wParam);

  // hooks don't have a way to pass user data thereby making it necessary to use
  // static variables
  static LRESULT CALLBACK procMouse(int code, WPARAM wParam, LPARAM lParam);
  static HHOOK _hookMouse;
  static __int64 _mouseDistance;
  static __int64 _mouseWheel;
  static POINT _mouseGlobalPos;
  static POINT _mouseClientPos;

  static LRESULT CALLBACK procKeyboard(int code, WPARAM wParam, LPARAM lParam);
  static HHOOK _hookKb;
  static __int64 _keyStrokes;

  int saveState() const;
  int restoreState();
};

// ----------------------------------------------------------------------------
class WinStatsFactory
{
public:
  WinStatsFactory();

  int createWindow(WinStats** ppWin);
  HICON appIcon() const;
  HMENU appMenu() const;

protected:
  void _registerClass(const char* ClassName, HICON hIcon);

private:
  static LRESULT CALLBACK __procStatic1(
    HWND hwnd, UINT cmd, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK __procStaticN(
    HWND hwnd, UINT cmd, WPARAM wParam, LPARAM lParam);

  HICON _icon = nullptr;
  HMENU _menu = nullptr;
};

// ----------------------------------------------------------------------------
inline HWND WinStats::hwnd() const {
  return _hwnd;
}

// ----------------------------------------------------------------------------
inline HICON WinStatsFactory::appIcon() const {
  return _icon;
}

// ----------------------------------------------------------------------------
inline HMENU WinStatsFactory::appMenu() const {
  return _menu;
}

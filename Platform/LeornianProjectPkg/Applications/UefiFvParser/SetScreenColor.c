
#include <stdio.h>
#include "SetScreenColor.h"

#if defined(__unix) || defined(__unix__) || defined(__LINUX__)

int SetScreenColor (char *TextColor) {
  printf(TextColor);
  return 0;
}

#elif defined(_WIN32) || defined(__WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64) || defined(__WIN64__)
//#include <windows.h>

int SetScreenColor (WORD TextColor) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  WORD saved_attributes;

  //
  // Save current attributes 
  //
  GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
  saved_attributes = consoleInfo.wAttributes;

  SetConsoleTextAttribute(hConsole, TextColor);

  return 0;
}

#else

int SetScreenColor (int TextColor) {
  return 0;
}

#endif
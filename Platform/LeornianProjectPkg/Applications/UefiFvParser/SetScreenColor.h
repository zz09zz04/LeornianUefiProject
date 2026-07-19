#pragma once

#include <stdio.h>

#if defined(__unix) || defined(__unix__) || defined(__LINUX__)

#define RED_TEXT			      "\033[31m"
#define GREEN_TEXT          "\033[32m"
#define ORANGE_TEXT         "\033[33m"
#define BLUE_TEXT           "\033[34m"
#define PURPLE_TEXT         "\033[35m"
#define CYAN_TEXT           "\033[36m"
#define GRAY_TEXT           "\033[37m"
#define LIGHTRED_TEXT       "\033[91m"
#define LIGHTGREEN_TEXT     "\033[92m"
#define LIGHTORANGE_TEXT    "\033[93m"
#define LIGHTBLUE_TEXT      "\033[94m"
#define LIGHTPURPLE_TEXT    "\033[95m"
#define LIGHTCYAN_TEXT      "\033[96m"
#define WHITE_TEXT          "\033[97m"

int SetScreenColor (
  char *TextColor
  );

#elif defined(_WIN32) || defined(__WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64) || defined(__WIN64__)

#include <Uefi.h>
#include <Protocol/SimpleTextOut.h>

#define FOREGROUND_BLUE         EFI_BLUE
#define FOREGROUND_GREEN        EFI_GREEN
#define FOREGROUND_RED          EFI_RED
#define FOREGROUND_INTENSITY    EFI_BRIGHT

#define BLUE_TEXT        FOREGROUND_BLUE
#define GREEN_TEXT       FOREGROUND_GREEN
#define RED_TEXT         FOREGROUND_RED
#define CYAN_TEXT        FOREGROUND_BLUE | FOREGROUND_GREEN
#define PURPLE_TEXT      FOREGROUND_BLUE | FOREGROUND_RED
#define ORANGE_TEXT      FOREGROUND_GREEN | FOREGROUND_RED
#define WHITE_TEXT       FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
#define GRAY_TEXT        FOREGROUND_INTENSITY
#define LIGHTBLUE_TEXT   FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define LIGHTGREEN_TEXT  FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define LIGHTRED_TEXT    FOREGROUND_RED | FOREGROUND_INTENSITY
#define LIGHTCYAN_TEXT   FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define LIGHTPURPLE_TEXT FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY
#define LIGHTORANGE_TEXT FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY

typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef void *HANDLE;

typedef struct {
    short X;
    short Y;
} COORD;

typedef struct {
    short Left;
    short Top;
    short Right;
    short Bottom;
} SMALL_RECT;

typedef struct {
    COORD dwSize;
    COORD dwCursorPosition;
    WORD  wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO;


#define STD_OUTPUT_HANDLE ((DWORD)-11)


__declspec(dllimport)
HANDLE
GetStdHandle(
    DWORD nStdHandle
);


__declspec(dllimport)
int
GetConsoleScreenBufferInfo(
    HANDLE hConsoleOutput,
    CONSOLE_SCREEN_BUFFER_INFO *lpConsoleScreenBufferInfo
);


__declspec(dllimport)
int
SetConsoleTextAttribute(
    HANDLE hConsoleOutput,
    WORD wAttributes
);



int SetScreenColor (
  WORD
  ); 

#else

#define BLUE_TEXT           0
#define GREEN_TEXT          0
#define RED_TEXT            0
#define CYAN_TEXT           0
#define PURPLE_TEXT         0
#define ORANGE_TEXT         0
#define WHITE_TEXT          0
#define GRAY_TEXT           0
#define LIGHTBLUE_TEXT      0
#define LIGHTGREEN_TEXT     0
#define LIGHTRED_TEXT       0
#define LIGHTCYAN_TEXT      0
#define LIGHTPURPLE_TEXT    0
#define LIGHTORANGE_TEXT    0

int SetScreenColor(
  int TextColor
  );

#endif
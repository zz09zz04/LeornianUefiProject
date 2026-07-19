/* ansi_escape.h
 *
 * ANSI Escape Codes for terminal colors, styles, and cursor control.
 * Compatible with most UNIX terminals, xterm, Linux console, Windows Terminal.
 */

#ifndef ANSI_ESCAPE_H
#define ANSI_ESCAPE_H

/* ============================================================
 * Basic Escape Sequence
 * ============================================================ */
#define ANSI_ESC "\x1b"

/* ============================================================
 * Reset
 * ============================================================ */
#define ANSI_RESET ANSI_ESC "[0m"

/* ============================================================
 * Text Styles
 * ============================================================ */
#define ANSI_BOLD          ANSI_ESC "[1m"
#define ANSI_DIM           ANSI_ESC "[2m"
#define ANSI_ITALIC        ANSI_ESC "[3m"
#define ANSI_UNDERLINE     ANSI_ESC "[4m"
#define ANSI_BLINK         ANSI_ESC "[5m"
#define ANSI_REVERSE       ANSI_ESC "[7m"
#define ANSI_HIDDEN        ANSI_ESC "[8m"
#define ANSI_STRIKETHROUGH ANSI_ESC "[9m"

/* Style Reset */
#define ANSI_NO_BOLD          ANSI_ESC "[22m"
#define ANSI_NO_ITALIC        ANSI_ESC "[23m"
#define ANSI_NO_UNDERLINE     ANSI_ESC "[24m"
#define ANSI_NO_BLINK         ANSI_ESC "[25m"
#define ANSI_NO_REVERSE       ANSI_ESC "[27m"
#define ANSI_NO_HIDDEN        ANSI_ESC "[28m"
#define ANSI_NO_STRIKETHROUGH ANSI_ESC "[29m"

/* ============================================================
 * Foreground Colors
 * ============================================================ */
#define ANSI_FG_BLACK   ANSI_ESC "[30m"
#define ANSI_FG_RED     ANSI_ESC "[31m"
#define ANSI_FG_GREEN   ANSI_ESC "[32m"
#define ANSI_FG_YELLOW  ANSI_ESC "[33m"
#define ANSI_FG_BLUE    ANSI_ESC "[34m"
#define ANSI_FG_MAGENTA ANSI_ESC "[35m"
#define ANSI_FG_CYAN    ANSI_ESC "[36m"
#define ANSI_FG_WHITE   ANSI_ESC "[37m"

#define ANSI_FG_DEFAULT ANSI_ESC "[39m"

/* Bright Foreground Colors */
#define ANSI_FG_BRIGHT_BLACK   ANSI_ESC "[90m"
#define ANSI_FG_BRIGHT_RED     ANSI_ESC "[91m"
#define ANSI_FG_BRIGHT_GREEN   ANSI_ESC "[92m"
#define ANSI_FG_BRIGHT_YELLOW  ANSI_ESC "[93m"
#define ANSI_FG_BRIGHT_BLUE    ANSI_ESC "[94m"
#define ANSI_FG_BRIGHT_MAGENTA ANSI_ESC "[95m"
#define ANSI_FG_BRIGHT_CYAN    ANSI_ESC "[96m"
#define ANSI_FG_BRIGHT_WHITE   ANSI_ESC "[97m"

/* ============================================================
 * Background Colors
 * ============================================================ */
#define ANSI_BG_BLACK   ANSI_ESC "[40m"
#define ANSI_BG_RED     ANSI_ESC "[41m"
#define ANSI_BG_GREEN   ANSI_ESC "[42m"
#define ANSI_BG_YELLOW  ANSI_ESC "[43m"
#define ANSI_BG_BLUE    ANSI_ESC "[44m"
#define ANSI_BG_MAGENTA ANSI_ESC "[45m"
#define ANSI_BG_CYAN    ANSI_ESC "[46m"
#define ANSI_BG_WHITE   ANSI_ESC "[47m"

#define ANSI_BG_DEFAULT ANSI_ESC "[49m"

/* Bright Background Colors */
#define ANSI_BG_BRIGHT_BLACK   ANSI_ESC "[100m"
#define ANSI_BG_BRIGHT_RED     ANSI_ESC "[101m"
#define ANSI_BG_BRIGHT_GREEN   ANSI_ESC "[102m"
#define ANSI_BG_BRIGHT_YELLOW  ANSI_ESC "[103m"
#define ANSI_BG_BRIGHT_BLUE    ANSI_ESC "[104m"
#define ANSI_BG_BRIGHT_MAGENTA ANSI_ESC "[105m"
#define ANSI_BG_BRIGHT_CYAN    ANSI_ESC "[106m"
#define ANSI_BG_BRIGHT_WHITE   ANSI_ESC "[107m"

/* ============================================================
 * 256-color Support
 * Usage:
 *   printf(ANSI_FG_256(196) "Red" ANSI_RESET);
 * ============================================================ */
#define ANSI_FG_256(n) "\x1b[38;5;" #n "m"
#define ANSI_BG_256(n) "\x1b[48;5;" #n "m"

/* ============================================================
 * True Color (24-bit RGB)
 * Usage:
 *   printf(ANSI_RGB_FG(255,128,0) "Orange" ANSI_RESET);
 * ============================================================ */
#define ANSI_RGB_FG(r,g,b) "\x1b[38;2;" #r ";" #g ";" #b "m"
#define ANSI_RGB_BG(r,g,b) "\x1b[48;2;" #r ";" #g ";" #b "m"

/* ============================================================
 * Cursor Control
 * ============================================================ */

/* Cursor movement */
#define ANSI_CURSOR_UP(n)       "\x1b[" #n "A"
#define ANSI_CURSOR_DOWN(n)     "\x1b[" #n "B"
#define ANSI_CURSOR_FORWARD(n)  "\x1b[" #n "C"
#define ANSI_CURSOR_BACK(n)     "\x1b[" #n "D"

/* Cursor position */
#define ANSI_CURSOR_POS(row,col) "\x1b[" #row ";" #col "H"

/* Save / Restore cursor */
#define ANSI_SAVE_CURSOR    ANSI_ESC "[s"
#define ANSI_RESTORE_CURSOR ANSI_ESC "[u"

/* Clear screen / line */
#define ANSI_CLEAR_SCREEN ANSI_ESC "[2J"
#define ANSI_CLEAR_LINE   ANSI_ESC "[2K"

/* ============================================================
 * Utility Macros
 * ============================================================ */

/* Wrap text with style */
#define ANSI_WRAP(style, text) style text ANSI_RESET

#endif /* ANSI_ESCAPE_H */
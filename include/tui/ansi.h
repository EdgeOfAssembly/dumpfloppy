/**
 * @file ansi.h
 * @brief TUI colour macros used by the report. Wins over libsf via `-Iinclude`.
 */
#ifndef TUI_ANSI_H
#define TUI_ANSI_H

#define TUI_CYAN           "\x1b[36m"
#define TUI_WHITE          "\x1b[37m"
#define TUI_BG_BRIGHT_RED  "\x1b[101m"
#define TUI_RESET          "\x1b[0m"
#define TUI_BOLD           "\x1b[1m"
#define TUI_BLINK          "\x1b[5m"

#endif /* TUI_ANSI_H */

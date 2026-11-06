#ifndef PLATFORM_DETACH_H
#define PLATFORM_DETACH_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Whether this process was launched from an interactive shell that's
// still around waiting on it (so a GUI run should hand control back) -
// as opposed to launched directly (a double-click, a Start Menu/Desktop
// shortcut, a Linux .desktop entry with no terminal, ...), where there's
// no shell prompt to return and the GUI can just run here.
bool platform_launched_from_terminal(void);

// Re-launches the current program (same argv) as a new process fully
// detached from this one's console/terminal, so a shell waiting on this
// process gets its prompt back immediately. Returns true if the new
// process was started successfully - the caller should exit right away
// in that case, leaving the detached process to do the actual work.
bool platform_relaunch_detached(int argc, char *argv[]);

// Windows: drops the console Windows auto-allocates for a console-
// subsystem binary launched with no console of its own (e.g. a double-
// click), so no stray console window sits behind the GUI. No-op
// elsewhere.
void platform_detach_console(void);

#ifdef __cplusplus
}
#endif

#endif

#include "platform/detach.h"

#ifdef _WIN32

#include <stdio.h>
#include <string.h>
#include <windows.h>

bool platform_launched_from_terminal(void) {
    HWND console = GetConsoleWindow();
    if (!console)
        return false;

    // GetConsoleProcessList's return value is how many processes are
    // attached to this console, not just how many fit in the buffer.
    // ==1 means only this process - Windows auto-allocated a fresh
    // console for us (typical of a double-click/shortcut launch), so
    // there's no shell prompt waiting on us. >1 means a shell (cmd.exe,
    // PowerShell, ...) shares the console with us.
    DWORD pids[2];
    DWORD count = GetConsoleProcessList(pids, 2);
    return count > 1;
}

bool platform_relaunch_detached(int argc, char *argv[]) {
    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_path, sizeof(exe_path)) == 0)
        return false;

    char cmdline[32768];
    int written = snprintf(cmdline, sizeof(cmdline), "\"%s\"", exe_path);
    if (written < 0 || (size_t)written >= sizeof(cmdline))
        return false;

    for (int i = 1; i < argc; i++) {
        int n = snprintf(cmdline + written, sizeof(cmdline) - (size_t)written, " \"%s\"", argv[i]);
        if (n < 0 || (size_t)(written + n) >= sizeof(cmdline))
            return false;
        written += n;
    }

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                             DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi);
    if (!ok)
        return false;

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

void platform_detach_console(void) { FreeConsole(); }

#else

#include <fcntl.h>
#include <unistd.h>

bool platform_launched_from_terminal(void) {
    return isatty(STDIN_FILENO) || isatty(STDOUT_FILENO);
}

bool platform_relaunch_detached(int argc, char *argv[]) {
    (void)argc;

    pid_t pid = fork();
    if (pid < 0)
        return false;
    if (pid > 0)
        return true; // parent: caller exits right after this returns

    // Child: start a new session (detaches the controlling terminal, so
    // it keeps running after the shell that launched us exits) and
    // redirect std streams away from the terminal - besides being the
    // correct way to detach, this also keeps the re-exec below from
    // seeing a tty on stdin/stdout and re-triggering a relaunch.
    setsid();

    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO)
            close(devnull);
    }

    execv("/proc/self/exe", argv);
    _exit(127); // execv failed; no terminal left to report to
}

void platform_detach_console(void) {}

#endif

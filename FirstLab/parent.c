#include <windows.h>
#include <stdlib.h>

DWORD my_strlen(const char *s) {
    DWORD len = 0;
    while (s[len]) len++;
    return len;
}

int my_strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

DWORD read_line(char *buf, DWORD max) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD read = 0;
    if (!ReadFile(hIn, buf, max - 1, &read, NULL)) return 0;
    while (read > 0 && (buf[read-1] == '\n' || buf[read-1] == '\r')) {
        read--;
    }
    buf[read] = '\0';
    return read;
}

void print(const char *s) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, my_strlen(s), &written, NULL);
}

int main() {
    srand(GetTickCount());

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hPipe1Read, hPipe1Write;
    HANDLE hPipe2Read, hPipe2Write;

    if (!CreatePipe(&hPipe1Read, &hPipe1Write, &saAttr, 0)) {
        print("Err CreatePipe 1\n");
        return 1;
    }
    if (!CreatePipe(&hPipe2Read, &hPipe2Write, &saAttr, 0)) {
        print("Err CreatePipe 2\n");
        return 1;
    }

    SetHandleInformation(hPipe1Write, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hPipe2Write, HANDLE_FLAG_INHERIT, 0);

    char file1[MAX_PATH], file2[MAX_PATH];

    print("Name for child1: ");
    read_line(file1, MAX_PATH);

    print("Name for child2: ");
    read_line(file2, MAX_PATH);

    STARTUPINFOA si1;
    PROCESS_INFORMATION pi1;
    ZeroMemory(&si1, sizeof(si1));
    si1.cb = sizeof(si1);
    si1.dwFlags = STARTF_USESTDHANDLES;
    si1.hStdInput = hPipe1Read;
    si1.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si1.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    char cmd1[MAX_PATH + 20];
    const char *exe1 = "child1.exe ";
    DWORD k = 0;
    while (exe1[k]) { cmd1[k] = exe1[k]; k++; }
    DWORD i = 0;
    while (file1[i]) { cmd1[k++] = file1[i++]; }
    cmd1[k] = '\0';

    if (!CreateProcessA(NULL, cmd1, NULL, NULL, TRUE, 0, NULL, NULL, &si1, &pi1)) {
        print("Err CreateProcess child1\n");
        return 1;
    }

    STARTUPINFOA si2;
    PROCESS_INFORMATION pi2;
    ZeroMemory(&si2, sizeof(si2));
    si2.cb = sizeof(si2);
    si2.dwFlags = STARTF_USESTDHANDLES;
    si2.hStdInput = hPipe2Read;
    si2.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si2.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    char cmd2[MAX_PATH + 20];
    const char *exe2 = "child2.exe ";
    k = 0;
    while (exe2[k]) { cmd2[k] = exe2[k]; k++; }
    i = 0;
    while (file2[i]) { cmd2[k++] = file2[i++]; }
    cmd2[k] = '\0';

    if (!CreateProcessA(NULL, cmd2, NULL, NULL, TRUE, 0, NULL, NULL, &si2, &pi2)) {
        print("Err CreateProcess child2\n");
        return 1;
    }

    CloseHandle(hPipe1Read);
    CloseHandle(hPipe2Read);
    CloseHandle(pi1.hThread);
    CloseHandle(pi2.hThread);

    print("\nEnter(exit to quit):\n");

    char input[1024];
    while (1) {
        print("> ");
        DWORD len = read_line(input, sizeof(input));
        if (len == 0) continue;

        if (my_strcmp(input, "exit") == 0) break;

        int r = rand() % 100;
        DWORD written;

        if (r < 80) {
            WriteFile(hPipe1Write, input, my_strlen(input), &written, NULL);
            WriteFile(hPipe1Write, "\n", 1, &written, NULL);
        } else {
            WriteFile(hPipe2Write, input, my_strlen(input), &written, NULL);
            WriteFile(hPipe2Write, "\n", 1, &written, NULL);
        }
    }

    CloseHandle(hPipe1Write);
    CloseHandle(hPipe2Write);

    WaitForSingleObject(pi1.hProcess, INFINITE);
    WaitForSingleObject(pi2.hProcess, INFINITE);

    CloseHandle(pi1.hProcess);
    CloseHandle(pi2.hProcess);

    print("Exiting.\n");
    return 0;
}
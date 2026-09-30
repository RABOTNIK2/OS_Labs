#include <windows.h>

#define MAX_NUMS 1024
#define PIPE_NAME "\\\\.\\pipe\\lab_pipe2"

DWORD my_strlen(const char *s) {
    DWORD len = 0;
    while (s[len]) len++;
    return len;
}

void print(const char *s) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, my_strlen(s), &written, NULL);
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

int parse_ints(const char *s, int *out, int max_count) {
    int count = 0;
    int i = 0;

    while (s[i] != '\0' && count < max_count) {
        while (s[i] == ' ' || s[i] == '\t') i++;
        if (s[i] == '\0') break;

        int sign = 1;
        if (s[i] == '-') { sign = -1; i++; }
        else if (s[i] == '+') { i++; }

        if (s[i] < '0' || s[i] > '9') return -1;

        int num = 0;
        while (s[i] >= '0' && s[i] <= '9') {
            num = num * 10 + (s[i] - '0');
            i++;
        }

        out[count++] = sign * num;
    }

    return count;
}

int main() {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hPipe1Read, hPipe1Write;
    if (!CreatePipe(&hPipe1Read, &hPipe1Write, &saAttr, 0)) {
        print("Err creating pipe1.\n");
        return 1;
    }

    SetHandleInformation(hPipe1Write, HANDLE_FLAG_INHERIT, 0);

    HANDLE hPipe2 = CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        1024,
        1024,
        0,
        NULL
    );

    if (hPipe2 == INVALID_HANDLE_VALUE) {
        print("Err creating pipe2.\n");
        return 1;
    }

    SetEnvironmentVariableA("PIPE2_NAME", PIPE_NAME);

    char filename[MAX_PATH];
    print("Enter file name: ");
    read_line(filename, MAX_PATH);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hPipe1Read;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    char cmd[MAX_PATH + 20];
    const char *exe = "child.exe ";
    DWORD k = 0;
    while (exe[k]) { cmd[k] = exe[k]; k++; }
    DWORD i = 0;
    while (filename[i]) { cmd[k++] = filename[i++]; }
    cmd[k] = '\0';

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        print("Err creating process.\n");
        return 1;
    }

    CloseHandle(hPipe1Read);

    ConnectNamedPipe(hPipe2, NULL);

    print("\nEnter names.\n");
    print("To exit type exit.\n");

    char input[4096];
    int nums[MAX_NUMS];
    int should_exit = 0;

    while (!should_exit) {
        print("> ");
        DWORD len = read_line(input, sizeof(input));
        if (len == 0) continue;

        int is_exit = 1;
        const char *exit_cmd = "exit";
        for (int j = 0; exit_cmd[j]; j++) {
            if (input[j] != exit_cmd[j]) { is_exit = 0; break; }
        }
        if (is_exit && input[4] == '\0') break;

        int count = parse_ints(input, nums, MAX_NUMS);
        if (count < 2) {
            print("Err need 2 numbers.\n");
            continue;
        }

        DWORD written;
        WriteFile(hPipe1Write, &count, sizeof(int), &written, NULL);
        WriteFile(hPipe1Write, nums, sizeof(int) * count, &written, NULL);

        int signal = 0;
        DWORD read_bytes = 0;
        if (ReadFile(hPipe2, &signal, sizeof(int), &read_bytes, NULL) && read_bytes > 0) {
            if (signal == 1) {
                print("Zero division. Exiting\n");
                should_exit = 1;
            }
        }
    }

    CloseHandle(hPipe1Write);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hPipe2);

    print("Exiting.\n");
    return 0;
}
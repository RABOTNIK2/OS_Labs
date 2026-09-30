#include <windows.h>

#define MAX_NUMS 1024

DWORD my_strlen(const char *s) {
    DWORD len = 0;
    while (s[len]) len++;
    return len;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        const char *err = "Err empty file name.\n";
        DWORD written;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), err, my_strlen(err), &written, NULL);
        return 1;
    }

    HANDLE hFile = CreateFileA(
        argv[1],
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        const char *err = "Err opening file.\n";
        DWORD written;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), err, my_strlen(err), &written, NULL);
        return 1;
    }

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    HANDLE hPipe2Write = NULL;
    char pipe2Name[MAX_PATH];
    DWORD nameLen = GetEnvironmentVariableA("PIPE2_NAME", pipe2Name, MAX_PATH);
    if (nameLen > 0) {
        hPipe2Write = CreateFileA(
            pipe2Name,
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    while (1) {
        int count = 0;
        DWORD bytes_read = 0;

        if (!ReadFile(hStdin, &count, sizeof(int), &bytes_read, NULL) || bytes_read == 0) {
            break;
        }

        if (count <= 0 || count > MAX_NUMS) {
            const char *err = "Err invalid numbers of int.\n";
            DWORD written;
            WriteFile(hFile, err, my_strlen(err), &written, NULL);
            break;
        }

        int nums[MAX_NUMS];
        if (!ReadFile(hStdin, nums, sizeof(int) * count, &bytes_read, NULL) || bytes_read == 0) {
            break;
        }

        int result = nums[0];
        int division_by_zero = 0;
        int zero_index = -1;

        for (int i = 1; i < count; i++) {
            if (nums[i] == 0) {
                division_by_zero = 1;
                zero_index = i;
                break;
            }
            result /= nums[i];
        }

        DWORD written;
        if (division_by_zero) {
            const char *msg = "Err zero division. Exiting\n";
            WriteFile(hFile, msg, my_strlen(msg), &written, NULL);

            if (hPipe2Write != NULL) {
                int signal = 1;
                WriteFile(hPipe2Write, &signal, sizeof(int), &written, NULL);
                CloseHandle(hPipe2Write);
            }

            CloseHandle(hFile);
            return 1;
        }

        char out[64];
        int pos = 0;
        int tmp = result;
        int negative = 0;

        if (tmp < 0) {
            negative = 1;
            tmp = -tmp;
        }

        char digits[32];
        int d = 0;
        if (tmp == 0) {
            digits[d++] = '0';
        } else {
            while (tmp > 0) {
                digits[d++] = '0' + (tmp % 10);
                tmp /= 10;
            }
        }

        if (negative) out[pos++] = '-';
        for (int i = d - 1; i >= 0; i--) {
            out[pos++] = digits[i];
        }
        out[pos++] = '\n';

        WriteFile(hFile, out, pos, &written, NULL);
    }

    if (hPipe2Write != NULL) {
        int signal = 0;
        DWORD written;
        WriteFile(hPipe2Write, &signal, sizeof(int), &written, NULL);
        CloseHandle(hPipe2Write);
    }

    CloseHandle(hFile);
    return 0;
}
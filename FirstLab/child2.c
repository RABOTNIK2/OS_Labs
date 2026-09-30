#include <windows.h>

#define READ_SIZE 1024
#define BUF_SIZE  2048

char my_tolower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

int is_vowel(char c) {
    c = my_tolower(c);
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y');
}

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
        const char *err = "Err openning file.\n";
        DWORD written;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), err, my_strlen(err), &written, NULL);
        return 1;
    }

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    char buffer[BUF_SIZE];
    char result[BUF_SIZE];
    DWORD bytes_read;

    while (ReadFile(hStdin, buffer, READ_SIZE, &bytes_read, NULL) && bytes_read > 0) {
        buffer[bytes_read] = '\0';

        DWORD j = 0;
        for (DWORD i = 0; i < bytes_read; i++) {
            if (!is_vowel(buffer[i])) {
                result[j++] = buffer[i];
            }
        }

        DWORD bytes_written;
        if (!WriteFile(hFile, result, j, &bytes_written, NULL)) {
            const char *err = "Err write to file.\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), err, my_strlen(err), &bytes_written, NULL);
            break;
        }
    }

    CloseHandle(hFile);
    return 0;
}
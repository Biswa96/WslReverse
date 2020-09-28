/*
 * This file is part of WslReverse project.
 * Licensed under the terms of the GNU General Public License v3 or later.
 * WslClient.h: WslClient COM interface declaration and related CLSID and IID.
 */

#ifndef WSLCLIENT_H
#define WSLCLIENT_H

 /* {615A13BE-241D-48B1-89B0-8E1D40FFD287} */
static const GUID CLSID_WslClient = {
    0x615A13BE,
    0x241D,
    0x48B1,
    { 0x89, 0xB0, 0x8E, 0x1D, 0x40, 0xFF, 0xD2, 0x87 } };

/* {91A11D73-FC34-455C-861F-0A371E4B56AE} */
static const GUID IID_IWslClient = {
    0x91A11D73,
    0xFC34,
    0x455C,
    { 0x86, 0x1F, 0x0A, 0x37, 0x1E, 0x4B, 0x56, 0xAE } };

typedef enum _WslClientEntry
{
    WSL_CLENT_ENTRY_BASH = 0,
    WSL_CLENT_ENTRY_WSL = 1,
    WSL_CLENT_ENTRY_WSLCONFIG = 2,
    /* WSL_CLENT_ENTRY_WSLHOST = 3, Removed from Win Build 20201 */
} WslClientEntry;

typedef struct _IWslClient IWslClient;

struct _IWslClientVtbl
{
    HRESULT(*QueryInterface)(
        /* In */ IWslClient* This,
        /* In */ GUID* Riid,
        /* Out */ PVOID* ppvObject);

    ULONG(*AddRef)(
        /* In */ IWslClient* This);

    ULONG(*Release)(
        /* In */ IWslClient* This);

    HRESULT(*Main)(
        /* In */ IWslClient* This,
        /* In */ WslClientEntry EntryType,
        /* In */ PCWSTR CommandLine,
        /* Out */ PDWORD Result);
};

struct _IWslClient
{
    struct _IWslClientVtbl* lpVtbl;
};

#endif /* WSLCLIENT_H */

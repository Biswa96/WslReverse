#include <Windows.h>

void Log(HRESULT result, PCWSTR function) {

    if (result == S_OK) {
        wprintf(L"%ls success\n", function);
    }
    else {
        wprintf(L"%ls failed error: %d\n", function, (result & 0xFFFF));
        exit(1);
    }
}

void usage() {
    printf(
        "\nWslReverse -- (c) Copyright 2018 Biswapriyo Nath\n"
        "Licensed under GNU Public License version 3 or higher\n\n"
        "Use hidden COM interface of Windows Subsystem for Linux for fun\n"
        "Usage: WslReverse.exe [-] [option] [argument]\n\n"
        "Options:\n"
        "  -d, --get-id       [distribution name]      Get distribution ID.\n"
        "  -G, --get-default                           Get default distribution ID.\n"
        "  -g, --get-config   [distribution name]      Get distribution configuration.\n"
        "  -h, --help                                  Show list of options.\n"
        "  -i, --install      [distribution name]      Install distribution (run as administrator).\n"
        "  -r, --run          [distribution name]      Run a Linux binary (incomplete feature).\n"
        "  -S, --set-default  [distribution name]      Set default distribution.\n"
        "  -s, --set-config   [distribution name]      Set configuration for distribution.\n"
        "  -u, --uninstall    [distribution name]      Uninstall distribution.\n"
        "\n"
    );
    exit(EXIT_FAILURE);
}

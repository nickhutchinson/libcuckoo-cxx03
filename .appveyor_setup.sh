#!/bin/bash
set -euo pipefail
set -o xtrace

# Enable crash dumps; don't show UI and hang the process. See:
# - https://msdn.microsoft.com/en-us/library/windows/desktop/bb787181(v=vs.85).aspx
# - https://msdn.microsoft.com/en-us/library/bb513638%28VS.85%29.aspx
mkdir -p crash_dumps
reg add 'HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps' \
    -f                                                                       \
    -v DumpType                                                              \
    -t REG_DWORD                                                             \
    -d 1
reg add 'HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps' \
    -f                                                                       \
    -v DumpFolder                                                            \
    -t REG_EXPAND_SZ                                                         \
    -d "$(cygpath -wa crash_dumps)"
reg add 'HKLM\Software\Microsoft\Windows\Windows Error Reporting' \
    -f                                                            \
    -v DontShowUI                                                 \
    -t REG_DWORD                                                  \
    -d 1

if ! type ninja >/dev/null 2>&1; then
    mkdir -p _deps
    pushd _deps
    curl -fLO https://github.com/ninja-build/ninja/releases/download/v1.7.2/ninja-win.zip
    7z x ninja-win.zip -obin ninja.exe
    ninja --version
    rm ninja-win.zip
    popd
fi

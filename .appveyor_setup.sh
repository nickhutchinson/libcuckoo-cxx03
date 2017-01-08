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

if ! type innoextract >/dev/null 2>&1; then
    mkdir -p _deps
    pushd _deps
    curl -fLO http://constexpr.org/innoextract/files/innoextract-1.6-windows.zip
    7z x innoextract-1.6-windows.zip -obin innoextract.exe
    innoextract --version
    rm innoextract-1.6-windows.zip
    popd
fi

if ! type ninja >/dev/null 2>&1; then
    mkdir -p _deps
    pushd _deps
    curl -fLO https://github.com/ninja-build/ninja/releases/download/v1.7.2/ninja-win.zip
    7z x ninja-win.zip -obin ninja.exe
    ninja --version
    rm ninja-win.zip
    popd
fi

if [[ ! -d "_deps/tbb44_20160803oss" ]]; then
    mkdir -p _deps
    pushd _deps
    curl -fLO https://www.threadingbuildingblocks.org/sites/default/files/software_releases/windows/tbb44_20160803oss_win.zip
    7z x tbb44_20160803oss_win.zip
    rm tbb44_20160803oss_win.zip
    popd
fi

if [[ $COMPILER =~ msvc-16 && ! -d "_deps/boost_1_62_0-msvc-10.0-64" ]]; then
    mkdir -p _deps
    pushd _deps
    curl -fLO 'http://netix.dl.sourceforge.net/project/boost/boost-binaries/1.62.0/boost_1_62_0-msvc-10.0-64.exe'

    # Only extract headers and libraries so it completes quicker. (It's not
    # perfect, because innoextract doesn't appear to let you only extract
    # particular directories. But it does seem to cut down on the amount of
    # unwanted docs/source files we waste time extracting.)
    innoextract boost_1_62_0-msvc-10.0-64.exe -I boost -I lib64-msvc-10.0 -s -p
    rm boost_1_62_0-msvc-10.0-64.exe

    # innoextract writes out a folder called 'app'. Rename it to something
    # useful.
    mv app boost_1_62_0-msvc-10.0-64

    # Try to keep under Appveyor cache limit by removing anything we don't need.
    # - documentation and source files
    # - dlls and import libs
    # - remove libs linked against static CRT
    pushd boost_1_62_0-msvc-10.0-64
    rm -fr doc libs tools
    rm -fr lib64-msvc-10.0/boost*
    rm -fr lib64-msvc-10.0/*vc100-{s,mt-s,sgd,mt-sgd}-*
    popd

    popd
fi

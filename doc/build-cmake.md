CMake build guide
======================

### command:
```code
mkdir build && cd build
cmake -DCMAKE_CXX_STANRDARD=17 .. 
```

### Available options:

- DISABLE_GUI
  - Disable building qt gui (Default is OFF)
- WITHOUT_QR_ENCODE
  - Disable qt qrencode support. (Default is OFF if libqrencode found)
- DISABLE_ZMQ
  - Disable ZMQ notifications (default is OFF).
  - [*NOTE*] - pure find_library is currently used to search for zmq. This means that zmq should be located in *default* libraries paths and include directories should be manually provided by -DCMAKE_CXX_FLAGS=-I\ /path/to/include
- WITH_SYSTEM_UNIVALUE
  - Default is **OFF**
  - Build with system UniValue (default is OFF)
- ENABLE_ASM
  - Enable assembly routines (default is ON)
- BDB_ROOT
  - Path to BerkleyDB 4.8
- DISABLE_WALLET
  - Disable wallet (enabled by default)
- ENABLE_GLIBC_BACK_COMPAT
  - enable backwards compatibility with glibc
- WITH_INCOMPATIBLE_BDB
  - Allow using a bdb version greater than 4.8
- EVENT_ROOT
  - Path to root of libevent library
  - If not specified - default library paths are used
- SQLITE_ROOT
  - Path to root of sqlite3 library
  - If not specified - default library paths are used
- OPENSSL_ROOT_DIR
  - Path to root if openssl library
  - If not specified - default library paths are used
- BOOST_ROOT
  - Path to root of boost
  - If not specified - default library paths are used
  - [*NOTE*] Pass here path to *path_to_boot/lib/cmake* folder instead of *path_to_boost/* if you have such
- MSVC_FORCE_STATIC
  - Build with MTd runtime linking. Use this if you want to statically link internal libraries. Ignored for non-MSVC build (default=ON)

There are also some hints options available for Boost and OpenSSL, see CMake module documentation for them. 

All options are specified like this: **-D**OPTION=VALUE\
If value is boolean - pass ON or OFF as a value

### Suggestions for Windows building
- Use **vcpkg** for dependencies
  - ```code
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg install package_name:x64-windows-static
    ```
  - Pass vcpkg toolchain as cmake option. Also specify used triplet (use x64-windows-static unless otherwise required)
    ```code
    -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
    -DVCPKG_TARGET_TRIPLET=x64-windows-static
    ```
- If building dependencies from source - specify MTd (Static debug runtime linker):
  - CMake:
    - There are libraries provided specific option for this (e.x. libevent)
    - Otherwise pass -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreadedDebug" as a cmake option
  - Others:
    - Pass "/MTd" as a compiler option (without "-")

### Step-by-step instruction for windows with clion and vcpkg
Configure Clion with VisualStudio toolchain
1) Install vspkg:
   - git clone https://github.com/Microsoft/vcpkg.git
   - cd vcpkg
   - ./bootstrap-vcpkg.sh
   - ./vcpkg integrate install
2) Install boost:
   - download boost_*.7z from point 1: https://www.boost.org/doc/libs/1_76_0/more/getting_started/windows.html
   - build from source: (In point 5.1 in link above)
     - .\bootstrap
     - .\b2 runtime-link=static runtime-debugging=on
3) Install packages in vcpkg:
   - ./vcpkg.exe install berkeleydb:x64-windows-static
   - ./vcpkg.exe install openssl:x64-windows-static
   - ./vcpkg.exe install sqlite3:x64-windows-static
   - ./vcpkg.exe install libevent:x64-windows-static
   - ./vcpkg.exe install protobuf:x64-windows-static
   - ./vcpkg.exe install qt5:x64-windows-static
   - ./vcpkg.exe install zeromq:x64-windows-static
   - ./vcpkg.exe install protobuf:x64-windows-static
4) Go to folder ..\vcpkg\installed\x64-windows-static\debug\lib and copy all content to folder ..\vcpkg\installed\x64-windows-static\lib
5) Install VisualStudio
6) In Settings -> Build, Execution, Deployment -> Toolchains - add VisualStuio Toolchain
7) In Settings -> Build, Execution, Deployment -> CMake - create debug profile
8) In debug profile -> CMake options - should be something like this:
   - -DCMAKE_CXX_STANDARD=17
   - -DBOOST_ROOT="C:\dev\boost\boost_1_77_0\stage\lib\cmake"
   - -DCMAKE_PREFIX_PATH="C:\dev\vcpkg\installed\x64-windows-static"
9) Profit
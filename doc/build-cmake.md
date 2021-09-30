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
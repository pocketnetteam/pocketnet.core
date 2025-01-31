# Поиск библиотеки i2pd
#
# Этот модуль определяет:
# I2PD_INCLUDE_DIRS - директории с заголовочными файлами
# I2PD_LIBRARIES - библиотеки для линковки
# I2PD_FOUND - найдена ли библиотека

find_path(I2PD_INCLUDE_DIR
    NAMES libi2pd/api.h
    PATHS ${CMAKE_PREFIX_PATH}/include/libi2pd
)

find_library(I2PD_LIBRARY
    NAMES i2pd libi2pd
    PATHS ${CMAKE_PREFIX_PATH}/lib
)

find_library(I2PD_CLIENT_LIBRARY
    NAMES i2pdclient libi2pdclient
    PATHS ${CMAKE_PREFIX_PATH}/lib
)

find_library(I2PD_LANG_LIBRARY
    NAMES i2pdlang libi2pdlang
    PATHS ${CMAKE_PREFIX_PATH}/lib
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(I2PD
    REQUIRED_VARS 
        I2PD_LIBRARY
        I2PD_CLIENT_LIBRARY
        I2PD_LANG_LIBRARY
        I2PD_INCLUDE_DIR
)

if(I2PD_FOUND)
    set(I2PD_LIBRARIES ${I2PD_LIBRARY} ${I2PD_CLIENT_LIBRARY} ${I2PD_LANG_LIBRARY})
    set(I2PD_INCLUDE_DIRS ${I2PD_INCLUDE_DIR})
endif()

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options system)

if(I2PD_FOUND AND NOT TARGET I2PD::i2pd)
    add_library(I2PD::i2pd UNKNOWN IMPORTED)
    set_target_properties(I2PD::i2pd PROPERTIES
        IMPORTED_LOCATION "${I2PD_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${I2PD_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "ZLIB::ZLIB;OpenSSL::SSL;OpenSSL::Crypto;Boost::program_options;Boost::system"
    )
endif()

if(I2PD_FOUND AND NOT TARGET I2PD::i2pdclient)
    add_library(I2PD::i2pdclient UNKNOWN IMPORTED)
    set_target_properties(I2PD::i2pdclient PROPERTIES
        IMPORTED_LOCATION "${I2PD_CLIENT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${I2PD_INCLUDE_DIR}"
        #INTERFACE_LINK_LIBRARIES "ZLIB::ZLIB;OpenSSL::SSL;OpenSSL::Crypto;Boost::program_options;Boost::system"
    )
endif()

if(I2PD_FOUND AND NOT TARGET I2PD::i2pdlang)
    add_library(I2PD::i2pdlang UNKNOWN IMPORTED)
    set_target_properties(I2PD::i2pdlang PROPERTIES
        IMPORTED_LOCATION "${I2PD_LANG_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${I2PD_INCLUDE_DIR}"
        #INTERFACE_LINK_LIBRARIES "ZLIB::ZLIB;OpenSSL::SSL;OpenSSL::Crypto;Boost::program_options;Boost::system"
    )
endif()

mark_as_advanced(
    I2PD_INCLUDE_DIR
    I2PD_LIBRARY
    I2PD_CLIENT_LIBRARY
    I2PD_LANG_LIBRARY
)
project(secp256k1)

include(CheckCXXSourceCompiles)

# TODO currently "arm" is the same as "no"
set(WITH_ASM "auto" CACHE STRING "Specify assembly optimizations to use (default is auto). x86_64|arm|no|auto")
set(WITH_FIELD "auto" CACHE STRING "Specify Field Implementation (default is auto). 64bit|32bit|auto")
set(WITH_SCALAR "auto" CACHE STRING "Specify scalar implementation (default is auto). 64bit|32bit|auto")
set(ENABLE_MODULE_RECOVERY OFF CACHE BOOL "Enable ECDSA pubkey recovery module (default is OFF)")
set(WITH_PIC OFF CACHE BOOL "Enable fPIC")

if(WITH_PIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Setting required options
set(REQ_ASM ${WITH_ASM})
set(REQ_FIELD ${WITH_FIELD})
set(REQ_SCALAR ${WITH_SCALAR})

# It was tried to implement below as it is in secp original configure script for better understanding what is going on
# TODO probably this should be optimized to avoid multiple times checking same things
include(secpchecks.cmake)
if ("${REQ_ASM}" STREQUAL "auto")
    secp_64bit_asm_check()
    if("${HAS_64BIT_ASM}" STREQUAL "yes")
        set(SET_ASM "x86_64")
    endif()
    if(NOT SET_ASM)
        set(SET_ASM "no")
    endif()
else()
    set(SET_ASM "${REQ_ASM}")
    if("${SET_ASM}" STREQUAL "x86_64")
        secp_64bit_asm_check()
        if(NOT "${HAS_64BIT_ASM}" STREQUAL "yes")
            message(FATAL_ERROR "x86_64 assembly optimization requested but not available")
        endif()
    elseif("${SET_ASM}" STREQUAL "arm")
    elseif("${SET_ASM}" STREQUAL "no")
    else()
        message(FATAL_ERROR "invalid assembly optimization selection")
    endif()
endif()

if("${REQ_FIELD}" STREQUAL "auto")
  if("${SET_ASM}" STREQUAL "x86_64")
    set(SET_FIELD "64bit")
  endif()
  if(NOT SET_FIELD)
    secp_int128_check()
    if("${HAS_INT128}" STREQUAL "yes")
      set(SET_FIELD "64bit")
    endif()
  endif()
  if(NOT SET_FIELD)
    set(SET_FIELD "32bit")
  endif()
else()
    set(SET_FIELD ${REQ_FIELD})
    if("${SET_FIELD}" STREQUAL "64bit")
        if(NOT "${SET_ASM}" STREQUAL "x86_64")
            secp_int128_check()
            if(NOT "${HAS_INT128}" STREQUAL "yes")
                message(FATAL_ERROR "64bit field explicitly requested but neither __int128 support or x86_64 assembly available")
            endif()
        endif()
    elseif("${SET_FIELD}" STREQUAL "32bit")
    else()
        message(FATAL_ERROR "invalid field implementation selection")
    endif()
endif()

if("${REQ_SCALAR}" STREQUAL "auto")
    secp_int128_check()
    if("$HAS_INT128" STREQUAL "yes")
        set(SET_SCALAR "64bit")
    endif()
    if(NOT SET_SCALAR)
        set(SET_SCALAR "32bit")
    endif()
else()
    set(SET_SCALAR ${REQ_SCALAR})
    if("${SET_SCALAR}" STREQUAL "64bit")
        secp_int128_check()
        if(NOT "${HAS_INT128}" STREQUAL "yes")
            message(FATAL_ERROR "64bit scalar explicitly requested but __int128 support not available")
        endif()
    elseif("${SET_SCALAR}" STREQUAL "32bit")
    else()
        message(FATAL_ERROR "invalid scalar implementation selected")
    endif()
endif()

if("${SET_ASM}" STREQUAL "x86_64")
    add_compile_definitions(USE_ASM_X86_64=1)
# Ignoring arm here because we do not need it and it can't be deduced to this by corresponding check
elseif("${SET_ASM}" STREQUAL "arm")
elseif("${SET_ASM}" STREQUAL "no")
else()
    message(FATAL_ERROR "invalid assembly optimizations")
endif()

if("${SET_FIELD}" STREQUAL "64bit")
    add_compile_definitions(USE_FIELD_5X52=1)
elseif("${SET_FIELD}" STREQUAL "32bit")
    add_compile_definitions(USE_FIELD_10X26=1)
else()
    message(FATAL_ERROR "invalid field implementation")
endif()

# TODO assume bignum is always equal to no.
# This is because there will be no such option after updating upstream and even pocketnet use this explicitly with "no" value.
add_compile_definitions(USE_NUM_NONE=1 USE_FIELD_INV_BUILTIN=1 USE_SCALAR_INV_BUILTIN=1)

if("${SET_SCALAR}" STREQUAL "64bit")
    add_compile_definitions(USE_SCALAR_4X64=1)
elseif("${SET_SCALAR}" STREQUAL "32bit")
    add_compile_definitions(USE_SCALAR_8X32=1)
else()
    message(FATAL_ERROR "invalid scalar implementation")
endif()

secp_builtin_check()
if (HAVE_BUILTIN)
    add_compile_definitions(HAVE_BUILTIN_EXPECT=1)
endif()

add_library(${PROJECT_NAME} include/secp256k1.h
                            include/secp256k1_preallocated.h
                            src/scalar.h
                            src/scalar_4x64.h
                            src/scalar_8x32.h
                            src/scalar_low.h
                            src/scalar_impl.h
                            src/scalar_4x64_impl.h
                            src/scalar_8x32_impl.h
                            src/scalar_low_impl.h
                            src/group.h
                            src/group_impl.h
                            src/num_gmp.h
                            src/num_gmp_impl.h
                            src/ecdsa.h
                            src/ecdsa_impl.h
                            src/eckey.h
                            src/eckey_impl.h
                            src/ecmult.h
                            src/ecmult_impl.h
                            src/ecmult_const.h
                            src/ecmult_const_impl.h
                            src/ecmult_gen.h
                            src/ecmult_gen_impl.h
                            src/num.h
                            src/num_impl.h
                            src/field_10x26.h
                            src/field_10x26_impl.h
                            src/field_5x52.h
                            src/field_5x52_impl.h
                            src/field_5x52_int128_impl.h
                            src/field_5x52_asm_impl.h
                            src/assumptions.h
                            src/util.h
                            src/testrand.h
                            src/testrand_impl.h
                            src/hash.h
                            src/hash_impl.h
                            src/field.h
                            src/field_impl.h
                            src/bench.h
                            src/scratch.h
                            src/scratch_impl.h
                            src/selftest.h
                            contrib/lax_der_parsing.h
                            contrib/lax_der_parsing.c
                            contrib/lax_der_privatekey_parsing.h
                            contrib/lax_der_privatekey_parsing.c
                            src/secp256k1.c
                            src/modules/extrakeys/main_impl.h
                            include/secp256k1_extrakeys.h
                            src/modules/schnorrsig/main_impl.h
                            include/secp256k1_schnorrsig.h
                            )
if(ENABLE_MODULE_RECOVERY)
    add_compile_definitions(ENABLE_MODULE_RECOVERY=1)
    target_sources(${PROJECT_NAME} PRIVATE include/secp256k1_recovery.h src/modules/recovery/main_impl.h src/modules/recovery/tests_impl.h)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} src/)

add_executable(gen_context src/gen_context.c)
target_include_directories(gen_context PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# TODO there are possible issues here if cross compiling. See https://cmake.org/cmake/help/latest/command/add_custom_command.html
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/ecmult_static_context.h COMMAND gen_context WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(have_ecmult ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/ecmult_static_context.h)
add_dependencies(${PROJECT_NAME} have_ecmult)

target_compile_definitions(${PROJECT_NAME} PRIVATE USE_ECMULT_STATIC_PRECOMPUTATION ENABLE_MODULE_EXTRAKEYS=1 ENABLE_MODULE_SCHNORRSIG=1)

# TODO hardcoded
add_compile_definitions(
                                        ECMULT_GEN_PREC_BITS=4
                                        ECMULT_WINDOW_SIZE=15)
include(CheckCXXSourceCompiles)

# Check if 64bit asm available
function(secp_64bit_asm_check)
    check_cxx_source_compiles(
        "#include <stdint.h>
        int main() {
            uint64_t a = 11, tmp;
            __asm__ __volatile__(\"movq \\@S|@0x100000000,%1; mulq %%rsi\" : \"+a\"(a) : \"S\"(tmp) : \"cc\", \"%rdx\");
        }" HAS_64BIT_ASM_TMP)
    if(HAS_64BIT_ASM_TMP)
        set(HAS_64BIT_ASM "yes" PARENT_SCOPE)
    elseif()
        set(HAS_64BIT_ASM "no" PARENT_SCOPE)
    endif()
endfunction()

# Check if __int128 available
function(secp_int128_check)
    check_cxx_source_compiles(
        "int main() {
        #ifndef __SIZEOF_INT128__
            fail here
        #endif
        }" HAS_INT128_TMP)
    if(HAS_64BIT_ASM_TMP)
        set(HAS_INT128 "yes" PARENT_SCOPE)
    elseif()
        set(HAS_INT128 "no" PARENT_SCOPE)
    endif()
endfunction()

# Check if builtin available
function(secp_builtin_check)
    check_cxx_source_compiles(
        "int main() {
            __builtin_expect(0,0);
        }" HAVE_BUILTIN_TMP)
    if(HAVE_BUILTIN_TMP)
        set(HAVE_BUILTIN 1 PARENT_SCOPE)
    endif()
endfunction()
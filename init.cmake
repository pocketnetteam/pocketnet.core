
function(init package version bug_report tarname url)
message(STATUS ${package} ${version} ${bug_report})
    add_compile_definitions(PACKAGE_NAME="${package}" PACKAGE_VERSION="${version}" PACKAGE_BUGREPORT="${bug_report}" PACKAGE_TARNAME="${tarname}" PACKAGE_URL="${url}")
endfunction()

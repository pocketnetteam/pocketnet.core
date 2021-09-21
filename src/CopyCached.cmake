
if(NOT SRC)
    message(FATAL_ERROR "No source dir specified")
endif()
if(NOT DST)
    message(FATAL_ERROR "No destination dir specified")
endif()

# Converting string with spaces to list
string(REPLACE " " ";" SRC ${SRC}) 

list(LENGTH SRC QM_LENGTH)
message(${QM_LENGTH})

foreach(SINGLE_UNIT ${SRC})
    get_filename_component(SINGLE_UNITNAME ${SINGLE_UNIT} NAME)
    configure_file(${SINGLE_UNIT} ${DST}/${SINGLE_UNITNAME} COPYONLY)
endforeach()

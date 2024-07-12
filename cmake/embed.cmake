# https://stackoverflow.com/a/27206982 CC BY-SA 4.0 Creates C resources file
# from files in given directory
function(create_resources dir prefix output header)
    get_filename_component(header_name ${header} NAME)
    string(TOUPPER ${header_name} header_name_upper)
    string(REPLACE "." "_" header_name_upper ${header_name_upper})
    string(TIMESTAMP timestamp_string "%Y-%m-%d - %H:%M:%S")
    # Create empty output file
    file(WRITE ${header} "")
    file(APPEND ${header} "/*\n * generated: ${timestamp_string}\n */\n")
    file(APPEND ${header} "#ifndef ${header_name_upper}\n")
    file(APPEND ${header} "#define ${header_name_upper}\n")
    file(APPEND ${header} "#include <stddef.h>\n")
    file(APPEND ${header} "#include <stdint.h>\n")
    file(APPEND ${header} "\n")
    file(WRITE ${output} "#include \"${header_name}\"\n")
    # Collect input files
    file(GLOB bins ${dir}/*)
    # Iterate through input files
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${bins})
    foreach (bin ${bins})
        # Get short filename
        string(REGEX MATCH "([^/]+)$" filename ${bin})
        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
        # Read hex data from file
        file(READ ${bin} filedata HEX)
        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
        # Append data to output file
        file(APPEND ${output} "const uint8_t ${prefix}${filename}[] = {${filedata}0x00};\n")
        file(APPEND ${output}
                "const size_t ${prefix}${filename}_size = sizeof(${prefix}${filename});\n")

        # Append data to header file
        file(APPEND ${header} "extern const uint8_t ${prefix}${filename}[];\n")
        file(APPEND ${header} "extern const size_t ${prefix}${filename}_size;\n")
    endforeach ()
    # Close header file
    file(APPEND ${header} "#endif /* ${header_name_upper} */\n")
endfunction()

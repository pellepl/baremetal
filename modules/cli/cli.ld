SECTIONS {

. = __etext + SIZEOF(.data);

.cli_func_entries . (READONLY) : {
    . = ALIGN(4);
    __cli_func_entries_start = .;
    KEEP(*(.cli_func_entries*))
    __cli_func_entries_end = .;
} 

}

SECTIONS {

.text : {
    . = ALIGN(2);
    __cli_func_entries_start = .;
    KEEP(*(.cli_func_entries*))
    __cli_func_entries_end = .;
} > ROM

}

SECTIONS {

.cli_func_entries _sidata + __bss_start__ - _sdata : {
    . = ALIGN(4);
    __cli_func_entries_start = .;
    KEEP(*(.cli_func_entries*))
    __cli_func_entries_end = .;
}

}

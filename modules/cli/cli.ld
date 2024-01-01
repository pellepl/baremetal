SECTIONS {

.cli_func_entries __etext + __data_end__ - __data_start__ : {
    . = ALIGN(4);
    __cli_func_entries_start = .;
    KEEP(*(.cli_func_entries*))
    __cli_func_entries_end = .;
} > FLASH

}

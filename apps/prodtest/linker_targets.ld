SECTIONS {

.target_entries __cli_func_entries_end : {
    . = ALIGN(4);
    __target_entries_start = .;
    KEEP(*(.target_entries*))
    __target_entries_end = .;
} > FLASH

}

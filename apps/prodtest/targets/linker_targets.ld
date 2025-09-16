SECTIONS {

.target_entries : {
    . = ALIGN(4);
    __target_entries_start = .;
    KEEP(*(.target_entries*))
    __target_entries_end = .;
} > FLASH :text

}

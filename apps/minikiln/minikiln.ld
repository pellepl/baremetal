SECTIONS {

.nobss : {
    . = ALIGN(4);
    __nobss_start = .;
    KEEP(*(.nobss*))
    __nobss_end = .;
} 

} INSERT AFTER .bss;

SECTIONS {
.event_cb : {
    . = ALIGN(4);
    __event_cb_start = .;
    KEEP(*(.event_cb*))
    __event_cb_end = .;
}

} INSERT AFTER .rodata;

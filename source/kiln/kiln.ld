SECTIONS {

.nobss : {
    . = ALIGN(4);
    __nobss_start = .;
    KEEP(*(.nobss*))
    __nobss_end = .;
} > RAM

}

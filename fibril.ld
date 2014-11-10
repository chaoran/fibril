SECTIONS
{
  .fibril_tls ALIGN(0x1000) : {
    _fibril_tls_start = ABSOLUTE(.);
    *(.fibril_tls)
    . = ALIGN(0x1000);
    _fibril_tls_end = ABSOLUTE(.);
  }
  .fibril_shm ALIGN(0x1000) : {
    _fibril_shm_start = ABSOLUTE(.);
    *(.fibril_shm)
    . = ALIGN(0x1000);
    _fibril_shm_end = ABSOLUTE(.);
  }
}
INSERT BEFORE .data

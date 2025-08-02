MEMORY
{
  TEXT (rwx) :    ORIGIN = 0x20000000, LENGTH = 256
  DATA (rwx) :    ORIGIN = 0x20000100, LENGTH = 256
}

INCLUDE cortex_m0_minimal.ld

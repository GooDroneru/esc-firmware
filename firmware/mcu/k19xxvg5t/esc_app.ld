/* ESC application — K1921VG5T (NIIET RISC-V)
 *
 * Application is linked right after the 4KB bootloader (app entry = 0x1000).
 * EEPROM (0xF000), device info (0xEFE0) and calibration (0xF800) locations come
 * from the shared esc_sectors.ld, matching the esc-bootloader vg5t layout.
 */

INCLUDE k1921vg5t_app.ld

INCLUDE ../../../dependencies/common/esc/esc_sectors.ld

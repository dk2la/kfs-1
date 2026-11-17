#ifndef VGA_H
#define VGA_H

/* Screen interface: VGA text mode, 80 columns by 25 rows. */

#define VGA_WIDTH	80
#define VGA_HEIGHT	25

/* Blank the whole screen and move the write position to the top left. */
void	vga_clear(void);

/* Write one character at the current position. '\n' starts a new line. */
void	vga_putchar(char c);

/* Write a NUL-terminated string. */
void	vga_write(const char *s);

#endif

#include "vga.h"
#include "string.h"
#include "types.h"

/*
 * In text mode the screen is an array of 16-bit cells mapped at 0xB8000:
 * the low byte is the character, the high byte its colour attribute.
 */
#define VGA_MEMORY	((volatile uint16_t *)0xB8000)

/* Light grey on black. */
#define VGA_ATTR	0x07

static size_t vga_row;
static size_t vga_col;

static uint16_t vga_cell(char c)
{
	return (uint16_t)(uint8_t)c | (uint16_t)VGA_ATTR << 8;
}

static void vga_newline(void)
{
	vga_col = 0;
	/* No scrolling yet: past the last row, start again from the top. */
	if (++vga_row == VGA_HEIGHT)
		vga_row = 0;
}

void vga_clear(void)
{
	size_t i;

	for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
		VGA_MEMORY[i] = vga_cell(' ');
	vga_row = 0;
	vga_col = 0;
}

void vga_putchar(char c)
{
	if (c == '\n') {
		vga_newline();
		return;
	}
	VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = vga_cell(c);
	if (++vga_col == VGA_WIDTH)
		vga_newline();
}

void vga_write(const char *s)
{
	size_t len = strlen(s);
	size_t i;

	for (i = 0; i < len; i++)
		vga_putchar(s[i]);
}

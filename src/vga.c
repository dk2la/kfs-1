#include "vga.h"
#include "io.h"
#include "types.h"

/*
 * In text mode the screen is an array of 16-bit cells mapped at 0xB8000:
 * the low byte is the character, the high byte its colour attribute
 * (background in the high nibble, foreground in the low nibble).
 */
#define VGA_MEMORY	((volatile uint16_t *)0xB8000)
#define VGA_CELLS	(VGA_WIDTH * VGA_HEIGHT)

/* VGA CRT controller: an index port selects a register, a data port holds it. */
#define CRTC_INDEX		0x3D4
#define CRTC_DATA		0x3D5
#define CRTC_CURSOR_START	0x0A
#define CRTC_CURSOR_END		0x0B
#define CRTC_CURSOR_POS_HIGH	0x0E
#define CRTC_CURSOR_POS_LOW	0x0F

/* The cursor is drawn on these scanlines of the 16-line character cell. */
#define CURSOR_FIRST_SCANLINE	14
#define CURSOR_LAST_SCANLINE	15

/*
 * Every screen has a backing copy of its cells. The active screen's cells
 * are also written to VGA memory, so switching only has to copy one way.
 */
struct screen {
	uint16_t	cells[VGA_CELLS];
	size_t		row;
	size_t		col;
	uint8_t		attr;
};

static struct screen screens[VGA_SCREENS];
static struct screen *active = &screens[0];

static uint8_t vga_attr(enum vga_color fg, enum vga_color bg)
{
	return (uint8_t)(bg << 4 | fg);
}

static uint16_t vga_cell(char c, uint8_t attr)
{
	return (uint16_t)((uint16_t)attr << 8 | (uint8_t)c);
}

static void set_cell(size_t index, uint16_t cell)
{
	active->cells[index] = cell;
	VGA_MEMORY[index] = cell;
}

static void crtc_write(uint8_t reg, uint8_t value)
{
	outb(CRTC_INDEX, reg);
	outb(CRTC_DATA, value);
}

static void cursor_enable(void)
{
	crtc_write(CRTC_CURSOR_START, CURSOR_FIRST_SCANLINE);
	crtc_write(CRTC_CURSOR_END, CURSOR_LAST_SCANLINE);
}

static void cursor_update(void)
{
	uint16_t pos = (uint16_t)(active->row * VGA_WIDTH + active->col);

	crtc_write(CRTC_CURSOR_POS_HIGH, (uint8_t)(pos >> 8));
	crtc_write(CRTC_CURSOR_POS_LOW, (uint8_t)pos);
}

static void blank_cells(size_t first, size_t count)
{
	size_t i;

	for (i = first; i < first + count; i++)
		set_cell(i, vga_cell(' ', active->attr));
}

static void scroll(void)
{
	size_t i;

	for (i = 0; i < VGA_CELLS - VGA_WIDTH; i++)
		set_cell(i, active->cells[i + VGA_WIDTH]);
	blank_cells(VGA_CELLS - VGA_WIDTH, VGA_WIDTH);
	active->row = VGA_HEIGHT - 1;
}

static void newline(void)
{
	active->col = 0;
	if (++active->row == VGA_HEIGHT)
		scroll();
}

static void backspace(void)
{
	if (active->col > 0) {
		active->col--;
	} else if (active->row > 0) {
		active->row--;
		active->col = VGA_WIDTH - 1;
	} else {
		return;
	}
	set_cell(active->row * VGA_WIDTH + active->col,
		 vga_cell(' ', active->attr));
}

void vga_init(void)
{
	size_t i;
	size_t j;

	for (i = 0; i < VGA_SCREENS; i++) {
		screens[i].attr = vga_attr(VGA_LIGHT_GREY, VGA_BLACK);
		screens[i].row = 0;
		screens[i].col = 0;
		for (j = 0; j < VGA_CELLS; j++)
			screens[i].cells[j] = vga_cell(' ', screens[i].attr);
	}
	cursor_enable();
	vga_switch_screen(0);
}

void vga_clear(void)
{
	blank_cells(0, VGA_CELLS);
	active->row = 0;
	active->col = 0;
	cursor_update();
}

void vga_set_color(enum vga_color fg, enum vga_color bg)
{
	active->attr = vga_attr(fg, bg);
}

void vga_putchar(char c)
{
	switch (c) {
	case '\n':
		newline();
		break;
	case '\b':
		backspace();
		break;
	default:
		set_cell(active->row * VGA_WIDTH + active->col,
			 vga_cell(c, active->attr));
		if (++active->col == VGA_WIDTH)
			newline();
		break;
	}
	cursor_update();
}

void vga_write(const char *s)
{
	while (*s)
		vga_putchar(*s++);
}

void vga_switch_screen(size_t index)
{
	size_t i;

	if (index >= VGA_SCREENS)
		return;
	active = &screens[index];
	for (i = 0; i < VGA_CELLS; i++)
		VGA_MEMORY[i] = active->cells[i];
	cursor_update();
}

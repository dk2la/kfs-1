#include "keyboard.h"
#include "printk.h"
#include "types.h"
#include "vga.h"

/* One digit per VGA colour, each drawn in that colour. */
static void show_colours(void)
{
	size_t colour;

	vga_write("colours: ");
	for (colour = VGA_BLACK; colour <= VGA_WHITE; colour++) {
		/* Black would be invisible on the black background. */
		vga_set_color(colour,
			      colour == VGA_BLACK ? VGA_LIGHT_GREY : VGA_BLACK);
		printk("%x", colour);
	}
	vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	vga_putchar('\n');
}

static void show_printk(void)
{
	const char *missing = NULL;

	printk("printk: %c %s %d %u %x %p %%\n",
	       'k', "str", -2147483647 - 1, 4294967295u, 0x2a,
	       (void *)0xb8000);
	printk("printk: %s %d\n", missing, 0);
}

/* Give every other screen its own colours and a title. */
static void prepare_screens(void)
{
	static const enum vga_color backgrounds[VGA_SCREENS] = {
		VGA_BLACK, VGA_BLUE, VGA_GREEN, VGA_RED,
	};
	size_t i;

	for (i = 1; i < VGA_SCREENS; i++) {
		vga_switch_screen(i);
		vga_set_color(VGA_WHITE, backgrounds[i]);
		vga_clear();
		printk("screen %u\n", i + 1);
	}
	vga_switch_screen(0);
}

/* Called from _start (src/boot.s) once the stack is set up. */
void kmain(void)
{
	int key;

	vga_init();
	vga_write("42\n");
	show_colours();
	show_printk();
	printk("screen 1 - type something, F1 to F%d switch screens\n",
	       VGA_SCREENS);
	prepare_screens();

	for (;;) {
		key = keyboard_read();
		if (key >= KEY_F1 && key <= KEY_F4)
			vga_switch_screen((size_t)(key - KEY_F1));
		else
			vga_putchar((char)key);
	}
}

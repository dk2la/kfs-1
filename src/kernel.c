#include "vga.h"

/* Called from _start (src/boot.s) once the stack is set up. */
void kmain(void)
{
	vga_clear();
	vga_write("42");
}

#include "keyboard.h"
#include "io.h"
#include "types.h"

/* PS/2 controller ports. */
#define PS2_DATA		0x60
#define PS2_STATUS		0x64
#define PS2_STATUS_OUTPUT_FULL	0x01	/* a byte is waiting on PS2_DATA */
#define PS2_STATUS_FROM_MOUSE	0x20	/* that byte comes from the mouse */

/*
 * Scancode set 1, as delivered by the controller. A key press sends the
 * code below; its release sends the same code with the top bit set.
 */
#define SC_RELEASE	0x80
#define SC_EXTENDED	0xE0	/* prefix of two-byte codes (arrows, ...) */
#define SC_LSHIFT	0x2A
#define SC_RSHIFT	0x36
#define SC_CAPSLOCK	0x3A
#define SC_F1		0x3B
#define SC_F4		0x3E

/* Scancode to character, without and with Shift. 0 means no character. */
static const char keymap[] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
	'*', 0, ' ',
};

static const char keymap_shift[] = {
	0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
	'*', 0, ' ',
};

_Static_assert(sizeof(keymap) == sizeof(keymap_shift),
	       "both keymaps must cover the same scancodes");

static int shift_held;
static int caps_lock;
static int extended;

static int is_letter(char c)
{
	return c >= 'a' && c <= 'z';
}

static int translate(uint8_t scancode)
{
	int shifted = shift_held;

	if (scancode >= SC_F1 && scancode <= SC_F4)
		return KEY_F1 + (scancode - SC_F1);
	if (scancode >= sizeof(keymap))
		return KEY_NONE;
	/* Caps Lock only affects letters, and Shift undoes it. */
	if (caps_lock && is_letter(keymap[scancode]))
		shifted = !shifted;
	return shifted ? keymap_shift[scancode] : keymap[scancode];
}

int keyboard_poll(void)
{
	uint8_t status = inb(PS2_STATUS);
	uint8_t scancode;

	if (!(status & PS2_STATUS_OUTPUT_FULL))
		return KEY_NONE;
	scancode = inb(PS2_DATA);
	if (status & PS2_STATUS_FROM_MOUSE)
		return KEY_NONE;

	if (scancode == SC_EXTENDED) {
		extended = 1;
		return KEY_NONE;
	}
	if (extended) {
		/* Second byte of an extended key: none of them is handled. */
		extended = 0;
		return KEY_NONE;
	}

	if (scancode == SC_LSHIFT || scancode == SC_RSHIFT) {
		shift_held = 1;
		return KEY_NONE;
	}
	if (scancode == (SC_LSHIFT | SC_RELEASE) ||
	    scancode == (SC_RSHIFT | SC_RELEASE)) {
		shift_held = 0;
		return KEY_NONE;
	}
	if (scancode == SC_CAPSLOCK) {
		caps_lock = !caps_lock;
		return KEY_NONE;
	}
	if (scancode & SC_RELEASE)
		return KEY_NONE;

	return translate(scancode);
}

int keyboard_read(void)
{
	int key;

	do {
		key = keyboard_poll();
	} while (key == KEY_NONE);
	return key;
}

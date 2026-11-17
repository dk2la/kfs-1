# KFS_1 design

This document records what we are building for the 42 project KFS_1 ("Grub, boot and screen"), how it is built, and how the work is carried out. It was agreed on 2026-11-17 and is the reference for implementation and review.

## Goal

A minimal i386 kernel that GRUB boots and that displays `42` on the screen.

The subject requires:

- a kernel bootable via GRUB
- assembly boot code that carries a Multiboot header and calls the kernel's main function
- kernel code compiled without any host library, linked with our own linker script
- a small kernel library with basic types and functions (`strlen`, `strcmp`, ...)
- an interface between the kernel and the screen
- a Makefile that compiles every source with the right compiler and flags, then links the kernel binary
- a bootable virtual image, with the whole work under 10 MB

## Scope

**In this delivery:** the mandatory part only.

**Deferred:** the five bonuses (scroll and cursor, colours, `printk`, keyboard input, multiple screens). Whether to do them is decided after the mandatory part is merged, and they would arrive as a separate pull request. The subject only grades bonuses when the mandatory part is perfect.

The code is laid out so that later KFS projects (memory, interrupts, processes) can build on it without a rewrite.

## Technical decisions

| Topic | Decision | Reason |
|---|---|---|
| Language | C, with NASM for the boot file | The subject's flags and nearly all OSDev material are in C |
| Architecture | i386, 32-bit protected mode | Mandatory |
| Boot protocol | Multiboot 1 (magic `0x1BADB002`) | Three-word header; QEMU can also load it directly with `-kernel` |
| Compiler | System `gcc -m32` | No cross-compiler to build; one `apt install` for the evaluator |
| Linker | System `ld -m elf_i386` with our own `linker.ld` | The subject forbids the host's linker script, not the host's `ld` |
| Image | ISO built by `grub-mkrescue` | Needs no root or loop devices |
| Screen | VGA text mode, memory at `0xB8000`, 80×25 | Available immediately after GRUB hands over, no driver setup needed |
| Style | Plain kernel-style C: tabs, `snake_case`, one header per module | The 42 Norm is not required for KFS |

`CC`, `LD` and `AS` stay overridable in the Makefile in case a cross-compiler is wanted later.

### Compiler flags

```
-m32 -std=gnu99 -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -nostdlib -nodefaultlibs -Wall -Wextra -Werror
```

`-fno-pie` is needed because Debian and Ubuntu build position-independent code by default, which a kernel linked at a fixed address does not want.

The subject also lists `-fno-exception` and `-fno-rtti`; those apply to C++ only and are not used.

## Boot sequence

1. The firmware starts GRUB from the ISO.
2. GRUB reads `grub.cfg`, loads `kernel.bin`, and finds the Multiboot header in its first 8 KiB.
3. GRUB places the kernel at 1 MiB, switches to 32-bit protected mode, and jumps to `_start`.
4. `_start` (assembly) sets the stack pointer to our own stack and calls `kmain`.
5. `kmain` (C) clears the screen and writes `42` through the screen module.
6. `kmain` returns; `_start` disables interrupts and halts in a loop.

## Layout

```
Makefile
linker.ld            places the Multiboot header first, kernel at 1 MiB
grub.cfg             one menu entry: multiboot /boot/kernel.bin
src/
  boot.s             Multiboot header, stack, _start
  kernel.c           kmain
  vga.c              screen: clear, put character, write string
lib/
  string.c           strlen, strcmp, memset, memcpy
include/
  types.h            fixed-width integer types, size_t
  vga.h
  string.h
docs/
  DESIGN.md          this document
README.md            dependencies, build, run, boot walkthrough
```

Build output goes to `build/` and is ignored by git.

## Build and run

Target platform: Debian or Ubuntu on x86_64, with packages installable through `apt`. Nothing is installed or run on macOS.

Packages: `make`, `nasm`, `gcc`, `gcc-multilib`, `grub-pc-bin`, `grub-common`, `xorriso`, `mtools`, `qemu-system-x86`.

| Command | Effect |
|---|---|
| `make` | Compile and link `kernel.bin` |
| `make iso` | Build the bootable ISO |
| `make run` | Boot the ISO in QEMU, drawing the emulated screen as text in the terminal |
| `make clean` / `make fclean` | Remove objects / remove all build output |
| `make re` | Rebuild from scratch |

### Image size

A default `grub-mkrescue` ISO can approach or exceed the 10 MB limit. The ISO is built with a restricted set of GRUB modules and without locales, themes or fonts, which brings it to about 0.8 MB. Compression is not used: `grub-mkrescue --compress` fails when combined with a restricted module list. The final size is checked as part of verification.

## Turn-in

- The ISO is built by `make iso` and is **not** committed to the GitHub repository (`*.iso` is in `.gitignore`).
- The subject asks for a virtual image to be turned in. If a separate 42 turn-in repository exists, the owner adds one built ISO there before defence.
- The subject PDF is ignored by git.

## How the work is carried out

- **Authorship:** Claude writes all code; the owner reviews at the end.
- **Repository:** the owner creates the GitHub repository with `master` and `develop` and connects this folder. No git action happens before that.
- **Branching:** Claude branches from `develop`, commits and pushes there, merges into `develop`, then opens one pull request from `develop` to `master` for review.
- **Commits:** several small commits, one per step: boot code and linker script, screen driver, library, Makefile and ISO, README.
- **Attribution:** no Claude lines in commit messages or in the pull request; commits use the owner's git identity.

## Verification

Before the pull request is opened, a cloud Linux agent working on the pushed branch must show that:

1. `make re` and `make iso` succeed with no warnings.
2. The kernel binary is a valid Multiboot image (`grub-file --is-x86-multiboot`).
3. The ISO boots in QEMU and the emulated screen shows `42`, with a screenshot kept as proof.
4. The ISO and the repository are each under 10 MB.

Fallback: if the cloud sandbox cannot install the packages, the same checks run in a Linux container through the Docker already present on the owner's Mac. The owner is told before the fallback is used.

## Open items

- Repository creation by the owner (blocks all implementation work).
- Whether the cloud sandbox allows installing QEMU, GRUB and NASM (checked first, once the branch is pushed).
- Whether to do the bonuses (decided after the mandatory part is merged).

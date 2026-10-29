# SPDX-License-Identifier: GPL-2.0-only
# GNU/ELF replacement for the upstream Mach-O end-of-code marker.
.section .text
.align 2
.globl end_of_the_world
end_of_the_world:
.section .note.GNU-stack,"",@progbits

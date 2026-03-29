; Vanguard 8 showcase ROM milestone 0 bootstrap cartridge.
; Implemented against docs/spec/00-overview.md, docs/spec/02-video.md,
; docs/spec/04-io.md, and showcase/docs/milestones/00.md.

        ORG 0x0000

reset_entry:
        di

        ; The current symbol-aware trace surface does not decode OUT (n),A yet.
        ; Keep the first 32 instructions in the documented decoder subset so
        ; milestone-0 trace capture remains usable without widening showcase
        ; scope into emulator debugger work.
        defs 31, 0x00

        ; Palette entry 1 = bright green on VDP-A.
        ld a, 0x01
        out (0x82), a
        ld a, 0x07
        out (0x82), a
        xor a
        out (0x82), a

        ; Keep display blanked and drive the whole screen from backdrop color 1.
        ; This is the narrowest visible state for milestone 0 because it avoids
        ; scene-table setup while still proving VDP palette/register writes.
        ld a, 0x01
        out (0x81), a
        ld a, 0x87
        out (0x81), a

hello_loop:
        jr hello_loop

        defs 0x4000 - $, 0xFF

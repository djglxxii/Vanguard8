; Vanguard 8 showcase ROM milestone 2 title and compositing scenes.
; Implemented against docs/spec/00-overview.md, docs/spec/01-cpu.md,
; docs/spec/02-video.md, docs/spec/04-io.md,
; showcase/docs/showcase-rom-plan.md, and showcase/docs/milestones/02.md.

        ORG 0x0000

    MACRO OUT0_A port, value
        ld a, value
        db 0xED, 0x39, port
    ENDM

    MACRO VDP_REG_A reg, value
        ld a, value
        out (0x81), a
        ld a, 0x80 | reg
        out (0x81), a
    ENDM

    MACRO VDP_REG_B reg, value
        ld a, value
        out (0x85), a
        ld a, 0x80 | reg
        out (0x85), a
    ENDM

    MACRO VDP_PALETTE_A index, rg, blue
        ld a, index
        out (0x82), a
        ld a, rg
        out (0x82), a
        ld a, blue
        out (0x82), a
    ENDM

    MACRO VDP_PALETTE_B index, rg, blue
        ld a, index
        out (0x86), a
        ld a, rg
        out (0x86), a
        ld a, blue
        out (0x86), a
    ENDM

    MACRO VDP_A_FILL address, count, value
        ld a, (address) & 0xFF
        out (0x81), a
        ld a, 0x40 | (((address) >> 8) & 0x3F)
        out (0x81), a
        REPT count
            ld a, value
            out (0x80), a
        ENDR
    ENDM

    MACRO VDP_B_FILL address, count, value
        ld a, (address) & 0xFF
        out (0x85), a
        ld a, 0x40 | (((address) >> 8) & 0x3F)
        out (0x85), a
        REPT count
            ld a, value
            out (0x84), a
        ENDR
    ENDM

    MACRO FILL_A_RECT y, x_byte, width_bytes, height, value
        REPT height, row
            VDP_A_FILL (((y) + row) * 128 + (x_byte)), width_bytes, value
        ENDR
    ENDM

    MACRO FILL_B_RECT y, x_byte, width_bytes, height, value
        REPT height, row
            VDP_B_FILL (((y) + row) * 128 + (x_byte)), width_bytes, value
        ENDR
    ENDM

    MACRO PRT0_CHAIN_HANDLER label_name, next_label, marker
label_name:
        call clear_prt0_flag
        ld hl, next_label
        ld (0x80E4), hl
        ld a, marker
        ld (0x8120), a
        ei
        db 0xED, 0x4D
    ENDM

reset_entry:
        di
        ld sp, 0xFF00

        ; Boot MMU setup from the hardware contract:
        ; CBAR=0x48, CBR=0xF0, BBR=0x04.
        OUT0_A 0x3A, 0x48
        OUT0_A 0x38, 0xF0
        OUT0_A 0x39, 0x04

        ; INT1 / PRT vector table in SRAM at 0x80E0-0x80FF.
        ld a, 0x80
        db 0xED, 0x47              ; LD I,A
        ld hl, int1_handler
        ld (0x80E0), hl
        ld hl, prt0_stage_0
        ld (0x80E4), hl
        ld hl, prt1_handler
        ld (0x80E6), hl
        OUT0_A 0x33, 0xE0          ; IL
        OUT0_A 0x34, 0x02          ; ITC (ITE1)

        ; PRT0 timeout period tuned to hold the title scene for roughly
        ; 120 frames before switching to the compositing scene.
        OUT0_A 0x0C, 0xE8          ; TMDR0L
        OUT0_A 0x0D, 0xFD          ; TMDR0H
        OUT0_A 0x0E, 0xE8          ; RLDR0L
        OUT0_A 0x0F, 0xFD          ; RLDR0H

        ld a, 0x20
        ld (0x8120), a

        ; Bank 0 = title / identity scene.
        OUT0_A 0x39, 0x04
        call 0x4000

        ; Enable PRT0 interrupt + down-count.
        OUT0_A 0x10, 0x11

idle_loop:
        ei
        jp idle_loop

clear_prt0_flag:
        db 0xED, 0x38, 0x10        ; IN0 A,(0x10) ; arm TIF0 clear
        db 0xED, 0x38, 0x0C        ; IN0 A,(0x0C) ; clear TIF0
        ret

int1_handler:
        ld a, 0x55
        ld (0x8102), a
        ei
        db 0xED, 0x4D

prt1_handler:
        ld a, 0x61
        ld (0x8104), a
        ei
        db 0xED, 0x4D

        PRT0_CHAIN_HANDLER prt0_stage_0, prt0_stage_1, 0x30
        PRT0_CHAIN_HANDLER prt0_stage_1, prt0_stage_2, 0x31
        PRT0_CHAIN_HANDLER prt0_stage_2, prt0_stage_3, 0x32
        PRT0_CHAIN_HANDLER prt0_stage_3, prt0_stage_4, 0x33
        PRT0_CHAIN_HANDLER prt0_stage_4, prt0_stage_5, 0x34
        PRT0_CHAIN_HANDLER prt0_stage_5, prt0_stage_6, 0x35
        PRT0_CHAIN_HANDLER prt0_stage_6, prt0_stage_7, 0x36
        PRT0_CHAIN_HANDLER prt0_stage_7, prt0_stage_8, 0x37
        PRT0_CHAIN_HANDLER prt0_stage_8, prt0_stage_9, 0x38
        PRT0_CHAIN_HANDLER prt0_stage_9, prt0_stage_10, 0x39

prt0_stage_10:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00

        ; Bank 1 = clear the title layer and draw the VDP-A overlay.
        OUT0_A 0x39, 0x08
        call 0x4000

        ; Bank 2 = draw the VDP-B reveal panels.
        OUT0_A 0x39, 0x0C
        call 0x4000

        ld hl, prt0_idle
        ld (0x80E4), hl
        ld a, 0x3A
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_idle:
        call clear_prt0_flag
        ld a, 0x3F
        ld (0x8120), a
        ei
        db 0xED, 0x4D

        defs 0x4000 - $, 0xFF

; Bank 0 page, mapped at logical 0x4000 when BBR=0x04.
        ORG 0x4000
title_scene_init:
        ld a, 0x40
        ld (0x8121), a

        ; Both VDPs stay in Graphic 4 / 212-line mode.
        VDP_REG_A 0, 0x06
        VDP_REG_A 1, 0x40
        VDP_REG_A 8, 0x00
        VDP_REG_B 0, 0x06
        VDP_REG_B 1, 0x40
        VDP_REG_B 8, 0x00

        ; Title layer palette: dark slate background with green/mint emblem.
        VDP_PALETTE_A 0x00, 0x02, 0x01
        VDP_PALETTE_A 0x01, 0x07, 0x00
        VDP_PALETTE_A 0x02, 0x27, 0x03

        ; Rear layer palette for the later compositing scene.
        VDP_PALETTE_B 0x00, 0x00, 0x02
        VDP_PALETTE_B 0x01, 0x02, 0x05
        VDP_PALETTE_B 0x02, 0x03, 0x07

        ; Title scene: fixed-ROM badge bars plus a blocky V8 crest.
        FILL_A_RECT 76, 48, 32, 3, 0x22
        FILL_A_RECT 105, 48, 32, 3, 0x22

        ; V crest.
        FILL_A_RECT 81, 51, 2, 4, 0x22
        FILL_A_RECT 81, 59, 2, 4, 0x22
        FILL_A_RECT 85, 52, 2, 4, 0x11
        FILL_A_RECT 85, 58, 2, 4, 0x11
        FILL_A_RECT 89, 53, 2, 4, 0x11
        FILL_A_RECT 89, 57, 2, 4, 0x11
        FILL_A_RECT 93, 54, 2, 4, 0x11
        FILL_A_RECT 93, 56, 2, 4, 0x11
        FILL_A_RECT 97, 55, 2, 4, 0x11
        FILL_A_RECT 101, 54, 4, 4, 0x11

        ; 8 crest.
        FILL_A_RECT 81, 67, 9, 3, 0x22
        FILL_A_RECT 84, 67, 2, 5, 0x11
        FILL_A_RECT 84, 74, 2, 5, 0x11
        FILL_A_RECT 89, 67, 9, 3, 0x11
        FILL_A_RECT 92, 67, 2, 5, 0x11
        FILL_A_RECT 92, 74, 2, 5, 0x11
        FILL_A_RECT 97, 67, 9, 3, 0x11
        FILL_A_RECT 100, 67, 2, 5, 0x11
        FILL_A_RECT 100, 74, 2, 5, 0x11
        FILL_A_RECT 105, 67, 9, 3, 0x11
        ret

        defs 0x8000 - $, 0x00

; Bank 1 page, mapped at logical 0x4000 when BBR=0x08.
        ORG 0x8000
compositing_overlay_init:
        ld a, 0x41
        ld (0x8121), a

        ; Enable VDP-A color-0 transparency for compositing.
        VDP_REG_A 8, 0x20
        VDP_PALETTE_A 0x00, 0x00, 0x00
        VDP_PALETTE_A 0x01, 0x07, 0x00
        VDP_PALETTE_A 0x02, 0x27, 0x03

        ; Clear the earlier title crest area so transparent color 0 can reveal
        ; VDP-B behind it.
        FILL_A_RECT 76, 48, 32, 32, 0x00

        ; VDP-A foreground frame, bridge, and badge.
        FILL_A_RECT 48, 24, 80, 4, 0x11
        FILL_A_RECT 156, 24, 80, 4, 0x11
        FILL_A_RECT 52, 24, 2, 104, 0x11
        FILL_A_RECT 52, 102, 2, 104, 0x11
        FILL_A_RECT 102, 44, 40, 4, 0x22
        FILL_A_RECT 60, 54, 20, 2, 0x22
        FILL_A_RECT 62, 54, 20, 8, 0x11
        FILL_A_RECT 70, 54, 20, 2, 0x22
        ret

        defs 0xC000 - $, 0x00

; Bank 2 page, mapped at logical 0x4000 when BBR=0x0C.
        ORG 0xC000
compositing_background_init:
        ld a, 0x42
        ld (0x8122), a

        ; VDP-B background: deep blue field with two reveal panels and a
        ; cyan-linked center band.
        VDP_REG_B 8, 0x00
        VDP_PALETTE_B 0x00, 0x00, 0x02
        VDP_PALETTE_B 0x01, 0x02, 0x05
        VDP_PALETTE_B 0x02, 0x03, 0x07
        FILL_B_RECT 64, 30, 12, 64, 0x11
        FILL_B_RECT 64, 86, 12, 64, 0x22
        FILL_B_RECT 92, 50, 28, 8, 0x21
        ret

        defs 0x10000 - $, 0x00

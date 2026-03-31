; Vanguard 8 showcase ROM milestone 4 title, compositing, tile, sprite,
; mixed-mode, and Graphic 4 command-engine scenes.
; Implemented against docs/spec/00-overview.md, docs/spec/01-cpu.md,
; docs/spec/02-video.md, docs/spec/04-io.md,
; showcase/docs/showcase-rom-plan.md, and showcase/docs/milestones/04.md.

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

    MACRO VDP_A_WRITE address, value
        ld a, (address) & 0xFF
        out (0x81), a
        ld a, 0x40 | (((address) >> 8) & 0x3F)
        out (0x81), a
        ld a, value
        out (0x80), a
    ENDM

    MACRO VDP_B_WRITE address, value
        ld a, (address) & 0xFF
        out (0x85), a
        ld a, 0x40 | (((address) >> 8) & 0x3F)
        out (0x85), a
        ld a, value
        out (0x84), a
    ENDM

    MACRO G2_A_PATTERN_ALL_BANKS pattern, row0, row1, row2, row3, row4, row5, row6, row7
        REPT 3, bank
            VDP_A_WRITE ((bank * 0x0800) + ((pattern) * 8) + 0), row0
            VDP_A_WRITE ((bank * 0x0800) + ((pattern) * 8) + 1), row1
            VDP_A_WRITE ((bank * 0x0800) + ((pattern) * 8) + 2), row2
            VDP_A_WRITE ((bank * 0x0800) + ((pattern) * 8) + 3), row3
            VDP_A_WRITE ((bank * 0x0800) + ((pattern) * 8) + 4), row4
            VDP_A_WRITE ((bank * 0x0800) + ((pattern) * 8) + 5), row5
            VDP_A_WRITE ((bank * 0x0800) + ((pattern) * 8) + 6), row6
            VDP_A_WRITE ((bank * 0x0800) + ((pattern) * 8) + 7), row7
        ENDR
    ENDM

    MACRO G2_A_COLOR_ALL_BANKS pattern, color_byte
        REPT 3, bank
            VDP_A_FILL (0x2000 + (bank * 0x0800) + ((pattern) * 8)), 8, color_byte
        ENDR
    ENDM

    MACRO G3_A_PATTERN_ALL_BANKS pattern, row0, row1, row2, row3, row4, row5, row6, row7
        REPT 3, bank
            VDP_A_WRITE (0x0300 + (bank * 0x0800) + ((pattern) * 8) + 0), row0
            VDP_A_WRITE (0x0300 + (bank * 0x0800) + ((pattern) * 8) + 1), row1
            VDP_A_WRITE (0x0300 + (bank * 0x0800) + ((pattern) * 8) + 2), row2
            VDP_A_WRITE (0x0300 + (bank * 0x0800) + ((pattern) * 8) + 3), row3
            VDP_A_WRITE (0x0300 + (bank * 0x0800) + ((pattern) * 8) + 4), row4
            VDP_A_WRITE (0x0300 + (bank * 0x0800) + ((pattern) * 8) + 5), row5
            VDP_A_WRITE (0x0300 + (bank * 0x0800) + ((pattern) * 8) + 6), row6
            VDP_A_WRITE (0x0300 + (bank * 0x0800) + ((pattern) * 8) + 7), row7
        ENDR
    ENDM

    MACRO G3_A_COLOR_ALL_BANKS pattern, color_byte
        REPT 3, bank
            VDP_A_FILL (0x1800 + (bank * 0x0800) + ((pattern) * 8)), 8, color_byte
        ENDR
    ENDM

    MACRO G3_TILE_ROW_FILL row, col, width, pattern
        VDP_A_FILL (((row) * 32) + (col)), width, pattern
    ENDM

    MACRO G3_TILE_PUT row, col, pattern
        VDP_A_WRITE (((row) * 32) + (col)), pattern
    ENDM

    MACRO TILE_ROW_FILL row, col, width, pattern
        VDP_A_FILL (0x1800 + ((row) * 32) + (col)), width, pattern
    ENDM

    MACRO TILE_PUT row, col, pattern
        VDP_A_WRITE (0x1800 + ((row) * 32) + (col)), pattern
    ENDM

    MACRO VDP_CMD_A_HMMV x, y, width_bytes, height, value
        VDP_REG_A 36, ((x) & 0xFF)
        VDP_REG_A 37, (((x) >> 8) & 0x03)
        VDP_REG_A 38, ((y) & 0xFF)
        VDP_REG_A 39, (((y) >> 8) & 0x03)
        VDP_REG_A 40, ((width_bytes) & 0xFF)
        VDP_REG_A 41, (((width_bytes) >> 8) & 0x03)
        VDP_REG_A 42, ((height) & 0xFF)
        VDP_REG_A 43, (((height) >> 8) & 0x03)
        VDP_REG_A 44, value
        VDP_REG_A 45, 0x00
        VDP_REG_A 46, 0xC0
    ENDM

    MACRO VDP_CMD_B_HMMV x, y, width_bytes, height, value
        VDP_REG_B 36, ((x) & 0xFF)
        VDP_REG_B 37, (((x) >> 8) & 0x03)
        VDP_REG_B 38, ((y) & 0xFF)
        VDP_REG_B 39, (((y) >> 8) & 0x03)
        VDP_REG_B 40, ((width_bytes) & 0xFF)
        VDP_REG_B 41, (((width_bytes) >> 8) & 0x03)
        VDP_REG_B 42, ((height) & 0xFF)
        VDP_REG_B 43, (((height) >> 8) & 0x03)
        VDP_REG_B 44, value
        VDP_REG_B 45, 0x00
        VDP_REG_B 46, 0xC0
    ENDM

    MACRO VDP_CMD_A_HMMM sx, sy, dx, dy, width_bytes, height
        VDP_REG_A 32, ((sx) & 0xFF)
        VDP_REG_A 33, (((sx) >> 8) & 0x03)
        VDP_REG_A 34, ((sy) & 0xFF)
        VDP_REG_A 35, (((sy) >> 8) & 0x03)
        VDP_REG_A 36, ((dx) & 0xFF)
        VDP_REG_A 37, (((dx) >> 8) & 0x03)
        VDP_REG_A 38, ((dy) & 0xFF)
        VDP_REG_A 39, (((dy) >> 8) & 0x03)
        VDP_REG_A 40, ((width_bytes) & 0xFF)
        VDP_REG_A 41, (((width_bytes) >> 8) & 0x03)
        VDP_REG_A 42, ((height) & 0xFF)
        VDP_REG_A 43, (((height) >> 8) & 0x03)
        VDP_REG_A 45, 0x00
        VDP_REG_A 46, 0xD0
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

wait_vdp_a_command_clear:
        VDP_REG_A 15, 0x02
.wait:
        in a, (0x81)
        and 0x01
        jr nz, .wait
        VDP_REG_A 15, 0x00
        ret

wait_vdp_b_command_clear:
        VDP_REG_B 15, 0x02
.wait:
        in a, (0x85)
        and 0x01
        jr nz, .wait
        VDP_REG_B 15, 0x00
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

        ; Bank 1 = clear the title layer and draw the VDP-A overlay.
        OUT0_A 0x39, 0x08
        call 0x4000

        ; Bank 2 = draw the VDP-B reveal panels.
        OUT0_A 0x39, 0x0C
        call 0x4000

        ld hl, prt0_stage_11
        ld (0x80E4), hl
        ld a, 0x3A
        ld (0x8120), a
        ei
        db 0xED, 0x4D

        PRT0_CHAIN_HANDLER prt0_stage_11, prt0_stage_12, 0x3B
        PRT0_CHAIN_HANDLER prt0_stage_12, prt0_stage_13, 0x3C
        PRT0_CHAIN_HANDLER prt0_stage_13, prt0_stage_14, 0x3D
        PRT0_CHAIN_HANDLER prt0_stage_14, prt0_stage_15, 0x3E
        PRT0_CHAIN_HANDLER prt0_stage_15, prt0_stage_16, 0x3F
        PRT0_CHAIN_HANDLER prt0_stage_16, prt0_stage_17, 0x40
        PRT0_CHAIN_HANDLER prt0_stage_17, prt0_stage_18, 0x41
        PRT0_CHAIN_HANDLER prt0_stage_18, prt0_stage_19, 0x42
        PRT0_CHAIN_HANDLER prt0_stage_19, prt0_stage_20, 0x43

prt0_stage_20:
        call clear_prt0_flag

        call tile_scene_init_fixed

        ld hl, prt0_stage_21
        ld (0x80E4), hl
        ld a, 0x44
        ld (0x8120), a
        ei
        db 0xED, 0x4D

        PRT0_CHAIN_HANDLER prt0_stage_21, prt0_stage_22, 0x45
        PRT0_CHAIN_HANDLER prt0_stage_22, prt0_stage_23, 0x46
        PRT0_CHAIN_HANDLER prt0_stage_23, prt0_stage_24, 0x47
        PRT0_CHAIN_HANDLER prt0_stage_24, prt0_stage_25, 0x48
        PRT0_CHAIN_HANDLER prt0_stage_25, prt0_stage_26, 0x49
        PRT0_CHAIN_HANDLER prt0_stage_26, prt0_stage_27, 0x4A
        PRT0_CHAIN_HANDLER prt0_stage_27, prt0_stage_28, 0x4B
        PRT0_CHAIN_HANDLER prt0_stage_28, prt0_stage_29, 0x4C
        PRT0_CHAIN_HANDLER prt0_stage_29, prt0_stage_30, 0x4D

prt0_stage_30:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00

        OUT0_A 0x39, 0x10
        call 0x4000

        OUT0_A 0x10, 0x11
        ld hl, prt0_stage_31
        ld (0x80E4), hl
        ld a, 0x4E
        ld (0x8120), a
        ei
        db 0xED, 0x4D

        PRT0_CHAIN_HANDLER prt0_stage_31, prt0_stage_32, 0x4F
        PRT0_CHAIN_HANDLER prt0_stage_32, prt0_stage_33, 0x50
        PRT0_CHAIN_HANDLER prt0_stage_33, prt0_stage_34, 0x51
        PRT0_CHAIN_HANDLER prt0_stage_34, prt0_stage_35, 0x52
        PRT0_CHAIN_HANDLER prt0_stage_35, prt0_stage_36, 0x53
        PRT0_CHAIN_HANDLER prt0_stage_36, prt0_stage_37, 0x54
        PRT0_CHAIN_HANDLER prt0_stage_37, prt0_stage_38, 0x55
        PRT0_CHAIN_HANDLER prt0_stage_38, prt0_stage_39, 0x56
        PRT0_CHAIN_HANDLER prt0_stage_39, prt0_stage_40, 0x57

prt0_stage_40:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x34, 0x00

        OUT0_A 0x39, 0x14
        call 0x4000

        OUT0_A 0x10, 0x11
        ld hl, prt0_stage_44
        ld (0x80E4), hl
        ld a, 0x5B
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_41:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x14
        call 0x6000
        OUT0_A 0x10, 0x11
        ld hl, prt0_stage_42
        ld (0x80E4), hl
        ld a, 0x59
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_42:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x14
        call 0x7000
        OUT0_A 0x10, 0x11
        ld hl, prt0_stage_43
        ld (0x80E4), hl
        ld a, 0x5A
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_43:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x14
        call 0x7800
        OUT0_A 0x10, 0x11
        ld hl, prt0_stage_44
        ld (0x80E4), hl
        ld a, 0x5B
        ld (0x8120), a
        ei
        db 0xED, 0x4D

        PRT0_CHAIN_HANDLER prt0_stage_44, prt0_stage_45, 0x5C
        PRT0_CHAIN_HANDLER prt0_stage_45, prt0_stage_46, 0x5D
        PRT0_CHAIN_HANDLER prt0_stage_46, prt0_stage_47, 0x5E
        PRT0_CHAIN_HANDLER prt0_stage_47, prt0_stage_48, 0x5F
        PRT0_CHAIN_HANDLER prt0_stage_48, prt0_stage_49, 0x60
        PRT0_CHAIN_HANDLER prt0_stage_49, prt0_stage_50, 0x61

prt0_stage_50:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00

        OUT0_A 0x39, 0x28
        call 0x4000

        OUT0_A 0x10, 0x11
        ld hl, prt0_stage_51
        ld (0x80E4), hl
        ld a, 0x62
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_51:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x4400
        OUT0_A 0x10, 0x11
        ld hl, prt0_stage_52
        ld (0x80E4), hl
        ld a, 0x63
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_52:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x4800
        OUT0_A 0x10, 0x11
        ld hl, prt0_stage_53
        ld (0x80E4), hl
        ld a, 0x64
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_53:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x4C00
        OUT0_A 0x10, 0x11

        ld hl, prt0_stage_54
        ld (0x80E4), hl
        ld a, 0x65
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_54:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x5000
        OUT0_A 0x10, 0x11

        ld hl, prt0_stage_55
        ld (0x80E4), hl
        ld a, 0x66
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_55:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x5400
        OUT0_A 0x10, 0x11

        ld hl, prt0_stage_56
        ld (0x80E4), hl
        ld a, 0x67
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_56:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x5800
        OUT0_A 0x10, 0x11

        ld hl, prt0_stage_57
        ld (0x80E4), hl
        ld a, 0x68
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_57:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x5C00
        OUT0_A 0x10, 0x11

        ld hl, prt0_stage_58
        ld (0x80E4), hl
        ld a, 0x69
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_58:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x6000
        OUT0_A 0x10, 0x11

        ld hl, prt0_stage_59
        ld (0x80E4), hl
        ld a, 0x6A
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_stage_59:
        call clear_prt0_flag
        OUT0_A 0x10, 0x00
        OUT0_A 0x39, 0x38
        call 0x6400

        ld hl, prt0_idle
        ld (0x80E4), hl
        ld a, 0x6B
        ld (0x8120), a
        ei
        db 0xED, 0x4D

prt0_idle:
        call clear_prt0_flag
        ld a, 0x3F
        ld (0x8120), a
        ei
        db 0xED, 0x4D

tile_scene_init_fixed:
        ld a, 0x43
        ld (0x8123), a

        ; VDP-A enters Graphic 2 with Sprite Mode 1. VDP-B is disabled so the
        ; menu scene is an unambiguous single-layer tile checkpoint.
        VDP_REG_A 0, 0x02
        VDP_REG_A 1, 0x40
        VDP_REG_A 2, 0x06
        VDP_REG_A 3, 0x80
        VDP_REG_A 4, 0x00
        VDP_REG_A 5, 0x78
        VDP_REG_A 6, 0x07
        VDP_REG_A 7, 0x01
        VDP_REG_A 8, 0x00
        VDP_REG_A 11, 0x00

        VDP_REG_B 0, 0x06
        VDP_REG_B 1, 0x00
        VDP_REG_B 7, 0x00

        VDP_PALETTE_A 0x00, 0x00, 0x00
        VDP_PALETTE_A 0x01, 0x01, 0x03
        VDP_PALETTE_A 0x02, 0x02, 0x04
        VDP_PALETTE_A 0x03, 0x04, 0x05
        VDP_PALETTE_A 0x04, 0x06, 0x02
        VDP_PALETTE_A 0x05, 0x07, 0x01
        VDP_PALETTE_A 0x06, 0x07, 0x07

        ; Pattern set repeated across the three 64-line Graphic 2 banks.
        G2_A_PATTERN_ALL_BANKS 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        G2_A_PATTERN_ALL_BANKS 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        G2_A_PATTERN_ALL_BANKS 2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        G2_A_PATTERN_ALL_BANKS 3, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF
        G2_A_PATTERN_ALL_BANKS 4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
        G2_A_PATTERN_ALL_BANKS 5, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
        G2_A_PATTERN_ALL_BANKS 6, 0x18, 0x3C, 0x7E, 0xDB, 0xDB, 0x7E, 0x3C, 0x18
        G2_A_PATTERN_ALL_BANKS 7, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
        G2_A_PATTERN_ALL_BANKS 8, 0xFF, 0x99, 0xBD, 0x81, 0x81, 0xBD, 0x99, 0xFF

        G2_A_COLOR_ALL_BANKS 0, 0x21
        G2_A_COLOR_ALL_BANKS 1, 0x32
        G2_A_COLOR_ALL_BANKS 2, 0x13
        G2_A_COLOR_ALL_BANKS 3, 0x41
        G2_A_COLOR_ALL_BANKS 4, 0x41
        G2_A_COLOR_ALL_BANKS 5, 0x41
        G2_A_COLOR_ALL_BANKS 6, 0x61
        G2_A_COLOR_ALL_BANKS 7, 0x52
        G2_A_COLOR_ALL_BANKS 8, 0x63

        TILE_ROW_FILL 0, 0, 32, 0
        TILE_ROW_FILL 1, 0, 32, 0
        TILE_ROW_FILL 2, 0, 32, 0
        TILE_ROW_FILL 3, 0, 32, 0
        TILE_ROW_FILL 4, 0, 32, 0
        TILE_ROW_FILL 5, 0, 32, 0
        TILE_ROW_FILL 6, 0, 32, 0
        TILE_ROW_FILL 7, 0, 32, 0
        TILE_ROW_FILL 8, 0, 32, 0
        TILE_ROW_FILL 9, 0, 32, 0
        TILE_ROW_FILL 10, 0, 32, 0
        TILE_ROW_FILL 11, 0, 32, 0
        TILE_ROW_FILL 12, 0, 32, 0
        TILE_ROW_FILL 13, 0, 32, 0
        TILE_ROW_FILL 14, 0, 32, 0
        TILE_ROW_FILL 15, 0, 32, 0
        TILE_ROW_FILL 16, 0, 32, 0
        TILE_ROW_FILL 17, 0, 32, 0
        TILE_ROW_FILL 18, 0, 32, 0
        TILE_ROW_FILL 19, 0, 32, 0
        TILE_ROW_FILL 20, 0, 32, 0
        TILE_ROW_FILL 21, 0, 32, 0
        TILE_ROW_FILL 22, 0, 32, 0
        TILE_ROW_FILL 23, 0, 32, 0

        TILE_ROW_FILL 1, 4, 24, 2
        TILE_ROW_FILL 2, 4, 24, 2
        TILE_ROW_FILL 3, 4, 24, 2
        TILE_ROW_FILL 6, 6, 20, 3
        TILE_ROW_FILL 18, 6, 20, 3

        TILE_PUT 7, 5, 4
        TILE_PUT 7, 26, 5
        TILE_ROW_FILL 7, 6, 20, 1
        TILE_PUT 8, 5, 4
        TILE_PUT 8, 26, 5
        TILE_ROW_FILL 8, 6, 20, 1
        TILE_PUT 9, 5, 4
        TILE_PUT 9, 26, 5
        TILE_ROW_FILL 9, 6, 20, 1
        TILE_PUT 10, 5, 4
        TILE_PUT 10, 26, 5
        TILE_ROW_FILL 10, 6, 20, 1
        TILE_PUT 11, 5, 4
        TILE_PUT 11, 26, 5
        TILE_ROW_FILL 11, 6, 20, 1
        TILE_PUT 12, 5, 4
        TILE_PUT 12, 26, 5
        TILE_ROW_FILL 12, 6, 20, 1
        TILE_PUT 13, 5, 4
        TILE_PUT 13, 26, 5
        TILE_ROW_FILL 13, 6, 20, 1
        TILE_PUT 14, 5, 4
        TILE_PUT 14, 26, 5
        TILE_ROW_FILL 14, 6, 20, 1
        TILE_PUT 15, 5, 4
        TILE_PUT 15, 26, 5
        TILE_ROW_FILL 15, 6, 20, 1
        TILE_PUT 16, 5, 4
        TILE_PUT 16, 26, 5
        TILE_ROW_FILL 16, 6, 20, 1
        TILE_PUT 17, 5, 4
        TILE_PUT 17, 26, 5
        TILE_ROW_FILL 17, 6, 20, 1

        TILE_ROW_FILL 9, 9, 15, 7
        TILE_ROW_FILL 12, 9, 15, 7
        TILE_ROW_FILL 15, 9, 15, 7

        TILE_PUT 2, 7, 6
        TILE_PUT 2, 10, 6
        TILE_PUT 2, 13, 6
        TILE_ROW_FILL 2, 17, 7, 8
        TILE_PUT 8, 8, 8
        TILE_PUT 11, 8, 8
        TILE_PUT 14, 8, 8

        ; Sprite Mode 1 cursor marker for the middle option.
        VDP_A_FILL 0x3800, 8, 0x00
        VDP_A_WRITE 0x3800, 0x18
        VDP_A_WRITE 0x3801, 0x3C
        VDP_A_WRITE 0x3802, 0x7E
        VDP_A_WRITE 0x3803, 0x18
        VDP_A_WRITE 0x3804, 0x18
        VDP_A_WRITE 0x3805, 0x18
        VDP_A_WRITE 0x3806, 0x18
        VDP_A_WRITE 0x3807, 0x00

        VDP_A_WRITE 0x3C00, 96
        VDP_A_WRITE 0x3C01, 56
        VDP_A_WRITE 0x3C02, 0x00
        VDP_A_WRITE 0x3C03, 0x05
        VDP_A_WRITE 0x3C04, 0xD0
        ret

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

; Bank 3 page, mapped at logical 0x4000 when BBR=0x10.
sprite_scene_init_bank3:
        ld a, 0x44
        ld (0x8124), a

        ; VDP-A provides transparent small foreground sprites.
        VDP_REG_A 0, 0x06
        VDP_REG_A 1, 0x40
        VDP_REG_A 5, 0xF8
        VDP_REG_A 6, 0x07
        VDP_REG_A 8, 0x20
        VDP_REG_A 11, 0x00

        ; VDP-B uses magnified 16x16 sprites on a Graphic 4 test field.
        VDP_REG_B 0, 0x06
        VDP_REG_B 1, 0x43
        VDP_REG_B 5, 0xF8
        VDP_REG_B 6, 0x07
        VDP_REG_B 8, 0x00
        VDP_REG_B 11, 0x00

        VDP_PALETTE_A 0x00, 0x00, 0x00
        VDP_PALETTE_A 0x03, 0x07, 0x02
        VDP_PALETTE_A 0x05, 0x07, 0x01
        VDP_PALETTE_A 0x06, 0x07, 0x07
        VDP_PALETTE_A 0x07, 0x03, 0x07

        VDP_PALETTE_B 0x00, 0x00, 0x00
        VDP_PALETTE_B 0x01, 0x01, 0x03
        VDP_PALETTE_B 0x02, 0x03, 0x06
        VDP_PALETTE_B 0x03, 0x05, 0x04
        VDP_PALETTE_B 0x04, 0x07, 0x02
        VDP_PALETTE_B 0x05, 0x07, 0x01

        ; Clear the A background to transparent color 0 and seed a dark B field.
        VDP_CMD_A_HMMV 0, 0, 128, 212, 0x00
        VDP_CMD_B_HMMV 0, 0, 128, 212, 0x11

        ; Two large review pads plus the scanline-limit lane and overlap lane.
        FILL_B_RECT 8, 12, 20, 20, 0x22
        FILL_B_RECT 8, 76, 20, 20, 0x33
        FILL_B_RECT 52, 8, 4, 12, 0x22
        FILL_B_RECT 52, 20, 4, 12, 0x22
        FILL_B_RECT 52, 32, 4, 12, 0x22
        FILL_B_RECT 52, 44, 4, 12, 0x22
        FILL_B_RECT 52, 56, 4, 12, 0x22
        FILL_B_RECT 52, 68, 4, 12, 0x22
        FILL_B_RECT 52, 80, 4, 12, 0x22
        FILL_B_RECT 52, 92, 4, 12, 0x22
        FILL_B_RECT 52, 104, 4, 12, 0x22
        FILL_B_RECT 92, 46, 16, 12, 0x23

        ; Small A sprite patterns.
        VDP_A_WRITE 0x7000, 0x18
        VDP_A_WRITE 0x7001, 0x3C
        VDP_A_WRITE 0x7002, 0x7E
        VDP_A_WRITE 0x7003, 0xFF
        VDP_A_WRITE 0x7004, 0xFF
        VDP_A_WRITE 0x7005, 0x7E
        VDP_A_WRITE 0x7006, 0x3C
        VDP_A_WRITE 0x7007, 0x18

        VDP_A_WRITE 0x7008, 0x10
        VDP_A_WRITE 0x7009, 0x38
        VDP_A_WRITE 0x700A, 0x7C
        VDP_A_WRITE 0x700B, 0xFE
        VDP_A_WRITE 0x700C, 0x7C
        VDP_A_WRITE 0x700D, 0x38
        VDP_A_WRITE 0x700E, 0x10
        VDP_A_WRITE 0x700F, 0x00

        VDP_A_FILL 0x7A00, 8, 0x05
        VDP_A_FILL 0x7A10, 8, 0x05
        VDP_A_FILL 0x7A20, 8, 0x05
        VDP_A_FILL 0x7A30, 8, 0x05
        VDP_A_FILL 0x7A40, 8, 0x05
        VDP_A_FILL 0x7A50, 8, 0x05
        VDP_A_FILL 0x7A60, 8, 0x05
        VDP_A_FILL 0x7A70, 8, 0x05
        VDP_A_FILL 0x7A80, 8, 0x05
        VDP_A_FILL 0x7A90, 8, 0x07
        VDP_A_FILL 0x7AA0, 8, 0x03

        VDP_A_WRITE 0x7C00, 56
        VDP_A_WRITE 0x7C01, 16
        VDP_A_WRITE 0x7C02, 0x00
        VDP_A_WRITE 0x7C03, 0x00
        VDP_A_FILL  0x7C04, 4, 0x00
        VDP_A_WRITE 0x7C08, 56
        VDP_A_WRITE 0x7C09, 40
        VDP_A_WRITE 0x7C0A, 0x00
        VDP_A_WRITE 0x7C0B, 0x00
        VDP_A_FILL  0x7C0C, 4, 0x00
        VDP_A_WRITE 0x7C10, 56
        VDP_A_WRITE 0x7C11, 64
        VDP_A_WRITE 0x7C12, 0x00
        VDP_A_WRITE 0x7C13, 0x00
        VDP_A_FILL  0x7C14, 4, 0x00
        VDP_A_WRITE 0x7C18, 56
        VDP_A_WRITE 0x7C19, 88
        VDP_A_WRITE 0x7C1A, 0x00
        VDP_A_WRITE 0x7C1B, 0x00
        VDP_A_FILL  0x7C1C, 4, 0x00
        VDP_A_WRITE 0x7C20, 56
        VDP_A_WRITE 0x7C21, 112
        VDP_A_WRITE 0x7C22, 0x00
        VDP_A_WRITE 0x7C23, 0x00
        VDP_A_FILL  0x7C24, 4, 0x00
        VDP_A_WRITE 0x7C28, 56
        VDP_A_WRITE 0x7C29, 136
        VDP_A_WRITE 0x7C2A, 0x00
        VDP_A_WRITE 0x7C2B, 0x00
        VDP_A_FILL  0x7C2C, 4, 0x00
        VDP_A_WRITE 0x7C30, 56
        VDP_A_WRITE 0x7C31, 160
        VDP_A_WRITE 0x7C32, 0x00
        VDP_A_WRITE 0x7C33, 0x00
        VDP_A_FILL  0x7C34, 4, 0x00
        VDP_A_WRITE 0x7C38, 56
        VDP_A_WRITE 0x7C39, 184
        VDP_A_WRITE 0x7C3A, 0x00
        VDP_A_WRITE 0x7C3B, 0x00
        VDP_A_FILL  0x7C3C, 4, 0x00
        VDP_A_WRITE 0x7C40, 56
        VDP_A_WRITE 0x7C41, 208
        VDP_A_WRITE 0x7C42, 0x00
        VDP_A_WRITE 0x7C43, 0x00
        VDP_A_FILL  0x7C44, 4, 0x00
        VDP_A_WRITE 0x7C48, 96
        VDP_A_WRITE 0x7C49, 104
        VDP_A_WRITE 0x7C4A, 0x01
        VDP_A_WRITE 0x7C4B, 0x00
        VDP_A_FILL  0x7C4C, 4, 0x00
        VDP_A_WRITE 0x7C50, 96
        VDP_A_WRITE 0x7C51, 108
        VDP_A_WRITE 0x7C52, 0x00
        VDP_A_WRITE 0x7C53, 0x00
        VDP_A_FILL  0x7C54, 4, 0x00
        VDP_A_WRITE 0x7C58, 0xD0

        ; Large B sprite pattern group 0: solid block.
        VDP_B_FILL 0x7000, 32, 0xFF

        ; Large B sprite pattern group 4: hollow block.
        VDP_B_WRITE 0x7020, 0xFF
        VDP_B_WRITE 0x7021, 0x81
        VDP_B_WRITE 0x7022, 0x81
        VDP_B_WRITE 0x7023, 0x81
        VDP_B_WRITE 0x7024, 0x81
        VDP_B_WRITE 0x7025, 0x81
        VDP_B_WRITE 0x7026, 0x81
        VDP_B_WRITE 0x7027, 0xFF
        VDP_B_WRITE 0x7028, 0xFF
        VDP_B_WRITE 0x7029, 0x81
        VDP_B_WRITE 0x702A, 0x81
        VDP_B_WRITE 0x702B, 0x81
        VDP_B_WRITE 0x702C, 0x81
        VDP_B_WRITE 0x702D, 0x81
        VDP_B_WRITE 0x702E, 0x81
        VDP_B_WRITE 0x702F, 0xFF
        VDP_B_WRITE 0x7030, 0xFF
        VDP_B_WRITE 0x7031, 0x81
        VDP_B_WRITE 0x7032, 0x81
        VDP_B_WRITE 0x7033, 0x81
        VDP_B_WRITE 0x7034, 0x81
        VDP_B_WRITE 0x7035, 0x81
        VDP_B_WRITE 0x7036, 0x81
        VDP_B_WRITE 0x7037, 0xFF
        VDP_B_WRITE 0x7038, 0xFF
        VDP_B_WRITE 0x7039, 0x81
        VDP_B_WRITE 0x703A, 0x81
        VDP_B_WRITE 0x703B, 0x81
        VDP_B_WRITE 0x703C, 0x81
        VDP_B_WRITE 0x703D, 0x81
        VDP_B_WRITE 0x703E, 0x81
        VDP_B_WRITE 0x703F, 0xFF

        VDP_B_FILL 0x7A00, 16, 0x02
        VDP_B_FILL 0x7A10, 16, 0x04

        VDP_B_WRITE 0x7C00, 12
        VDP_B_WRITE 0x7C01, 40
        VDP_B_WRITE 0x7C02, 0x00
        VDP_B_WRITE 0x7C03, 0x00
        VDP_B_FILL  0x7C04, 4, 0x00
        VDP_B_WRITE 0x7C08, 12
        VDP_B_WRITE 0x7C09, 168
        VDP_B_WRITE 0x7C0A, 0x04
        VDP_B_WRITE 0x7C0B, 0x00
        VDP_B_FILL  0x7C0C, 4, 0x00
        VDP_B_WRITE 0x7C10, 0xD0
        ret

        defs 0x14000 - $, 0x00

; Bank 4 page, mapped at logical 0x4000 when BBR=0x14.
        ORG 0x14000
mixed_mode_scene_step0_bank4:
        ld a, 0x45
        ld (0x8125), a

        VDP_REG_A 0, 0x04
        VDP_REG_A 1, 0x40
        VDP_REG_A 5, 0x84
        VDP_REG_A 6, 0x06
        VDP_REG_A 7, 0x00
        VDP_REG_A 8, 0x20
        VDP_REG_A 11, 0x00

        VDP_REG_B 0, 0x06
        VDP_REG_B 1, 0x40
        VDP_REG_B 8, 0x00

        VDP_PALETTE_A 0x00, 0x00, 0x00
        VDP_PALETTE_A 0x02, 0x07, 0x02
        VDP_PALETTE_A 0x03, 0x07, 0x01
        VDP_PALETTE_A 0x04, 0x07, 0x07
        VDP_PALETTE_A 0x05, 0x05, 0x03

        VDP_PALETTE_B 0x00, 0x00, 0x00
        VDP_PALETTE_B 0x01, 0x01, 0x03
        VDP_PALETTE_B 0x02, 0x03, 0x06
        VDP_PALETTE_B 0x03, 0x06, 0x03
        VDP_PALETTE_B 0x04, 0x05, 0x05

        VDP_A_WRITE 0x4200, 0xD0
        VDP_B_WRITE 0x7C00, 0xD0
        VDP_CMD_B_HMMV 0, 0, 128, 212, 0x11

        G3_A_PATTERN_ALL_BANKS 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        G3_A_PATTERN_ALL_BANKS 1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        G3_A_PATTERN_ALL_BANKS 2, 0x3C, 0x7E, 0xDB, 0xFF, 0xFF, 0xDB, 0x7E, 0x3C

        G3_A_COLOR_ALL_BANKS 0, 0x00
        G3_A_COLOR_ALL_BANKS 1, 0x20
        G3_A_COLOR_ALL_BANKS 2, 0x40

        G3_TILE_ROW_FILL 1, 4, 24, 2
        G3_TILE_ROW_FILL 5, 4, 24, 1
        G3_TILE_ROW_FILL 6, 4, 2, 1
        G3_TILE_ROW_FILL 6, 26, 2, 1
        G3_TILE_ROW_FILL 7, 4, 2, 1
        G3_TILE_ROW_FILL 7, 26, 2, 1
        G3_TILE_ROW_FILL 8, 4, 2, 1
        G3_TILE_ROW_FILL 8, 26, 2, 1
        G3_TILE_ROW_FILL 9, 4, 2, 1
        G3_TILE_ROW_FILL 9, 26, 2, 1
        G3_TILE_ROW_FILL 10, 4, 2, 1
        G3_TILE_ROW_FILL 10, 26, 2, 1
        G3_TILE_ROW_FILL 11, 4, 2, 1
        G3_TILE_ROW_FILL 11, 26, 2, 1
        G3_TILE_ROW_FILL 12, 4, 2, 1
        G3_TILE_ROW_FILL 12, 26, 2, 1
        G3_TILE_ROW_FILL 13, 4, 24, 1
        ret

        defs 0x16000 - $, 0x00

mixed_mode_scene_step1_bank4:
        VDP_CMD_B_HMMV 28, 52, 72, 64, 0x33

        G3_A_PATTERN_ALL_BANKS 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        G3_A_PATTERN_ALL_BANKS 1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        G3_A_PATTERN_ALL_BANKS 2, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x7E, 0x3C, 0x18
        G3_A_PATTERN_ALL_BANKS 3, 0x7E, 0x42, 0x5A, 0x5A, 0x5A, 0x5A, 0x42, 0x7E
        G3_A_PATTERN_ALL_BANKS 4, 0x3C, 0x7E, 0xDB, 0xFF, 0xFF, 0xDB, 0x7E, 0x3C

        G3_A_COLOR_ALL_BANKS 0, 0x00
        G3_A_COLOR_ALL_BANKS 1, 0x20
        G3_A_COLOR_ALL_BANKS 2, 0x30
        G3_A_COLOR_ALL_BANKS 3, 0x40
        G3_A_COLOR_ALL_BANKS 4, 0x50
        ret

        defs 0x17000 - $, 0x00

mixed_mode_scene_step2_bank4:
        VDP_CMD_B_HMMV 44, 68, 40, 32, 0x44

        G3_TILE_ROW_FILL 1, 4, 24, 3
        G3_TILE_ROW_FILL 2, 4, 24, 3
        G3_TILE_ROW_FILL 5, 4, 24, 1
        G3_TILE_ROW_FILL 6, 4, 2, 1
        G3_TILE_ROW_FILL 6, 26, 2, 1
        G3_TILE_ROW_FILL 7, 4, 2, 1
        G3_TILE_ROW_FILL 7, 26, 2, 1
        G3_TILE_ROW_FILL 8, 4, 2, 1
        G3_TILE_ROW_FILL 8, 26, 2, 1
        G3_TILE_ROW_FILL 9, 4, 2, 1
        G3_TILE_ROW_FILL 9, 26, 2, 1
        G3_TILE_ROW_FILL 10, 4, 2, 1
        G3_TILE_ROW_FILL 10, 26, 2, 1
        ret

        defs 0x17800 - $, 0x00

mixed_mode_scene_step3_bank4:
        G3_TILE_ROW_FILL 11, 4, 2, 1
        G3_TILE_ROW_FILL 11, 26, 2, 1
        G3_TILE_ROW_FILL 12, 4, 2, 1
        G3_TILE_ROW_FILL 12, 26, 2, 1
        G3_TILE_ROW_FILL 13, 4, 2, 1
        G3_TILE_ROW_FILL 13, 26, 2, 1
        G3_TILE_ROW_FILL 14, 4, 2, 1
        G3_TILE_ROW_FILL 14, 26, 2, 1
        G3_TILE_ROW_FILL 15, 4, 2, 1
        G3_TILE_ROW_FILL 15, 26, 2, 1
        G3_TILE_ROW_FILL 16, 4, 2, 1
        G3_TILE_ROW_FILL 16, 26, 2, 1
        G3_TILE_ROW_FILL 17, 4, 24, 1
        G3_TILE_ROW_FILL 8, 7, 6, 2
        G3_TILE_ROW_FILL 13, 7, 6, 2
        G3_TILE_ROW_FILL 10, 19, 6, 1
        G3_TILE_PUT 2, 6, 4
        G3_TILE_PUT 2, 10, 4
        G3_TILE_PUT 2, 14, 4
        G3_TILE_PUT 2, 18, 4
        G3_TILE_PUT 9, 20, 3
        G3_TILE_PUT 12, 20, 3
        ret

        defs 0x18000 - $, 0x00

; Bank 5 page, mapped at logical 0x4000 when BBR=0x28.
        ORG 0x18000
command_scene_step0_bank5:
        ld a, 0x46
        ld (0x8126), a
        VDP_REG_A 0, 0x06
        VDP_REG_A 1, 0x40
        VDP_REG_A 5, 0xF8
        VDP_REG_A 6, 0x07
        VDP_REG_A 8, 0x00
        VDP_REG_A 11, 0x00

        VDP_REG_B 0, 0x06
        VDP_REG_B 1, 0x00
        VDP_REG_B 8, 0x00

        VDP_PALETTE_A 0x00, 0x00, 0x00
        VDP_PALETTE_A 0x01, 0x01, 0x03
        VDP_PALETTE_A 0x02, 0x03, 0x06
        VDP_PALETTE_A 0x03, 0x07, 0x02
        VDP_PALETTE_A 0x04, 0x07, 0x01
        VDP_PALETTE_A 0x05, 0x05, 0x05

        VDP_A_WRITE 0x7C00, 0xD0
        VDP_B_WRITE 0x7C00, 0xD0

        VDP_CMD_A_HMMV 0, 0, 128, 212, 0x11
        ret

        defs 0x18400 - $, 0x00

command_scene_step1_bank5:
        VDP_CMD_A_HMMV 8, 12, 112, 16, 0x22
        ret

        defs 0x18800 - $, 0x00

command_scene_step2_bank5:
        VDP_CMD_A_HMMV 8, 44, 112, 52, 0x12
        ret

        defs 0x18C00 - $, 0x00

command_scene_step3_bank5:
        VDP_CMD_A_HMMV 8, 52, 8, 16, 0x22
        ret

        defs 0x19000 - $, 0x00

command_scene_step4_bank5:
        VDP_CMD_A_HMMV 32, 52, 8, 16, 0x22
        ret

        defs 0x19400 - $, 0x00

command_scene_step5_bank5:
        VDP_CMD_A_HMMV 56, 52, 8, 16, 0x22
        ret

        defs 0x19800 - $, 0x00

command_scene_step6_bank5:
        VDP_CMD_A_HMMV 8, 112, 112, 52, 0x12
        ret

        defs 0x19C00 - $, 0x00

command_scene_step7_bank5:
        VDP_CMD_A_HMMV 8, 120, 8, 16, 0x44
        ret

        defs 0x1A000 - $, 0x00

command_scene_step8_bank5:
        VDP_CMD_A_HMMV 32, 120, 8, 16, 0x44
        ret

        defs 0x1A400 - $, 0x00

command_scene_step9_bank5:
        VDP_CMD_A_HMMV 56, 120, 8, 16, 0x44
        ret

        defs 0x1C000 - $, 0x00

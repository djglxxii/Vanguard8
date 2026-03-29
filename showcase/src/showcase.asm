; Vanguard 8 showcase ROM milestone 1 bring-up cartridge.
; Implemented against docs/spec/00-overview.md, docs/spec/01-cpu.md,
; docs/spec/04-io.md, showcase/docs/showcase-rom-plan.md, and
; showcase/docs/milestones/01.md.

        ORG 0x0000

reset_entry:
        di
        ld sp, 0xFF00

        ; Boot MMU setup from the hardware contract:
        ; CBAR=0x48, CBR=0xF0, BBR=0x04.
        ld a, 0x48
        db 0xED, 0x39, 0x3A        ; OUT0 (0x3A),A
        ld a, 0xF0
        db 0xED, 0x39, 0x38        ; OUT0 (0x38),A
        ld a, 0x04
        db 0xED, 0x39, 0x39        ; OUT0 (0x39),A

        ; INT1/PRT vector table in SRAM at 0x80E0-0x80FF.
        ld a, 0x80
        db 0xED, 0x47              ; LD I,A
        ld hl, int1_handler
        ld (0x80E0), hl
        ld hl, prt0_handler
        ld (0x80E4), hl
        ld hl, prt1_handler
        ld (0x80E6), hl
        ld a, 0xE0
        db 0xED, 0x39, 0x33        ; OUT0 (0x33),A  ; IL
        ld a, 0x02
        db 0xED, 0x39, 0x34        ; OUT0 (0x34),A  ; ITC (ITE1)

        ; Trace-visible bank probe: read the immediate color byte from each
        ; bank page before entering the continuous loop.
        ld a, 0x04
        db 0xED, 0x39, 0x39        ; OUT0 (0x39),A
        ld a, (0x4001)
        ld (0x8100), a
        ld a, 0x08
        db 0xED, 0x39, 0x39        ; OUT0 (0x39),A
        ld a, (0x4001)
        ld (0x8101), a
        ld a, 0x04
        db 0xED, 0x39, 0x39        ; OUT0 (0x39),A

        ; Palette entry 1 = bright green. Palette entry 2 = bright blue.
        ld a, 0x01
        out (0x82), a
        ld a, 0x07
        out (0x82), a
        xor a
        out (0x82), a

        ld a, 0x02
        out (0x82), a
        xor a
        out (0x82), a
        ld a, 0x07
        out (0x82), a

scene_loop:
        call 0x4000
        jp scene_loop

int1_handler:
        ld a, 0x55
        ld (0x8102), a
        ei
        db 0xED, 0x4D              ; RETI

prt0_handler:
        ld a, 0x60
        ld (0x8103), a
        ei
        db 0xED, 0x4D              ; RETI

prt1_handler:
        ld a, 0x61
        ld (0x8104), a
        ei
        db 0xED, 0x4D              ; RETI

        defs 0x4000 - $, 0xFF

; Bank 0 page, mapped at logical 0x4000 when BBR=0x04.
bank0_entry:
        ld a, 0x02
        ld (0x8110), a
        out (0x81), a
        ld a, 0x87
        out (0x81), a
        defs 0x7FFA - $, 0x00
bank0_tail:
        ld a, 0x08
        db 0xED, 0x39, 0x39        ; OUT0 (0x39),A
        ret

; Bank 1 page, mapped at logical 0x4000 when BBR=0x08.
        ORG 0x8000
bank1_entry:
        ld a, 0x01
        ld (0x8111), a
        out (0x81), a
        ld a, 0x87
        out (0x81), a
        defs 0xBFFA - $, 0x00
bank1_tail:
        ld a, 0x04
        db 0xED, 0x39, 0x39        ; OUT0 (0x39),A
        ret

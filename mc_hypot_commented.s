; vim:ft=arm

    .globl _monte_carlo
    .p2align 2
_monte_carlo:
    sub    sp, sp, #80                     ; =80
    stp    d9, d8, [sp, #32]               ; 16-byte Folded Spill
    stp    x20, x19, [sp, #48]             ; 16-byte Folded Spill
    stp    x29, x30, [sp, #64]             ; 16-byte Folded Spill
    add    x29, sp, #64                    ; =64
    mov    x19, x0                     ; x19 = rand_samples
    mov    x20, #32557                 ; x20 = 6364136223846793005ULL
    movk   x20, #19605, lsl #16
    movk   x20, #62509, lsl #32
    movk   x20, #22609, lsl #48
    ucvtf  d1, x0                      ; d1 = rand_samples
    fmul   d8, d1, d0                  ; d8 = r
    fadd   d9, d8, d8                  ; d9 = 2r
    fmov   d0, #1.00000000
    fadd   d0, d9, d0                  ; d0 = rmax
    mov    w0, #-32
    bl     _ldexp                      ; d0 = scale
    add    x8, sp, #16                 ; x8 = &thrd_rngx
    lsr    x10, x8, #3                 ;
    mov    w8, #1                      ;
    mov    w9, #1                      ;
    bfi    x9, x10, #4, #60            ;
    madd   x10, x9, x20, x9            ;
    mov    x11, #56674                 ; x11 = 9039304369631583586
    movk   x11, #36998, lsl #16
    movk   x11, #3950, lsl #32
    movk   x11, #32114, lsl #48
    add    x10, x10, x11               ; x10 = state(x), x9 = inc(x)
    ; stp    x10, x9, [sp, #16]        ; Only address used, no need to store pcg32_random_t
    mov    x12, sp                     ; x12 = &thrd_rngy
    lsr    x12, x12, #3                ;
    bfi    x8, x12, #4, #60            ;
    madd   x12, x8, x20, x8            ;
    add    x11, x12, x11               ; x11 = state(y), x8 = inc(y)
    ; stp    x11, x8, [sp]             ; Only address used, no need to store pcg32_random_t
    mov    x0, #0                      ; x0 = inside
    ; cbz    x19, LBB0_4               ; rand_samples cannot be zero
    fmul   d1, d8, d8                  ; d1 = sqr
    fmov   d2, #4.00000000
    fmul   d2, d1, d2                  ; d2 = 4 * sqr
LBB0_2:                                    ; =>This Inner Loop Header: Depth=1
    lsr    x12, x10, #45               ;
    lsr    x13, x10, #27               ;
    lsr    x14, x10, #59               ;
    madd   x10, x10, x20, x9           ;
    eor    w12, w12, w13               ;
    ror    w12, w12, w14               ;
    ucvtf  d3, w12                     ; d3 = random x value
    fmul   d3, d0, d3                  ; d3 = x_dot
    lsr    x12, x11, #45               ;
    lsr    x13, x11, #27               ; x13 
    lsr    x14, x11, #59               ; x14 = rot
    madd   x11, x11, x20, x8           ;
    eor    w12, w12, w13               ;
    ror    w12, w12, w14               ;
    ucvtf  d4, w12                     ; d4 = random y value
    fmul   d4, d0, d4                  ; d4 = y_dot
    fsub   d5, d8, d3                  ; d5 = r - x_dot
    fsub   d6, d8, d4                  ; d6 = r - y_dot
    fmul   d6, d6, d6                  ; d6 = d6 ** 2
    fmadd  d5, d5, d5, d6              ; d5 = sqd1
    fsub   d3, d9, d3                  ; d3 = 2 * r - x_dot
    fmul   d4, d4, d4                  ; d4 = y_dot ** 2
    fmadd  d3, d3, d3, d4              ; d3 = sqd2
    fcmp   d5, d1
    cset   w12, lt
    fcmp   d3, d2
    cset   w13, ge
    and    w12, w13, w12
    add    x0, x0, x12
    subs   x19, x19, #1                    ; =1
    b.ne   LBB0_2
    str    x10, [sp, #16]
    str    x11, [sp]
LBB0_4:
    ldp    x29, x30, [sp, #64]             ; 16-byte Folded Reload
    ldp    x20, x19, [sp, #48]             ; 16-byte Folded Reload
    ldp    d9, d8, [sp, #32]               ; 16-byte Folded Reload
    add    sp, sp, #80                     ; =80
    ret

    .globl _main
    .p2align 2
_main:
    sub    sp, sp, #80                     ; =80
    stp    x20, x19, [sp, #16]             ; 16-byte Folded Spill
    stp    x29, x30, [sp, #32]             ; 16-byte Folded Spill
    str    d9, [sp, #48]                   ; 16-byte Folded Spill
    add    x29, sp, #48                    ; =48
    fmov   d0, #5.00000000             ; d0 = radius
    fmov   d9, d0
    mov    x0, #16384                  ; construct rand_samples
    movk   x0, #19531, lsl #16
    mov    x20, x0
    bl     _monte_carlo
    mov    x19, x0
    stp    x0, x20, [sp]
    adrp   x0, printf_str1@PAGE
    add    x0, x0, printf_str1@PAGEOFF
    bl     _printf
    ucvtf  d0, x19                     ; d0 = inside <double>
    ucvtf  d1, x20
    fdiv   d0, d0, d1 
    fmov   d1, #4.0
    fmul   d0, d0, d1
    fmul   d0, d0, d9
    fmul   d0, d0, d9                  ; d3 = area
    str    d0, [sp]
    adrp   x0, printf_str2@PAGE
    add    x0, x0, printf_str2@PAGEOFF
    bl     _printf
    mov    w0, #0                      ; return 0
    ldr    d9, [sp, #48]                   ; 16-byte Folded Reload
    ldp    x29, x30, [sp, #32]             ; 16-byte Folded Reload
    ldp    x20, x19, [sp, #16]             ; 16-byte Folded Reload
    add    sp, sp, #80                     ; =80
    ret

    .section    __TEXT,__cstring,cstring_literals
printf_str1:
    .asciz    "%llu/%llu\n"

printf_str2:
    .asciz    "%g\n"

    .section .rodata.logo_png, "a", @progbits
    .balign 4

    .global logo_png_start
    .type logo_png_start, @object
logo_png_start:
    .incbin "other/logo.png"

    .global logo_png_end
    .type logo_png_end, @object
logo_png_end:

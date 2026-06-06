OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(KernelMain)

/*
    VMA: where the kernel thinks it lives at runtime (virtual addresses)
    LMA: where the bootloader physically loads it in RAM

    the bootloader reads p_paddr from each PT_LOAD program header to know
    where to copy each segment. after ExitBootServices the bootloader sets
    up page tables mapping VMA -> LMA before jumping to KernelMain.

    0xFFFFFFFF80000000: standard higher-half kernel VMA, top 2GB of address space
                        required by -mcmodel=kernel
    0x200000:           2MB physical; safely above legacy BIOS/UEFI reserved region.
                        This approach should be fine for the computers we wanna run on uwu
*/

KERNEL_VMA = 0xFFFFFFFF80000000;
KERNEL_LMA = 0x200000;

SECTIONS
{
    . = KERNEL_VMA;

    /*
        AT(KERNEL_LMA): physical load address for the bootloader
        the . += syntax keeps VMA advancing correctly while AT() sets
        where the data actually lives in the file / physical RAM

        each section's LMA = KERNEL_LMA + (VMA_offset_from_base)
        so the physical layout mirrors the virtual layout exactly,
        just shifted down by (KERNEL_VMA - KERNEL_LMA)
    */

    .text ALIGN(4096) : AT(KERNEL_LMA + (ADDR(.text) - KERNEL_VMA))
    {
        VioletTextStart = .;
        *(.text.boot)       /* KernelMain first, matches ENTRY(KernelMain) */
        *(.text .text.*)
        VioletTextEnd = .;
    }

    .rodata ALIGN(4096) : AT(KERNEL_LMA + (ADDR(.rodata) - KERNEL_VMA))
    {
        VioletRodataStart = .;
        *(.rodata .rodata.*)
        VioletRodataEnd = .;
    }

    .data ALIGN(4096) : AT(KERNEL_LMA + (ADDR(.data) - KERNEL_VMA))
    {
        VioletDataStart = .;
        *(.data .data.*)
        VioletDataEnd = .;
    }

    .bss ALIGN(4096) : AT(KERNEL_LMA + (ADDR(.bss) - KERNEL_VMA))
    {
        VioletBssStart = .;
        *(.bss .bss.*)
        *(COMMON)
        VioletBssEnd = .;
    }

    /*
        first byte past the kernel image, page-aligned
        our PMM reads this to know what physical pages the kernel occupies
        so it doesnt hand those pages out as free memory
    */
    VioletKernelEnd = ALIGN(4096);

    /*
        clang emits these even with -ffreestanding so we needa strip them
        .eh_frame / .eh_frame_hdr: C++ exception unwind tables, useless without libunwind
        .comment: compiler version string, don't care
        .note.*: --build-id=none handles most but belt+suspenders(uwu)
    */
    /DISCARD/ :
    {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.comment)
        *(.note.*)
    }
}

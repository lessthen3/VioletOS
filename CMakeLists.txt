##########################################################################
#                        VioletOS v0.0.1
#         Created by Ranyodh Singh Mandur - 💜 2026-2026
#
#              Licensed under the MIT License (MIT).
#         For more details, see the LICENSE file or visit:
#               https://opensource.org/licenses/MIT
#
#         VioletOS is a free open source operating system
##########################################################################

####################################### Enforce CMake Version #######################################

cmake_minimum_required(VERSION 4.2.0) #need this for vs2026

####################################### Config Options #######################################

option(VIOLET_BUILD_BOOTLOADER "build the bootloader" OFF)
option(VIOLET_BUILD_KERNEL     "build kernel OwO"     OFF)
option(VIOLET_BUILD_TESTS      "build tests >w<"      OFF)

####################################### Prevent cmake from trying to link a test binary during config #######################################

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

####################################### Project definition #######################################

project(VioletOS VERSION 0.0.1 DESCRIPTION "Violet OS" LANGUAGES C ASM)

####################################### Enforce C23 Standard #######################################

set(CMAKE_C_STANDARD 23) 
set(CMAKE_C_STANDARD_REQUIRED ON)

####################################### Sanity check #######################################

if(NOT VIOLET_BUILD_BOOTLOADER AND NOT VIOLET_BUILD_KERNEL AND NOT VIOLET_BUILD_TESTS)
    message(FATAL_ERROR
        "[VioletOS]: nothing to build! pass at least one of:\n"
        "  -DVIOLET_BUILD_BOOTLOADER=ON\n"
        "  -DVIOLET_BUILD_KERNEL=ON\n"
        "  -DVIOLET_BUILD_TESTS=ON\n"
    )
endif()

####################################### Pull in arch definitions from toolchain #######################################

include(violet.toolchain.cmake)

####################################### Project folders >w< #######################################

set_property(GLOBAL PROPERTY USE_FOLDERS ON) 
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "third_party/cmake")

####################################### Check For No Config Set #######################################

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "No build type specified, ending build. CMAKE WILL NOW EXIT")
endif()

####################################### Shared freestanding flags #######################################

set(VIOLET_FREESTANDING_FLAGS
    -ffreestanding          # no libc, no CRT, no OS, no bitches (┬┬﹏┬┬)
    -fno-builtin            # dont silently replace memcpy/memset loops with libc calls
    -fno-stack-protector    # __stack_chk_fail doesnt exist, would crash on first fn call
    -fno-pic                # kernel is at a fixed address, GOT/PLT indirection = wrong
    -fno-pie
    -mno-red-zone           # CRITICAL: x86_64 ABI red zone below RSP. interrupts clobber it, without this flag every interrupt = random stack corruption
    -mno-sse                # SSE regs need CR4.OSFXSR set first, so not safe before cpu init
    -mno-sse2               # gcc will happily emit SSE for struct copies otherwise
    -mno-mmx
    -mno-avx
    -Wall
    -Wextra
)

####################################### UEFI include function #######################################

function(apply_uefi_includes fp_Target fp_Visibility)

    if(NOT fp_Visibility STREQUAL "PUBLIC" AND NOT fp_Visibility STREQUAL "PRIVATE" AND NOT fp_Visibility STREQUAL "INTERFACE")
        message(FATAL_ERROR "[Violet] violet_apply_platform_definitions: invalid visibility '${fp_Visibility}', must be PUBLIC, PRIVATE, or INTERFACE")
    endif()

    target_include_directories(${fp_Target} ${fp_Visibility}
        ${PROJECT_SOURCE_DIR}/third_party/edk2
    )

    if(VIOLET_ARCH_X86_64)
        target_include_directories(${fp_Target} ${fp_Visibility}
            ${PROJECT_SOURCE_DIR}/third_party/edk2/X64
        )
    else()
        message(FATAL_ERROR "unsupported ISA, sorry not sorry >w<")
    endif()

endfunction()

####################################### Shared Sources #######################################

file(
    GLOB_RECURSE 
    VIOLET_SHARED_SOURCES
    CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/violet_shared/*.c
    ${PROJECT_SOURCE_DIR}/src/violet_shared/*.h
)

####################################### UEFI Bootloader #######################################

if(VIOLET_BUILD_BOOTLOADER)

    file(
        GLOB_RECURSE 
        VIOLET_BOOTLOADER_SOURCES
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/violet_bootloader/*.c
        ${PROJECT_SOURCE_DIR}/src/violet_bootloader/*.h
        ${PROJECT_SOURCE_DIR}/src/violet_bootloader/*.S
    )

    source_group(
        TREE   "${PROJECT_SOURCE_DIR}/src/violet_bootloader/bootloader"
        PREFIX "bootloader"
        FILES  ${VIOLET_BOOTLOADER_SOURCES}
    )

    add_executable(violet_bootloader 
        ${VIOLET_BOOTLOADER_SOURCES}
        ${VIOLET_SHARED_SOURCES}
    )

    set_target_properties(violet_bootloader PROPERTIES
        OUTPUT_NAME "BOOTX64"   # UEFI fallback path expects this exact name
        SUFFIX      ".efi"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/violet_bootloader"
    )

    target_include_directories(violet_bootloader PRIVATE
        ${PROJECT_SOURCE_DIR}/src/violet_bootloader
        ${PROJECT_SOURCE_DIR}/src/violet_shared        # UwU
    )

    apply_uefi_includes(violet_bootloader PRIVATE)

    target_compile_options(violet_bootloader PRIVATE
        ${VIOLET_FREESTANDING_FLAGS} 

        -target x86_64-pc-win32-coff # PE32+ target: clang handles this natively, uefi uses windows style stuff idk its arbitrary but ye 

        -fshort-wchar # UEFI uses 16 bit wide chars aka UTF-16 or UCS-2

        $<$<CONFIG:Debug>:   -g3 -O0>
        $<$<CONFIG:Release>: -O2>
    )

    # lld is required here, apparently ld.bfd literally cannot produce PE32+ output
    target_link_options(violet_bootloader PRIVATE
        -target x86_64-pc-win32-coff
        -nostdlib
        -Wl,-subsystem:efi_application
        -Wl,-entry:UefiMain
        -fuse-ld=lld
    )

    violet_apply_platform_definitions(violet_bootloader PRIVATE)
    set_target_properties(violet_bootloader PROPERTIES FOLDER "violet/bootloader")

endif(VIOLET_BUILD_BOOTLOADER)


####################################### Kernel ;A; #######################################

if(VIOLET_BUILD_KERNEL)

    ####################################### Set sources

    file(
        GLOB_RECURSE 
        VIOLET_KERNEL_SOURCES 
        CONFIGURE_DEPENDS 
        ${PROJECT_SOURCE_DIR}/src/violet_kernel/*.c
        ${PROJECT_SOURCE_DIR}/src/violet_kernel/*.h
    )

    file(
        GLOB_RECURSE 
        VIOLET_ARCH_SOURCES
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/arch/*.S
        ${PROJECT_SOURCE_DIR}/src/arch/*.c
        ${PROJECT_SOURCE_DIR}/src/arch/*.h
    )

    ####################################### Set source groups uwu

    source_group(
        TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/violet_kernel/kernel"
        PREFIX "kernel"
        FILES ${VIOLET_KERNEL_SOURCES}
    )

    source_group(
        TREE   "${PROJECT_SOURCE_DIR}/src/arch"
        PREFIX "arch"
        FILES  ${VIOLET_ARCH_SOURCES}
    )

    ####################################### kernel target

    add_executable(violet_kernel
        ${VIOLET_KERNEL_SOURCES}
        ${VIOLET_ARCH_SOURCES}
        ${VIOLET_SHARED_SOURCES}
    )

    ####################################### set the kernel output properties

    set_target_properties(violet_kernel PROPERTIES
        OUTPUT_NAME "kernel"
        SUFFIX      ".elf"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/violet_kernel"
    )

    ####################################### do the include thing i always do for projects for include convenience owo

    target_include_directories(violet_kernel PRIVATE
        ${PROJECT_SOURCE_DIR}/src/violet_kernel
        ${PROJECT_SOURCE_DIR}/src/arch
        ${PROJECT_SOURCE_DIR}/src/violet_shared
    )

    apply_uefi_includes(violet_kernel PRIVATE) #we need uefi for GOP fallback in case of a driver or DE crash(or linus) uwu

    ####################################### make sure we don't do anything stupid since we can't libc shit and w/e

    target_compile_options(violet_kernel PRIVATE
        ${VIOLET_FREESTANDING_FLAGS}

        #[[ 
            kernel lives at 0xFFFFFFFF80000000 at the top 2GB of virtual address space
            default "small" code model assumes everything is in low 2GB
            generates truncated 32-bit addresses that point to completely wrong memory
            "kernel" model fixes this and generates proper 64-bit high-address references
        ]]

        -mcmodel=kernel

        -masm=intel # intel syntax asm; way more readable than AT&T garbage uwu

        -fshort-wchar # UEFI uses 16 bit wide chars aka UTF-16 or UCS-2

        $<$<CONFIG:Debug>:   -g3 -O0>
        $<$<CONFIG:Release>: -O2>
    )

    #[[ 
        kernel.ld controls where .text/.data/.bss actually land in virtual memory
        max-page-size=0x1000: without this the linker may pad sections to 2MB alignment
        wasting physical pages between .text and .data for no reason
        --build-id=none: strips .note.gnu.build-id — keeps the ELF clean
    ]]

    target_link_options(violet_kernel PRIVATE
        -nostdlib
        -nostartfiles
        -nodefaultlibs
        -static
        -fuse-ld=lld                             # explicit lld
        -T ${PROJECT_SOURCE_DIR}/violet_kernel.ld
        -Wl,-z,max-page-size=0x1000
        -Wl,--build-id=none
        -W1,-m,elf_x86_64
    )

    violet_apply_platform_definitions(violet_kernel PRIVATE)
    set_target_properties(violet_kernel PROPERTIES FOLDER "violet/kernel")

endif()


####################################### Tests >////< #######################################

if(VIOLET_BUILD_TESTS)

    file(
        GLOB_RECURSE 
        VIOLET_TEST_SOURCES
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/tests/*.c
        ${PROJECT_SOURCE_DIR}/tests/*.h
    )

    ####################################### Set source groups uwu

    source_group(
        TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests"
        PREFIX "tests"
        FILES ${VIOLET_TEST_SOURCES}
    )

    ####################################### tests target uwu

    add_executable(violet_tests 
        ${VIOLET_TEST_SOURCES}
    )

    set_target_properties(violet_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/violet_tests"
    )

    target_include_directories(violet_tests PRIVATE
        ${PROJECT_SOURCE_DIR}/src/violet_shared
        ${PROJECT_SOURCE_DIR}/tests
    )

    target_compile_options(violet_tests PRIVATE -Wall -Wextra -g)
    set_target_properties(violet_tests PROPERTIES FOLDER "violet/tests")

endif()

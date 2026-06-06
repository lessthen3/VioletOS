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

####################################### require target arch to be passed explicitly #######################################

if(NOT DEFINED VIOLET_TARGET_ARCH OR VIOLET_TARGET_ARCH STREQUAL "")
    #annoying cmake try compile shenanigans
    get_filename_component(f_DirName "${CMAKE_BINARY_DIR}" NAME)
    
    if(f_DirName MATCHES "TryCompile" OR CMAKE_IN_TRY_COMPILE)
        return() # cmake internal probe, just bail silently uwu
    endif()

    message(FATAL_ERROR 
        "VIOLET_TARGET_ARCH not specified! daddy needs to know where kitten is running uwu\n"
        "pass -DVIOLET_TARGET_ARCH=<target> where target is one of:\n"
        " x86, x86_64, arm64, arm32, riscv64, riscv32, mips64, mips32"
    )
endif()

####################################### Cache CMake Vars UwU #######################################

set(VIOLET_ARCH_X86_64      OFF CACHE BOOL "" FORCE)
set(VIOLET_ARCH_X86         OFF CACHE BOOL "" FORCE)

set(VIOLET_ARCH_ARM64       OFF CACHE BOOL "" FORCE)
set(VIOLET_ARCH_ARM32       OFF CACHE BOOL "" FORCE)

set(VIOLET_ARCH_RISCV64     OFF CACHE BOOL "" FORCE)
set(VIOLET_ARCH_RISCV32     OFF CACHE BOOL "" FORCE)

set(VIOLET_ARCH_MIPS64      OFF CACHE BOOL "" FORCE)
set(VIOLET_ARCH_MIPS32      OFF CACHE BOOL "" FORCE)

set(VIOLET_64_BIT           OFF CACHE BOOL "" FORCE)
set(VIOLET_32_BIT           OFF CACHE BOOL "" FORCE)

####################################### Set Cached Vars >w< #######################################

if(VIOLET_TARGET_ARCH STREQUAL "x86_64")
    set(VIOLET_ARCH_X86_64       ON CACHE BOOL "" FORCE)
    set(VIOLET_64_BIT            ON CACHE BOOL "" FORCE)

    message(STATUS "[VioletOS]: Mmm, legacy x86_64 architecture detected, kitten! Whatcha gonna do with all those registers? 😏💜")

elseif(VIOLET_TARGET_ARCH STREQUAL "x86")
    set(VIOLET_ARCH_X86          ON CACHE BOOL "" FORCE)
    set(VIOLET_32_BIT            ON CACHE BOOL "" FORCE)

    message(STATUS "[VioletOS]: Oh my... 32-bit x86? Such a tight, restricted little address space... don't overflow your stack on me, architecture-kun~ 🫣💦")

elseif(VIOLET_TARGET_ARCH STREQUAL "arm64")
    set(VIOLET_ARCH_ARM64        ON CACHE BOOL "" FORCE)
    set(VIOLET_64_BIT            ON CACHE BOOL "" FORCE)

    message(STATUS "[VioletOS]: AArch64 detected! Looking submissive and breedable, little silicon slice... uwu 🥺👉👈")

elseif(VIOLET_TARGET_ARCH STREQUAL "arm32")
    set(VIOLET_ARCH_ARM32        ON CACHE BOOL "" FORCE)
    set(VIOLET_32_BIT            ON CACHE BOOL "" FORCE)

    message(STATUS "[VioletOS]: Ahhh~ Little legacy ARM32 baby... begging to be cross-compiled... 🥺 Put me in Thumb mode and choke my pipeline, toolchain-senpai! Absolute bottom tier instruction density owo 🎀🍼 Tag-align my literal pools or I'll leak all over your registers~ 😭💦")

elseif(VIOLET_TARGET_ARCH STREQUAL "riscv64")
    set(VIOLET_ARCH_RISCV64      ON CACHE BOOL "" FORCE)
    set(VIOLET_64_BIT            ON CACHE BOOL "" FORCE)

    message(STATUS "[VioletOS]: RISC-V 64? Omg, an open-source slut for custom extensions! Fill my build tree with your modules, senpai~ 🫣✨")

elseif(VIOLET_TARGET_ARCH STREQUAL "riscv32")
    set(VIOLET_ARCH_RISCV32      ON CACHE BOOL "" FORCE)
    set(VIOLET_32_BIT            ON CACHE BOOL "" FORCE)

    message(STATUS "[VioletOS]: Embedded RISC-V 32... so tiny and helpless... literally just a submissive little soft-core on an FPGA board 🥺💔")

elseif(VIOLET_TARGET_ARCH STREQUAL "mips64")
    set(VIOLET_ARCH_MIPS64       ON CACHE BOOL "" FORCE)
    set(VIOLET_64_BIT            ON CACHE BOOL "" FORCE)

    message(STATUS "[VioletOS]: MIPS64?! Big, dominant retro hardware... please don't load-delay slot your way into my cache lines daddy 😫💥")

elseif(VIOLET_TARGET_ARCH STREQUAL "mips32")
    set(VIOLET_ARCH_MIPS32       ON CACHE BOOL "" FORCE)
    set(VIOLET_32_BIT            ON CACHE BOOL "" FORCE)

    message(STATUS "[VioletOS]: MIPS32... a vintage little boy begging to be emulated. Let me cycle-count your branch delays, pretty please? 🥵🎮")

else()
    message(FATAL_ERROR "what the fuck are you targetting my brother in christ?")
endif()

####################################### Apply Definitions OwO #######################################

function(violet_apply_platform_definitions fp_Target fp_Visibility)
    
    # validate visibility arg
    if(NOT fp_Visibility STREQUAL "PUBLIC" AND NOT fp_Visibility STREQUAL "PRIVATE" AND NOT fp_Visibility STREQUAL "INTERFACE")
        message(FATAL_ERROR "[Violet] violet_apply_platform_definitions: invalid visibility '${fp_Visibility}', must be PUBLIC, PRIVATE, or INTERFACE")
    endif()

    if(VIOLET_ARCH_X86_64)
        target_compile_definitions(${fp_Target} ${fp_Visibility}
            VIOLET_ARCH_X86_64
            VIOLET_64_BIT
        )
    elseif(VIOLET_ARCH_X86)
        target_compile_definitions(${fp_Target} ${fp_Visibility}
            VIOLET_ARCH_X86
            VIOLET_32_BIT
        )
    elseif(VIOLET_ARCH_ARM64)
        target_compile_definitions(${fp_Target} ${fp_Visibility}
            VIOLET_ARCH_ARM64
            VIOLET_64_BIT
        )
    elseif(VIOLET_ARCH_ARM32)
        target_compile_definitions(${fp_Target} ${fp_Visibility}
            VIOLET_ARCH_ARM32
            VIOLET_32_BIT
        )
    elseif(VIOLET_ARCH_RISCV64)
        target_compile_definitions(${fp_Target} ${fp_Visibility}
            VIOLET_ARCH_RISCV64
            VIOLET_64_BIT
        )
    elseif(VIOLET_ARCH_RISCV32)
        target_compile_definitions(${fp_Target} ${fp_Visibility}
            VIOLET_ARCH_RISCV32
            VIOLET_32_BIT
        )
    elseif(VIOLET_ARCH_MIPS64)
        target_compile_definitions(${fp_Target} ${fp_Visibility}
            VIOLET_ARCH_MIPS64
            VIOLET_64_BIT
        )
    elseif(VIOLET_ARCH_MIPS32)
        target_compile_definitions(${fp_Target} ${fp_Visibility}
            VIOLET_ARCH_MIPS32
            VIOLET_32_BIT
        )
    endif()
endfunction()
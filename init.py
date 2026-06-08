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
import subprocess
import os
import argparse
import platform
import shutil
import sys
import re
import glob 
import io

from shutil import which
from enum import Enum

############################################################################## Pretty Text Utility Function UwU ##############################################################################

def create_coloured_text(fp_SampleText: str, fp_DesiredColour: str) -> str:

    fp_DesiredColour = fp_DesiredColour.lower()

    f_ListOfColours = {
        "black": '\033[30m', "red": '\033[31m', "green": '\033[32m',
        "yellow": '\033[33m', "blue": '\033[34m', "magenta": '\033[35m',
        "cyan": '\033[36m', "white": '\033[37m',

        "bright black": '\033[90m', "bright red": '\033[91m', "bright green": '\033[92m',
        "bright yellow": '\033[93m', "bright blue": '\033[94m', "bright magenta": '\033[95m',
        "bright cyan": '\033[96m', "bright white": '\033[97m'
    }

    if fp_DesiredColour not in f_ListOfColours:
        print(create_coloured_text("[Warning]: no valid input detected for create_coloured_text, returned original text in all lower-case", "yellow"))
        return fp_SampleText
    
    else:
        return f"{f_ListOfColours.get(fp_DesiredColour, '')}{fp_SampleText}\033[0m"

############################################################################## Logging functions OwO ##############################################################################

def log_info(fp_Message: str) -> None:
    print(create_coloured_text(f"[INFO]: {fp_Message}", "bright green"))

def log_warning(fp_Message: str) -> None:
    print(create_coloured_text(f"[WARNING]: {fp_Message}", "yellow"))

def log_error(fp_Message: str) -> None:
    print(create_coloured_text(f"[ERROR]: {fp_Message}", "red"))

def log_success(fp_Message: str) -> None:
    print(create_coloured_text(f"[SUCCESS]: {fp_Message}", "cyan"))

def print_tip(fp_Message: str) -> None:
    print(create_coloured_text(f"[TIP]: {fp_Message}", "bright cyan"))

############################################################################## Utility for Validating Required Build Tools ##############################################################################

def ensure_tool_installed(fp_ToolName: str) -> bool:

    if which(fp_ToolName) is None:
        log_error(f"Required tool '{fp_ToolName}' not found in PATH")
        return False
    else:
        return True

############################################################################## Build Session Accumulators ##############################################################################

# populated during the build loop, dumped at the end if flags are set
g_ErrorLog:   dict[str, list[str]] = {}  # dep_name -> [error lines]
g_WarningLog: dict[str, list[str]] = {}  # dep_name -> [warning lines]
g_CurrentDep: str = "VioletOS"           # this is more for build_deps.py but w/e

############################################################################## Compiled Regex Patterns for Build Output Classification ##############################################################################

_g_ErrorPatterns = [
    re.compile(r':\s*error\b',              re.IGNORECASE), # "error:" / ": error" — GCC, Clang, MSVC
    re.compile(r'\bfatal\s+error\b',        re.IGNORECASE), # "fatal error:" — preprocessor, linker
    re.compile(r'\bfailed:\b',              re.IGNORECASE),  # ninja "FAILED: CMakeFiles/..." / MSBuild "Build FAILED, need the : so func tests dont trigger it ;w;"

    re.compile(r'\blnk[12]\d{3}\b',         re.IGNORECASE), # MSVC linker errors: LNK1181, LNK2019 etc
    re.compile(r'\b[Cc][2356789]\d{3}\b',   re.IGNORECASE), # MSVC compiler errors: C2065, C3861 
    re.compile(r'\bld:\s+error\b',          re.IGNORECASE), # GNU ld errors
    re.compile(r'\bundefined\s+symbol\b',   re.IGNORECASE), # linker: undefined symbol
    re.compile(r'\bduplicate\s+symbol\b',   re.IGNORECASE), # linker: duplicate symbol
    re.compile(r'\bundefined\s+reference\b',re.IGNORECASE), # GCC linker variant
    re.compile(r'\breferenced\s+from\b',    re.IGNORECASE), # Apple ld variant
    
    re.compile(r'\bninja:\s+error\b',       re.IGNORECASE), # "ninja: error:" — only ninja errors, not every ninja line
    re.compile(r'\bcommand\s+failed\b',     re.IGNORECASE), # generic CMake command failure
    re.compile(r'cmake\s+error',            re.IGNORECASE), # CMake configure errors

    re.compile(r'\binternal\s+compiler\s+error\b', re.IGNORECASE), # GCC/Clang ICE
]

_g_WarningPatterns = [
    re.compile(r':\s*warning\b',            re.IGNORECASE), # "warning:" / ": warning" — GCC, Clang, MSVC
    re.compile(r'\b[Cc]4\d{3}\b',           re.IGNORECASE), # MSVC warnings are in the 4000s: C4100, C4244 etc
    re.compile(r'\blnk4\d{3}\b',            re.IGNORECASE), # linker warnings should be 4000s as well
    re.compile(r'\bcmake\s+warning\b',      re.IGNORECASE), # CMake configure warnings
]

############################################################################## Run command for live console feed ##############################################################################

"""
    Runs a subprocess command and streams stdout live.
    Errors  → printed red  in real time, collected into g_ErrorLog
    Warnings → printed yellow in real time, collected into g_WarningLog
    Raises CalledProcessError if the command fails.
"""


def run_command_with_live_output(fp_Command, fp_WorkingDirectory=".") -> None:

    global g_ErrorLog, g_WarningLog, g_CurrentDep

    f_Process = subprocess.Popen(
        fp_Command,
        cwd=fp_WorkingDirectory,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        encoding="utf-8",
        errors="replace"
    )

    f_OutputLines = []

    try:
        for line in f_Process.stdout:

            f_Stripped = line.rstrip('\n')

            if any(pat.search(line) for pat in _g_ErrorPatterns):
                sys.stdout.write(create_coloured_text(f_Stripped, "bright red") + '\n')
                if g_CurrentDep:
                    g_ErrorLog.setdefault(g_CurrentDep, []).append(f_Stripped)

            elif any(pat.search(line) for pat in _g_WarningPatterns):
                sys.stdout.write(create_coloured_text(f_Stripped, "yellow") + '\n')
                if g_CurrentDep:
                    g_WarningLog.setdefault(g_CurrentDep, []).append(f_Stripped)

            else:
                sys.stdout.write(line)

            f_OutputLines.append(line)

        f_Process.wait()

        if f_Process.returncode != 0:
            raise subprocess.CalledProcessError(
                f_Process.returncode,
                fp_Command,
                output=''.join(f_OutputLines)
            )

    finally:
        f_Process.stdout.close()

############################################################################## Markdown Summary Dump ##############################################################################

def write_build_summary_markdown(fp_BaseDir: str, fp_PrintErrors: bool, fp_PrintWarnings: bool) -> None:

    if fp_PrintErrors and g_ErrorLog:

        f_ErrorPath = os.path.join(fp_BaseDir, "build_errors.md")

        with open(f_ErrorPath, "w", encoding="utf-8") as f_File:
            f_File.write("# Peach-E Build Error Summary\n\n")

            for lv_Dep, lv_Lines in g_ErrorLog.items():
                f_File.write(f"## `{lv_Dep}`\n\n")
                f_File.write("```\n")
                for lv_Line in lv_Lines:
                    f_File.write(lv_Line + "\n")
                f_File.write("```\n\n")

        log_info(f"\nError summary written to {f_ErrorPath}")

    if fp_PrintWarnings and g_WarningLog:

        f_WarnPath = os.path.join(fp_BaseDir, "build_warnings.md")

        with open(f_WarnPath, "w", encoding="utf-8") as f_File:
            f_File.write("# Peach-E Build Warning Summary\n\n")

            for lv_Dep, lv_Lines in g_WarningLog.items():
                f_File.write(f"## `{lv_Dep}`\n\n")
                f_File.write("```\n")
                for lv_Line in lv_Lines:
                    f_File.write(lv_Line + "\n")
                f_File.write("```\n\n")

        log_info(f"Warning summary written to {f_WarnPath}")

############################################################################## convert /mnt/ -> ntfs ##############################################################################

def wsl_path_to_windows_ntfs(fp_WslPath: str) -> str: # /mnt/x/foo/bar → X:\foo\bar
    
    f_Match = re.match(r'^/mnt/([a-zA-Z])(/.*)$', fp_WslPath)

    if f_Match:
        f_Drive  = f_Match.group(1).upper()
        f_Rest   = f_Match.group(2).replace("/", "\\")
        return f"{f_Drive}:{f_Rest}"
    
    return ""  # not a /mnt/ path, return an empty string to let the caller know op was not successful uwu

############################################################################## Make a bootable disk.img ##############################################################################

def make_disk(fp_OutputDirectory: str) -> bool:

    f_RequiredTools = ['dd', 'mkfs.vfat', 'mmd', 'mcopy']

    for lv_Tool in f_RequiredTools:
        if not ensure_tool_installed(lv_Tool):
            log_error("Missing required tools for disk image creation! Please install the tools needed and try again UwU")
            return False

    f_DiskPath = os.path.join(fp_OutputDirectory, "disk.img")

    f_BootloaderPath = os.path.join(os.getcwd(), 'build', 'violet_bootloader', 'BOOTX64.efi')
    f_KernelPath     = os.path.join(os.getcwd(), 'build', 'violet_kernel', 'violet_kernel.elf')

    log_info(f"Generating virtual hard drive at {f_DiskPath}...")

    try:
        # Create a blank 48MB file full of zeros
        subprocess.run(["dd", "if=/dev/zero", f"of={f_DiskPath}", "bs=1M", "count=48"], check=True) 

        # Format it as a FAT32 partition
        subprocess.run(["mkfs.vfat", "-F", "32", f_DiskPath], check=True) 

        # Create the standard UEFI directory structure layout internally
        subprocess.run(["mmd", "-i", f_DiskPath, "::/EFI"], check=True)
        subprocess.run(["mmd", "-i", f_DiskPath, "::/EFI/BOOT"], check=True)

        # Copy bootloader into the hardcoded UEFI path
        if not os.path.isfile(f_BootloaderPath):
            log_error("No valid bootloader found, unable to make bootable disk.img ;w;")
            return False
        
        subprocess.run(["mcopy", "-i", f_DiskPath, f_BootloaderPath, "::/EFI/BOOT/BOOTX64.EFI"], check=True) 

        # Copy kernel elf to the root directory where the bootloader expects it
        if os.path.isfile(f_KernelPath):
            subprocess.run(["mcopy", "-i", f_DiskPath, f_KernelPath, "::/violet_kernel.elf"], check=True)
        else:
            log_warning("No kernel ELF found, skipping kernel copy to disk; bootloader only")        
        
        log_success(f"disk.img created at {fp_OutputDirectory} nya~")

        return True

    except subprocess.CalledProcessError as f_Exception:
        log_error(f"Failed to build disk image: {f_Exception}")
        return False
    except PermissionError:
        log_error("Permission denied! Please run this script with sudo.")
    except Exception as f_GeneralException:
        log_error(f"Unhandled Python exception: {f_GeneralException}") 

############################################################################## create a qemu bat file to run on windows uwu ##############################################################################

def create_qemu_bat(fp_OutputPath: str, fp_WaitForGdb: bool = False) -> bool:

    if fp_WaitForGdb:
        f_GdbFlag  = " -S"  
    else: 
        f_GdbFlag = ""

    f_BatLines = [
        "@echo off",
        f"cd /d \"{fp_OutputPath}\"",
        "qemu-system-x86_64 ^",
        "    -machine q35 ^",
        "    -m 256M ^",
        "    -drive if=pflash,format=raw,readonly=on,file=OVMF_CODE_4M.fd ^",
        "    -drive if=pflash,format=raw,file=OVMF_VARS_4M.fd ^",
        "    -drive format=raw,file=disk.img ^",
        "    -serial stdio ^",
        "    -display gtk,zoom-to-fit=on ^",
        "    -no-reboot ^",
        "    -d int ^",
        "    -D qemu_crash.log ^",
        "    -M smm=off ",
        "pause",
    ]

    f_BatPath = os.path.join(fp_OutputPath, "launch.bat")

    with open(f_BatPath, "w", newline="\r\n") as f_File:
        f_File.write("\n".join(f_BatLines))

    log_success(f"Ready! double-click launch.bat in {fp_OutputPath} to run 💜")

    if fp_WaitForGdb:
        log_info("QEMU will freeze at boot waiting for GDB on localhost:1234")
    
    return True

############################################################################## prepare the boot media for qemu ##############################################################################

def prepare_qemu_boot_directory() -> bool:

    f_BaseDirectory = os.getcwd()
    f_QemuBootDirectory = os.path.join(f_BaseDirectory, "qemu_boot")

    f_OvmfCodePath = os.path.join(f_BaseDirectory, 'third_party', "ovmf", "OVMF_CODE_4M.fd")
    f_OvmfVarsPath = os.path.join(f_BaseDirectory, 'third_party', 'ovmf', 'OVMF_VARS_4M.fd')

    if os.path.isdir(f_QemuBootDirectory):
        shutil.rmtree(f_QemuBootDirectory)

    os.makedirs("qemu_boot")

    make_disk(f_QemuBootDirectory)

    if not os.path.isfile(os.path.join(f_QemuBootDirectory, 'disk.img')):
        log_error(f"disk.img not found at {f_QemuBootDirectory}, did you run --make_disk?")
        return False
    
    if not os.path.isfile(f_OvmfCodePath):
        log_error(f"OVMF_CODE_4M.fd not found at {f_OvmfCodePath}, did you modify the third_party/ovmf directory?")
        return False
    
    if not os.path.isfile(f_OvmfVarsPath):
        log_error(f"OVMF_VARS_4M.fd not found at {f_OvmfVarsPath}, did you modify the third_party/ovmf directory?")
        return False
    
    shutil.copy2(f_OvmfCodePath, f_QemuBootDirectory)
    shutil.copy2(f_OvmfVarsPath, f_QemuBootDirectory)

    return True

############################################################################## Prepare for Windows UwU from my wsl ##############################################################################

def prepare_qemu_for_windows_from_wsl(fp_WindowsOutputPath: str) -> bool:

    ########################## Verify the output path ##########################

    if not os.path.isdir(fp_WindowsOutputPath):
        log_error(f"Target windows path is not a real directory or not mounted: {fp_WindowsOutputPath}")
        return False

    ########################## Create the actual disk.img and get the OVMF stuff ##########################
    
    if not prepare_qemu_boot_directory():
        log_error("Unable to create a valid qemu boot directory inside prepare_qemu_for_windows_from_wsl ;w;")
        return False
    
    ########################## Verify Directories and contents uwu ##########################

    f_LocalSource = os.path.join(os.getcwd(), 'qemu_boot')
    f_CopyDestination = os.path.join(fp_WindowsOutputPath, 'qemu_boot')

    ########################## copy required stuff for booting qemu ##########################
    
    log_info(f"Copying run artifacts to {f_CopyDestination}...")

    ########################## copy bootable media to ntfs ##########################

    shutil.copytree(f_LocalSource, f_CopyDestination, dirs_exist_ok=True)

    ########################## Create and move bat into windows path uwu ##########################

    create_qemu_bat(f_CopyDestination)

    return True

############################################################################## QEMU Launch ##############################################################################

def run_qemu(fp_BaseDir: str, fp_WaitForGdb: bool) -> bool:

    if not ensure_tool_installed("qemu-system-x86_64"):
        log_error("qemu-system-x86_64 not found in PATH, is QEMU installed?")
        return False

    f_OvmfCodePath = os.path.join(fp_BaseDir, "ovmf", "OVMF_CODE.fd")
    f_OvmfVarsPath = os.path.join(fp_BaseDir, "ovmf", "OVMF_VARS.fd")
    f_DiskPath     = os.path.join(fp_BaseDir, "disk.img")

    for f_Path, f_Name in [
        (f_OvmfCodePath, "OVMF_CODE.fd"),
        (f_OvmfVarsPath, "OVMF_VARS.fd"),
        (f_DiskPath,     "disk.img"),
    ]:
        if not os.path.isfile(f_Path):
            log_error(f"{f_Name} not found at {f_Path}")
            print_tip(f"run --make_disk first to create the disk image, and put OVMF .fd files in ovmf/")
            return False

    f_QemuCmd = [
        "qemu-system-x86_64",
        "-machine",  "q35",
        "-m",        "256M",
        "-drive",    f"if=pflash,format=raw,readonly=on,file={f_OvmfCodePath}",
        "-drive",    f"if=pflash,format=raw,file={f_OvmfVarsPath}",
        "-drive",    f"format=raw,file={f_DiskPath}",
        "-serial",   "stdio",
        "-display",  "gtk,zoom-to-fit=on",
        "-no-reboot",
        "-s",        # GDB server always on :1234
    ]

    if fp_WaitForGdb:
        f_QemuCmd.append("-S")  # freeze at boot until GDB connects
        log_info("QEMU frozen, waiting for GDB on localhost:1234 ~ connect with: gdb kernel.elf -> target remote :1234")

    log_info("launching QEMU ~ nya~")

    try:
        run_command_with_live_output(f_QemuCmd, fp_BaseDir)

    except subprocess.CalledProcessError:
        log_error("QEMU exited with non-zero code")
        return False

    return True

############################################################################## Main CMake Function ##############################################################################

def run_cmake(fp_BuildType: str, fp_Generator: str, fp_TargetArch: str, fp_ExtraBuildArgs: list, fp_ExtraConfigs: list, fp_BuildOutputDirectoryName : str) -> bool:

    f_GeneratorMap = {
        "vs2026": "Visual Studio 18 2026",
        "vs2022": "Visual Studio 17 2022",
        "vs2019": "Visual Studio 16 2019",
        "vs2017": "Visual Studio 15 2017",
        "vs2015": "Visual Studio 14 2015",

        "xcode": "Xcode",

        "ninja": "Ninja", #everything under here is untested so uh goodluck w that uwu
        "ninja-mc": "Ninja Multi-Config",

        "unix": "Unix Makefiles",
        "unix-cb": "CodeBlocks - Unix Makefiles",
        "unix-eclipse": "Eclipse CDT4 - Unix Makefiles",

        "mingw": "MinGW Makefiles",
        "msys": "MSYS Makefiles",
        "nmake": "NMake Makefiles",
        "nmake-jom": "NMake Makefiles JOM"
    }

    ############# Ensure Valid Generator was Selected #############

    if fp_Generator not in f_GeneratorMap:
        log_error("Invalid Generator Selected, use -h to see what generators are available owo")
        return False
    
    ############# Determine if Generator is Single Config #############
    
    f_IsMultiConfig = fp_Generator in ["vs2026", "vs2022", "vs2019", "vs2017", "vs2015", "xcode", "ninja-mc"]

    f_CMakeConfigCommand = ['cmake', '-S', '.', '-B', fp_BuildOutputDirectoryName, '-G', f_GeneratorMap[fp_Generator]]
    
    if not f_IsMultiConfig:

        if fp_BuildType == "Release and Debug": #Don't allow "both" configs for single config generators uwu
            
            log_error("Invalid build type selected: YOU CANNOT USE BOTH WHEN GENERATING FOR A SINGLE CONFIG GENERATOR")
            return False
        
        else:
            f_CMakeConfigCommand += ['-DCMAKE_BUILD_TYPE=' + fp_BuildType.capitalize()]

    ############# Set Target Platform #############

    if fp_TargetArch == "":
        log_error("No target ISA was selected, please specify which platform VioletOS is being built for uwu")
        return False
    
    f_CMakeConfigCommand += [
        "-DCMAKE_TOOLCHAIN_FILE=violet.toolchain.cmake",
        f"-DVIOLET_TARGET_ARCH={fp_TargetArch}"
    ]

    ############# Generate CMake Project #############

    try:
        log_info(f"Running CMake project generation for {f_GeneratorMap[fp_Generator]}...")

        run_command_with_live_output(f_CMakeConfigCommand + fp_ExtraConfigs)

    except subprocess.CalledProcessError as err:
        log_error("CMake project generation failed!")
        return False

    log_success("CMake project generation completed!")

    ############# Run CMake Build Process for Single Config #############

    if not f_IsMultiConfig:
        try:
            log_info(f"Running CMake single config build for {fp_BuildType}...")

            run_command_with_live_output(['cmake', '--build', fp_BuildOutputDirectoryName] + fp_ExtraBuildArgs)

        except subprocess.CalledProcessError as err:
            log_error(f"CMake single config {fp_BuildType} build process failed!")
            return False

        log_success(f"{fp_BuildType} build completed!")

        return True #return immediately since we don't need to go through the --config commands for single config generators

    ############# Run Debug Build #############

    if( fp_BuildType == "Debug" or fp_BuildType == "Release and Debug" ):
        try:
            log_info("Running CMake build for Debug...")

            run_command_with_live_output(['cmake', '--build', fp_BuildOutputDirectoryName, '--config', 'Debug'] + fp_ExtraBuildArgs)

        except subprocess.CalledProcessError as err:
            log_error("CMake debug build process failed!")
            return False

        log_success("Debug build completed!")

    ############# Run Release Build #############

    if( fp_BuildType == "Release" or fp_BuildType == "Release and Debug" ):
        try:
            log_info("Running CMake build for Release...")

            run_command_with_live_output(['cmake', '--build', fp_BuildOutputDirectoryName, '--config', 'Release'] + fp_ExtraBuildArgs)

        except subprocess.CalledProcessError as err:
            log_error("CMake release build process failed!")
            return False

        log_success("Release build completed!")

    ############# Success! #############

    log_info("\nYour CMake project should be good to go!")

    return True

############################################################################## Build Status Enum >///< ##############################################################################

class ToolStatus(Enum):
    OK =                                        0
    BUILD_SUCCESS  =                            1
    BUILD_FAILED =                              2
    CLEAN_OR_NUKE_REQUESTED =                   3
    CLEAN_FAILED =                              4
    NUKE_FAILED =                               5
    ANDROID_INSTALL_FAILED =                    6
    ANDROID_PACKAGE_FAILED =                    7
    ANDROID_LAUNCH_FAILED =                     8
    MISFORMED_BUILD_ARGUMENTS_PASSED =          9
    QEMU_LAUNCH_FAILED =                        10
    QEMU_NOT_FOUND        =                     11
    DISK_BUILD_FAILED     =                     12
    WINDOWS_QEMU_FROM_WSL_FAILED =              13

############################################################################## Main Function ##############################################################################

def main() -> ToolStatus:

    ############# Check for Required Build Tools in PATH #############
    
    if not ensure_tool_installed("cmake"): 
        return ToolStatus.BUILD_FAILED   

    ############# Setup Parser #############

    usage_message =                                                                                     \
        create_coloured_text("init.py ", 'bright magenta') +                                              \
        create_coloured_text("--[release, debug or both] ", "bright blue") +                              \
        create_coloured_text("-G [desired_generator] ", "bright green") +                                 \
        create_coloured_text("...extra flags", "bright cyan")

    parser = argparse.ArgumentParser(
        description=create_coloured_text('Used for Building VioletOS(💜) from Source', 'bright green'), 
        usage=usage_message, 
        add_help=True,
        formatter_class=argparse.RawTextHelpFormatter
    )

    ############# Set Parser Arguments #############

    parser.add_argument(
        '--release', 
        action='store_true', 
        help=create_coloured_text('Used for a release build', 'bright magenta')
    )

    parser.add_argument(
        '--debug', 
        action='store_true', 
        help=create_coloured_text('Used for a debug build', 'bright magenta')
    )

    parser.add_argument(
        '--both', 
        action='store_true', 
        help=create_coloured_text('Used to build both a debug and release build', 'bright magenta')
    )

    parser.add_argument(
        '--clean', 
        action='store_true', 
        help=create_coloured_text('Used to clean VioletOS build artifacts from a previous run', 'bright magenta')
    )

    parser.add_argument(
        '--nuke', 
        action='store_true', 
        help=create_coloured_text('Used to completely wipe the build directory including all compiled dependencies', 'bright magenta')
    )

    parser.add_argument(
        '-G', 
        nargs=1,
        metavar="[generator]",
        help=
            create_coloured_text('Used to set the project file generator, options are as follows:', 'bright magenta') + "\n" +                                        
                "\t" + create_coloured_text('-G ', 'blue') +  create_coloured_text("vs2015 --> vs2026 ", 'bright green') + create_coloured_text('Generates solution for Visual Studio 2015 - 2026', 'cyan') + "\n" + 
                
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("xcode ", 'bright green') + create_coloured_text('Generates project files for Xcode', 'cyan') + "\n" +                            
                
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("ninja ", 'bright green') + create_coloured_text('Generates project files using Ninja', 'cyan') + "\n" +                          
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("ninja-mc ", 'bright green') + create_coloured_text('For Ninja Multi-Config', 'cyan') + "\n" +                                    
                
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("unix ", 'bright green') + create_coloured_text('For Unix Makefiles', 'cyan') + "\n" +                                            
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("unix-eclipse ", 'bright green') + create_coloured_text('Generate Unix Makefiles for Eclipse CDT', 'cyan') + "\n" +               
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("unix-cb ", 'bright green') + create_coloured_text('Generates Unix Makefiles for CodeBlocks', 'cyan') + "\n" +                    

                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("mingw ", 'bright green') + create_coloured_text('Generates MinGW Makefiles', 'cyan') + "\n" +                                    
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("msys ", 'bright green') + create_coloured_text('Generates MSYS Makefiles', 'cyan') + "\n" +                                      
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("nmake ", 'bright green') + create_coloured_text('Generates NMake Makefiles', 'cyan') + "\n" +                                    
                "\t" + create_coloured_text('-G ', 'blue') + create_coloured_text("nmake-jom ", 'bright green') + create_coloured_text('Generates JOM Makefiles', 'cyan')
    )   
    
    parser.add_argument(
        '-T',
        nargs=1,
        metavar="[target]",
        help=
            create_coloured_text("Valid toolchain keys are: ", 'bright magenta') + "\n" + 
                "\t" + create_coloured_text("-T", "blue") + create_coloured_text(" x86_64",'bright green') + "\n" +
                "\t" + create_coloured_text("-T", "blue") + create_coloured_text(" x86",'bright green') + "\n" +
                "\t" + create_coloured_text("-T", "blue") + create_coloured_text(" arm64",'bright green') + "\n" +
                "\t" + create_coloured_text("-T", "blue") + create_coloured_text(" arm32",'bright green') + "\n" +
                "\t" + create_coloured_text("-T", "blue") + create_coloured_text(" risc-v",'bright green') + "\n" +
                "\t" + create_coloured_text("-T", "blue") + create_coloured_text(" mips",'bright green') + "\n" 
    )

    parser.add_argument(
        '-J',
        nargs=1,
        metavar="[max_jobs]",
        help=create_coloured_text("Set max number of jobs the compiler can do at once owo",'bright magenta')
    )

    parser.add_argument(
        '--dump_errors',
        action='store_true',
        help=create_coloured_text('Dump all build errors to build_errors.md', 'bright magenta')
    )

    parser.add_argument(
        '--dump_warnings',
        action='store_true',
        help=create_coloured_text('Dump all build warnings to build_warnings.md', 'bright magenta')
    )

    parser.add_argument(
        '--dump_output',
        action='store_true',
        help=create_coloured_text('Dump all build warnings + errors', 'bright magenta')
    )

    parser.add_argument(
        '--export_commands',
        action='store_true',
        help=create_coloured_text('Export compile commands', 'bright magenta')
    )

    parser.add_argument(
        '--verbose',
        action='store_true',
        help=create_coloured_text('Adds verbose and will add additional flags depending on generator', 'bright magenta')
    )

    parser.add_argument(
        "--bootloader",
        action="store_true",
        help=create_coloured_text('Build the UEFI bootloader target', 'bright magenta')
    )

    parser.add_argument(
        "--kernel",
        action="store_true",
        help=create_coloured_text('Build the kernel ELF target', 'bright magenta')
    )

    parser.add_argument(
        "--tests",
        action="store_true",
        help=create_coloured_text('Build hosted unit tests >///<', 'bright magenta')
    )

    parser.add_argument(
        "--gdb",
        action="store_true",
        help=create_coloured_text('Pass -s -S to QEMU (freeze at boot, wait for GDB on :1234)', 'bright magenta')
    )

    parser.add_argument(
        '--run',
        action='store_true',
        help=create_coloured_text('Launch QEMU after a successful build', 'bright magenta')
    )

    parser.add_argument(
        '--make_disk',
        nargs = 1,
        type=str,
        metavar='[output_path]',
        help=create_coloured_text('Make a bootable disk image after a successful build uwu', 'bright magenta')
    )

    parser.add_argument(
        '--prepare_windows_run',
        nargs = 1,
        type=str,
        metavar='[output_path]',
        help=create_coloured_text('Prepare the build artifacts so that we can run it on windows from wsl2 >w<', 'bright magenta')
    )

    args = parser.parse_args()

    ############# Detect Platform #############

    f_CurrentPlatform = platform.system()

    ############# Get Current Working Directory #############

    f_BaseDir = os.getcwd()

    ############# Extra args and cmake configs owo #############

    f_ExtraBuildArgs = []
    f_ExtraGenerationConfigs = []

    ############# KABOOOOOOOOOOOOOOOOOOOOOOOOOOM #############

    if args.nuke and args.clean:
        log_error("tried to pass --nuke and --clean, you can only pick one; with great power comes great responsibility einstein said that, are you saying you're smarter than einstein?")
        return ToolStatus.MISFORMED_BUILD_ARGUMENTS_PASSED

    if args.nuke:
        
        print(create_coloured_text("[THE HORROR]: 3..2..1.. clearance granted, firing 😔", "bright magenta"))

        try:
            shutil.rmtree('build')
        except FileNotFoundError:
            log_info("build directory doesn't exist, just ignoring --nuke call >w<")
        except PermissionError as f_Exception:
            log_error(f"permission denied for nuking ;w; what am i supposed to do now? what: {f_Exception}")
            return ToolStatus.NUKE_FAILED
        except Exception as f_Exception:
            log_error(f"unable to nuke build directory idk y, what: {f_Exception}")
            return ToolStatus.NUKE_FAILED

    elif args.clean:

        log_info("Scraping only violet components... leaving dependencies cozy ~nya~ 💜✨")

        f_TargetBaseNames = ["violet_kernel", "violet_bootloader", "violet_tests"]

        # Wipe out standard CMake intermediate directory layout states

        for f_Target in f_TargetBaseNames:
            shutil.rmtree(f"build/CMakeFiles/{f_Target}.dir", ignore_errors=True) 

        # Recursive Binary Extermination Pass

        for f_Target in f_TargetBaseNames:
            
            f_RecursivePattern = f"build/**/{f_Target}*"  # target both exact name matches and variants with extensions (.exe, .lib, .a, .pdb)
            
            for f_FilePath in glob.glob(f_RecursivePattern, recursive=True):

                if "third_party" in f_FilePath or "src" in f_FilePath:  # Avoid accidentally nuking live source root folders if something goes wild
                    continue
                    
                if os.path.isdir(f_FilePath): # Catch and delete directory matching artifacts (like target project folders in VS/Xcode)
                    
                    if f_FilePath.endswith(".dir") or f_FilePath.endswith(".framework") or f_FilePath.endswith(".bundle"):
                        shutil.rmtree(f_FilePath, ignore_errors=True)
                else: # Catch and delete individual binary files (.exe, .a, .lib, .so, .dylib, .pdb, .ninja)
                    try:
                        os.remove(f_FilePath)
                        log_info(f"successfully removed: '{f_FilePath}'")
                    except OSError:
                        log_error(f"failed to remove: '{f_FilePath}' during --clean")
                        return ToolStatus.CLEAN_FAILED

        log_info("Clean completed! Peach components completely purged, dependencies preserved.")

    ############# Validate Build Config #############

    f_BuildType = "nothing"

    if(args.debug):
        f_BuildType = "Debug"

    elif(args.release):
        f_BuildType = "Release"

    elif(args.both):
        f_BuildType = "Release and Debug"
    
    elif args.clean or args.nuke:
        return ToolStatus.CLEAN_OR_NUKE_REQUESTED #just gonna assume if no build config was passed they just wanted a clean uwu

    else:
        log_error("No valid build type input detected, use -h or --help if you're unfamiliar")
        return ToolStatus.MISFORMED_BUILD_ARGUMENTS_PASSED

    ############# Check for Generator #############
        
    if(not args.G):
        log_error("please specify cmake generator using -G [desired_generator] >w<")
        return ToolStatus.BUILD_FAILED

    f_DesiredGenerator = args.G[0].lower() #convert to all lower case for easier handling

    ############# Export compile commands? #############

    if args.verbose:
        if f_DesiredGenerator == "vs2022":
            f_ExtraBuildArgs += ['--verbose', '--', '-verbosity:diagnostic']

    ############# Thread Limiter #############

    if args.J:
        f_MaxNumberOfJobs = args.J[0]

        f_ExtraBuildArgs.extend(["--parallel", f_MaxNumberOfJobs])

    ############# Export compile commands? #############

    if args.export_commands:
        f_ExtraGenerationConfigs.append('-DCMAKE_EXPORT_COMPILE_COMMANDS=ON') 

    ############# Force Clang #############

    if not ensure_tool_installed("clang") and not ensure_tool_installed("clang++"):          
        return ToolStatus.BUILD_FAILED

    f_ExtraGenerationConfigs.extend(["-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++"])

    ############# Target Platform Config #############

    f_ToolchainKey = ""

    if args.T:

        f_ToolchainKey = args.T[0].lower()

        f_ValidToolchainKeys = [
            "x86_64", "x86", "arm64", "arm32", "riscv64", "riscv32", "mips"
        ]

        if f_ToolchainKey not in f_ValidToolchainKeys:
            log_error("invalid toolchain key was detected, please use -h to see the list of valid toolchain keys")
            return ToolStatus.BUILD_FAILED
        
    else:
        f_MachineArch = platform.machine().lower()

        if "x86_64" in f_MachineArch or "amd64" in f_MachineArch:
            f_ToolchainKey = "x86_64"
        elif "x86" in f_MachineArch or "i386" in f_MachineArch or "i686" in f_MachineArch:
            f_ToolchainKey = "x86"
        elif "arm64" in f_MachineArch or "aarch64" in f_MachineArch:
            f_ToolchainKey = "arm64"
        elif "arm" in f_MachineArch:
            f_ToolchainKey = "arm32"
        elif "riscv64" in f_MachineArch:
            f_ToolchainKey = "riscv64"
        elif "riscv64" in f_MachineArch:
            f_ToolchainKey = "riscv64"
        elif "mips64" in f_MachineArch:
            f_ToolchainKey = "mips64"
        elif "mips" in f_MachineArch:
            f_ToolchainKey = "mips32"
        else:
            log_error(f"Unknown machine arch: {f_MachineArch}")
            return ToolStatus.BUILD_FAILED

        log_info(f"Auto-detected platform: {f_ToolchainKey} ~ nya~")

    ############# Run Build Fingers Crossed >w< #############

    if args.bootloader:
        log_info("Configuring bootloader (clang → PE32+)...")
        f_ExtraGenerationConfigs.extend(['-DVIOLET_BUILD_BOOTLOADER=ON'])

    if args.kernel:
        log_info("Configuring kernel (clang → ELF)...")
        f_ExtraGenerationConfigs.extend(['-DVIOLET_BUILD_KERNEL=ON'])

    if args.tests:
        log_info("Configuring tests...")
        f_ExtraGenerationConfigs.append('-DVIOLET_BUILD_TESTS=ON')

    # need at least one target selected
    if not args.bootloader and not args.kernel and not args.tests and not args.clean and not args.nuke:
        log_error("didn't specify what to build, you need to explicitly pass --bootloader, --kernel, or --tests ^w^")
        return ToolStatus.MISFORMED_BUILD_ARGUMENTS_PASSED
    
    build_result = run_cmake(f_BuildType, f_DesiredGenerator, f_ToolchainKey, f_ExtraBuildArgs, f_ExtraGenerationConfigs, 'build')

    ############# Provide Printout #############

    if args.dump_output:
        write_build_summary_markdown(".", True, True);
    elif args.dump_warnings:
        write_build_summary_markdown(".", False, True);
    elif args.dump_errors:
        write_build_summary_markdown(".", True, False);
    
    ############# return false on failed build ;w; #############

    if not build_result:
        return ToolStatus.BUILD_FAILED
    
    ############# Check for QEMU Stuff #############

    if args.make_disk and not make_disk(args.make_disk[0]):
        return ToolStatus.DISK_BUILD_FAILED
    
    elif args.prepare_windows_run and not prepare_qemu_for_windows_from_wsl(args.prepare_windows_run[0]):
        return ToolStatus.WINDOWS_QEMU_FROM_WSL_FAILED
    
    ############# Report Build Stats #############

    log_info("Final Build Summary: \n")
    print(create_coloured_text(f"Generator: {f_DesiredGenerator}", "bright magenta"))
    print(create_coloured_text(f"Build Type: {f_BuildType}", "bright magenta"))
    print(create_coloured_text(f"Platform: {f_ToolchainKey}\n", "bright magenta"))

    ############# QEMU Launch #############

    # if args.run or args.gdb:
    #     if not run_qemu(f_BaseDir, fp_WaitForGdb=args.gdb):
    #         return ToolStatus.QEMU_LAUNCH_FAILED

    return ToolStatus.BUILD_SUCCESS

############################################################################## Main Caller ##############################################################################

if __name__ == "__main__":

    if platform.system() == "Windows": #enable ANSI colour codes for Windows Console
        os.system('color') 

    if main() == ToolStatus.BUILD_FAILED:
        log_error("execution of full build process was unsuccessful\n")
    else:
        print(create_coloured_text("done!\n", "magenta"))


#Rawr OwO

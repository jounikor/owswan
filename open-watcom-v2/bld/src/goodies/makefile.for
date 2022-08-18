#                             EXAMPLE MAKEFILE
#                             ----------------
#
# DESCRIPTION
# -----------
# Generic make file for a Fortran program.

# EXAMPLE
# -------
# This makefile assumes you are trying to compile a program for 32-bit OS/2
# called "example.for" and subroutines contained in two separate files
# called "sub1.for" and "sub2.for".

# INSTRUCTIONS
# ------------
# Set the following macros to appropriate values:
#     Compiler - the name of the compiler. Either 'wfc386' for
#             32-bit programs or 'wfc' for 16-bit ones.
#     Compiler_options- what ever options you wish for your compiles.
#     Linker_options - what ever options you wish for your links.
#     System      - The name of a system from the
#             "\WATCOM\BINW\WLSYSTEM.LNK" file. This identifies
#             the target operating system the program is to
#             run on.
#     Exe_file - the name of the resulting executable file.
#     Object_files   - the list of object files to be linked together.
#
# If this makefile is called "makefile" then just run WMAKE.
# If it has another name such as "example.mak" then you have to
# give the file name at the command line:
#   WMAKE -f example.mak

# REMARKS
# -------
# Default Watcom compiler options are set using an environment variable
# which can be placed in "config.sys". For instance, one may have:
#
# SET WFC386=-3 -FPI87 -OL -OM -OT -D1 -NOTE
#
# Then, the Compiler_Options variable below will modify the default.
#
# DEBUGGER SETUP
# --------------
# The following environment variable makes Open Watcom Debugger a bit nicer
# in OS/2 (you can set this environment variable in config.sys)
#
#  SET WD=-Lines#33
#
# The -Lines#33 parameter makes WD use a 33 line display which
# in an OS/2 Window on VGA makes more use of the screen area.

# BEGINNING OF MAKE FILE....

##########################
## User settable macros ##
##########################

Compiler = wfc386
#Compiler = wfc

# Compiler_Options =
# Following is best setup for WVIDEO debugger.
Compiler_Options = -D2 -warn

# Linker_options   =
Linker_options   = debug all

System       = os2v2

Exe_file     = example.exe

Object_files = example.obj &
               sub1.obj    &
               sub2.obj

####################
## Makefile rules ##
####################

$(Exe_file): $(Object_files)
        *wlink system $(System) $(Linker_Options) name $(Exe_file) &
            file {$(Object_files)}

.for.obj:
        *$(Compiler) $(Compiler_Options) $<

## OWSWAN - WonderSwan Color tool chain (BETA 3) based on Open Watcom v2 Fork

This fork and branch contain tools and modification to compile C-programs to [WonderSwan Color](https://en.wikipedia.org/wiki/WonderSwan) handheld console target. 

The development has been done on OSX 10.13.6 as the main host OS. No testing has been done on any other host OS but Linux should be just fine. The build system has been 'cut' to compile in my system and therefore the build does not contain all material as the full Watcom build would do.

Bear in mind that:
* This is a hack.. a fork of Dos target.
* Testing has been limited.. and what has been done is done against Mednafen.

### What is provided ###
* Two new targets added into the `wlink.lnk`: for small ROMs `wsc` and `wscl` for larger ROMs:
  - `wcl -bcl=wsc` to compile and link for the WonderSwan Color. Program code starts at `$2000:0000`.
  - `wcc -bt=wsc` to compile for the WonderSwan Color. 
  - `wcl -bcl=wscl` to compile and link for the WonderSwan Color. Program code starts at `$4000:0000`.
  - `wcc -bt=wscl` to compile for the WonderSwan Color.

* A `wscromtool` to convert the output flat binary to WonderSwan Color ROM. There are few quirks to it.. which will be documented later..
* A simple `wsccrt0.obj` to do the required initialization and call your C main().
* A simple `libwscl.lib` stub for all kinds of helper fuctions.

### Peculiarities ###
* Only *large memory model* is supported i.e., compile with `-ml`.
* Stack checking must be disabled. Either compile with `-s` or use `#pragma off (check_stack)`. The startup code does not provide required glue for that to work.
* No floating point libraries. Fixed point \o/
* Non-ROMmable initialized static variable and statically allocated variable storage is very limited. Basically the RAM between `$0000:0100` -> `$0000:0dff` is shared between data, bss and stack segments. The stack grows down from `$0000:0e00` and no checks are performed whether bss or data gets overwritten by extensive stack usage.
  - This is the assumed lower RAM layout. In a case you want/need to have a different setup you need to modify the `wlink.lnk` file.
  - The `wlink.lnk` sets the "stack size" to 512 bytes to avoid linking time warnings.. however, this setting is artificial.
* In order to ensure that your static or const variables really end up to ROM and not into the small available RAM the following measures are good to know:
  - Compile with option `-zc` in order to place constant strings into the text segment.
  - Use `__far` to place data segments into FARDATA, which in this case end up into the ROM. Still be careful with strings defined with a pointer and not as an array as they still easily end up to RAM but the pointer to the string into FARDATA.
  - Use `const` for the data part of the variable definitions to ensure that not just the pointer is const.
  - Another trick is to compile with `-nd=somename` to define a special prefix for the "CONST", "CONST2", "\_DATA", and "\_BSS" segment names. However, this will move also your statically allocated variables into ROM area thus making them immutable..
  - It is a good practise to check how your map file looks like to spot unwanted surprises.
* The map file is always produced by default. The map file is required by the `wscromtool`.
* Do not link against generic Watcom provided libraries including the C Library.
  - It is still possibe/safe to include certain OWC provide headers such as `stdint.h`. However, you are on your own here.
* No attempt has been made to ensure that C++ works.
* The `wscromtool` strips paths away when building additional raw data ROM banks. This implies a file with the same name but different path are treated as the same file and will cause multiple definitions of macros in the output header file.
* The header file `libwsc.h` defines:
  - `extern unsigned char __wsc_data_bank`, which is initialized by the `wsccrt0`. This variable contains the first 'additional' raw data ROM bank added by the `wscromtool`, if any.
  - `extern unsigned char __wsc_first_bank`, which is initialized by the `wsccrt0`. This variable contains the first ROM bank where the code execution starts.
  - Several helper (inline ASM) functions/macros for bank switching and handling IO ports.
* If your compile/linking target is `wsc`:
  - The compiled code execution starts at `$2000:0000` thus leaving 917504 bytes for compiled code and data.
  - Raw data banks are placed at the beginning of the ROM (preceding the compiled code).
  - Minimum 1Mbit and up to 128Mbit ROMs are supported.
* If your compile/linking target is `wscl`:
  - The compiled code execution starts at `$4000:0000` thus leaving 786432 bytes for compiled code and data. However, this arrangement leaves both REG_BANK_ROM0 & REG_BANK_ROM1 fully unoccupied and available for the programmer to exploit.
  - Raw data banks are placed at the beginning of the ROM (preceding the compiled code).
  - *Minimum 8Mbit* and up to 128Mbit ROMs are supported. The `wscromtool` will make sure ROM between `$4000:0000` and `$ffff:ffff` is always fully mappable.
* By default, if `wcl` is used to compile + link your project, the output binary file has the same name as your first C-file with `.bin` extension. You can always override this using the `-fe` commandine option.
* By default, if `wcl` is used to compile + link your project, the output map file has the same name as your first C-file with `.map` extension. You can always override this using the `-fm` commandine option.


### Assumptions and memory map layout when main(void) is called ###
* REG_BANK_ROM2 is $ff.
* REG_BANK_ROM0 is set to value in `__wsc_first_bank`.
* REG_BANK_ROM1 is ??.
* REG_BANK_SRAM is ??.
* Code execution (wsccrt0) starts either from `$2000:0000` (the compile/linkin target was set to `wsc`) or from `$4000:0000` (the compile/linkin target was set to `wscl`).
  - No parameters are passed to main().
* Stack top is located at `$0000:0e00`.
* IRQ vectors up to `$0000:0040` are in use.
* The WS(C) IRQ base is set to `$0000:0020`.
* RAM from `$0000:0100` to `$0000:0dff` is reserved for C-compiler to use.
* All WS(C) IRQs are turned off and acknowledged. CPU IRQs are enabled. 

### Additional information included into the ROM ###
* The `wscromtool` uses bytes from `$fffe:0006 to $fffe:000f` in the last ROM bank in addition to the standard ROM metadata and reset vector.
* Bytes from `$ffff:0006` to `$ffff:000f` contain the standard ROM metadata. 
* `$ffff:0005` is 0x00.
* Bytes from `$ffff:0000` to `$ffff:0004` contain a JMP to bootstrap code. 
* `$fffe:0007` contains the bank where the ROM starts.
* `$fffe:0008` contains the bank where the 'filesystem' data starts.
* The `wscromtool` uses bytes below `$fffe:0006` in the last ROM bank to store the non-const initialized data.

### wscromtool ###
The `wscromtool` is used to:
* Build the final ROM including metadata and boostrap code.
* Compile the (optional) binary file that can be used to add raw data banks into the ROM. During the creation of the data file an associated C-header file is generated, which lists the BANK, SIZE and OFFSET of the raw data files.

To see the top level commands and options just enter the tool name:
```
jounis-MBP:open-watcom-v2 jouni$ wscromtool 
WonderSwan ROM tool v0.3 (c) 2022 Jouni 'Mr.Spiv' Korhonen

Usage: wscromtool command [options] [files..]
Commands:
  rom  Prepare final ROM.
  fs   Create ROM bank(s) file for 'ROM file system'.
Options:
  --verbose,-v Print various information while processing.

For command specific options type:
  wscromtool command
```

To see the 'rom' command options enter the tool name and the command:
```
jounis-MBP:open-watcom-v2 jouni$ wscromtool rom
Usage: rom [options] input_rom map_file [output_rom]:

Options:
  --data,-D        Data segments to be added into the ROM.
  --pub-id,-p      Publisher ID.  Defaults to 0.
  --game-id,-g     Game ID. Defaults to 0.
  --game-rev,-r    Game revision.  Defaults to 0.
  --mono,-m        Monochrome WonderSwan.
  --sram-eeprom,-s SRAM/EEPROM size (1k..4M) bits. Default to none.
  --1-cycle,-1     ROM access speed (1 cycle instead of 3 cycles).
  --8bit-bus,-8    ROM bus width (8-bit instead of 16-bit).
  --vertical,-V    Vertical orientation instead of horizontal.
  --rtc,-R         Realtime clock present.
  input_rom        The output ROM from the compiler/linker.
  output_rom       The final ROM after patching. If not give no final
                   ROM is saved but all checks are performed.
  map_file         The map file produced by the linker.
```

To see the 'fs' command options enter the tool name and the command:
```
jounis-MBP:open-watcom-v2 jouni$ wscromtool fs
Usage: fs [options] output_bin outout_hdr file [files..]:

Options:
  --keep-order,-k  Do not rearrange files.
  output_bin       Output binary file containing data segments.
  output_hdr       Generated C-header file for the output binary.
  file,files..     Files to add into the output binary. Maximum
                   allowed single file size is 65536 bytes.
```

### WSC directory layout and files ###
```
bld/libwsc/startup/
  - Location for startup code (wsccrt0) and 'source' header files. Cross compiled to WSC.

bld/libwsc/lib/
  - libwscl.lib source and header files

bld/libwsc/h/
  - libwsc.h and other source header files

bld/wscromtool/
  - Location for wscromtool sources.

rel/libwsc/
  - Target location for wsccrt0.obj.
  - Target location for libwscl.lib.

rel/libwsc/h/
  - Target location for libwsc.h header.
```

### libwsc considerations ###
* You do not need to link against the library. Just including the `libwsc.h` is enough for the essential macros and definitions.
* `libwsc.h` is the main header for:
  - `libwscl.lib` functions.
  - Inlined asm macros and/or functions simple low level procedures.
  - Defines for IO ports and theis bits & masks.
* Avoid static variables and statically allocated variables because they will consume the scarce low RAM.

### Compiling for Wonderswan ###
* Set `WATCOM` environment variable to point where your installation ´rel´ is located.
* Add `$WATCOM/osx64` (or whatever host system you use) into your PATH.
* Remember to add `$WATCOM/libwsc/h` into you include search path.
* Remember to add `$WATCOM/h` into you include search path if you use any OW provided headers (remember the note earlier..).
* Remember the 'essential' compiler switches. For example I used the following line:
  - `wcl -bcl=wsc -1 -zc -ml -s -i=$WATCOM/h -i=$WATCOM/libwsc/h hello.c libwscl.lib`

### Building OpenWatcom ###
See the normal OpenWatcom build instructions. The Wonderswan modifications should build automatically. Note that currently testing has been only made against old OSX (i.e. host OS `osxx64`).

### Useful links ###
* [WSMan by trap15](http://daifukkat.su/docs/wsman/)
* [80186 programming for the Wonderswan and Wonderswan Color](https://www.chibialiens.com/8086/wonderswan.php)
* [wstech v24](https://gitlab.com/TASVideos/BizHawk/-/blob/master/wonderswan/wstech24.txt)


### TODO ###
- [x] Non-const data initialization support.
- [x] Consider rearraning segments and everything so that code starts somewhere after 0x4000:0000 instead of 0x2000:0000.
- [x] wscromtool 'command' for building the final ROM.
- [x] wscromtool 'command' for building the 'filesystem' for additional data segments.
- [x] Refactor (i.e. implement proper) wsccrt0.
- [x] Larger than 1MB (8Mbit) support. This would we a very simple "filesystem" to swap in required ROM bank and provide offset to required const data.
- [ ] Proper documentation.
- [x] Sample code.
- [x] WonderSwan specific library for helper functions.



## Open Watcom v2 Fork
|Project Build Status||Download|
|---|---|---|
|![Build Status](https://dev.azure.com/open-watcom/open-watcom-v2/_apis/build/status/open-watcom-v2-CI?branchName=master&stageName=Last%20CI%20build)|CI Build|[Github Release](https://github.com/open-watcom/open-watcom-v2/releases/tag/Last-CI-build) , [Azure Pipelines Build](https://dev.azure.com/open-watcom/open-watcom-v2/_build/latest?definitionId=11&branchName=master)|
|![Build Status](https://dev.azure.com/open-watcom/open-watcom-v2/_apis/build/status/open-watcom-v2-Release?branchName=master&stageName=GitHub%20Release)|Current Release Build|[Github Release](https://github.com/open-watcom/open-watcom-v2/releases/tag/Current-build) , [Azure Pipelines Build](https://dev.azure.com/open-watcom/open-watcom-v2/_build/latest?definitionId=14&branchName=master)|
|![Build Status](https://dev.azure.com/open-watcom/open-watcom-v2/_apis/build/status/open-watcom-v2-Coverity%20Scan?branchName=master)|Coverity Scan|[Analysis Results](https://scan.coverity.com/projects/open-watcom-open-watcom-v2) , [Azure Pipelines Build](https://dev.azure.com/open-watcom/open-watcom-v2/_build/latest?definitionId=16&branchName=master)|
||Releases Archive|[**All Github Releases**](https://github.com/open-watcom/open-watcom-v2/releases)
###
![WikiDocs](https://github.com/open-watcom/open-watcom-v2/workflows/WikiDocs/badge.svg)[](https://github.com/open-watcom/open-watcom-v2/actions?query=workflow%3AWikiDocs)
![Mirror](https://github.com/open-watcom/open-watcom-v2/workflows/Mirror/badge.svg)[](https://github.com/open-watcom/open-watcom-v2/actions?query=workflow%3AMirror)
###
## Welcome to the Open Watcom v2 Project! 

For more information about the project and build instructions see the GitHub wiki.


Discuss the Project on GitHub, Reddit or Discord.
----------------------------------------------------------------------------

GitHub, join the discussion [`Open Watcom on GitHub`](https://github.com/open-watcom/open-watcom-v2/discussions)

Reddit Server, join the discussion [`Open Watcom on Reddit`](https://www.reddit.com/r/OpenWatcom/)

Discord Server for Open Watcom 2.0, use following [invite link](https://discord.gg/39w5wZM89p) to setup access to Open Watcom 2.0 Discord server.

This Discord Server is moderated by the Open Watcom 2.0 Github group to remove spam, unrelated discussions about personal opinions, etc. It is intended for user and developer assistance with Open Watcom 2.0. It is possible to ask about an older versions of Open Watcom, but it is primarily for Open Watcom V2.

Other general Discord server for "Open Watcom" exists [invite link](https://discord.gg/5WexPNn). It is mainly for older versions of Open Watcom.

[`Oficial OpenWatcom site`](http://openwatcom.org) only WEB site is up, all other services (bugzilla, Wiki, News server, Perforce) is down for long time, it looks like it is dead.

Source Tree Layout
------------------

Open Watcom allows you to place the source tree almost anywhere (although
we recommend avoiding paths containing spaces). The root of the source
tree should be specified by the `OWROOT` environment variable in `setvars`
(as described in [`Build`](https://github.com/open-watcom/open-watcom-v2/wiki/Build) document). All relative paths in this document are
taken relative to `OWROOT` location. Also this document uses the backslash
character as a path separator as is normal for DOS, Windows, and OS/2. Of
course on Linux systems a slash character should be used instead.

The directory layout is as follows:

    bld
      - The root of the build tree. Each project has a subdirectory under
        bld. For example:
          bld\cg       -> code generator
          bld\cc       -> C compiler
          bld\plusplus -> C++ compiler
          (see projects.txt for details)

    build
      - Various files used by building tools. Of most interest are the
        *.ctl files which are scripts for the builder tool (see below)
        and make files (makeint et al.).

    docs
      - Here is everything related to documentation, sources and tools.

    distrib
      - Contains manifests and scripts for building binary distribution
        packages.

    contrib
      - Third party source code which is not integral part of Open Watcom.
        This directory contains especially several DOS extenders.

    build\bin
      - This is where all build tools created during phase one are placed.

    rel
      - This is default location where the software we actually ship gets
        copied after it is built - it matches the directory structure of
        our shipping Open Watcom C/C++/FORTRAN tools. You can install the
        system by copying the rel directory to your host and then setting
        several environment variables.

        Note: the rel directory structure is created on the fly. The
        location of rel tree can be changed by `OWRELROOT` environment
        variable.

OpenWatcom Installation
-----------------------
[Installer installation instruction](https://open-watcom.github.io/open-watcom-v2-wikidocs/c_readme.html#Installation) in Documentation (OW Wiki).

OpenWatcom Building
-------------------
[Building instruction](https://github.com/open-watcom/open-watcom-v2/wiki/Build) in OW Wiki.

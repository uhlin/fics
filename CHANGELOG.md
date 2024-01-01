# Change Log #

All notable changes to this fork of FICS version 1.6.2 will be
documented in this file.

## [Unreleased] ##
- Added argument lists to many function declarations.
- Added initialization of many variables.
- Changed the addplayer program to generate passwords that are 8
  characters long (was 4).
- Deleted non-existent functions from the header files.
- Fixed unchecked return values
- Made functions and variables private (aka static) where possible.
- Reformatted code according to OpenBSD's KNF.
- Replaced `rand()` calls with arc4random.
- Replaced multiple `sprintf()` calls with `snprintf()`.
- Replaced multiple occurrences of `strcpy()` and `strcat()` with
  size-bounded versions.

## [1.0] - 2023-12-28 ##
- Added a new build system (and deleted the old).
- Added argument lists to many function declarations.
- Added auto-generation of the header file `ficspaths.h` and included
  it in `config.h`.
- Added better handling of memory allocation errors.
- Added usage of the `time_t` typedef in multiple places. This instead
  of `int`.
- Declared file-local functions and variables as `PRIVATE`.
- Deleted disabled code
- Did new revisions of the following files:
  - `board.c`
  - `eco.c`
  - `fics_addplayer.c`
  - `formula.c`
  - `makerank.c`
  - `network.c`
  - `shutdown.c`
  - `stdinclude.h`
  - `utils.c`
  - ...
- Fixed a `sscanf()` bug in `com_anews()`.
- Fixed a bug in `fix_time()` (did return a local address)
- Fixed bogus `crypt()` calls. (The second arg was wrong.)
- Fixed bugs in `process_move()`
- Fixed bugs in `stored_mail_moves()`
- Fixed comparison between pointer and integer in `com_inchannel()`.
- Fixed dead assignments
- Fixed implicit integers
- Fixed _multiple_ cases of use of possibly uninitialized variables.
- Fixed _multiple_ possible buffer overflows
- Fixed _multiple_ `sprintf()` format overflows
- Fixed the type of the variable passed to `strgtime()` and
  `strltime()`. Multiple occurrences.
- Fixed unused variables
- Reformatted code according to OpenBSD's KNF.
- Renamed functions in order to avoid conflicts with system
  declarations.

## 2023-12-07 ##
- Forked FICS version 1.6.2 made by Richard Nash.

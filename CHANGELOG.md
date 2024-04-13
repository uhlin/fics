# Change Log #

All notable changes to this fork of FICS version 1.6.2 will be
documented in this file.

## [Unreleased] ##
- Added parameter lists to many function declarations.
- Added usage of `reallocarray()` which handles multiplication
  overflow.
- Added usage of `msnprintf()`, `mstrlcpy()` and `mstrlcat()` which
  detects and logs truncation.
- Added usage of the functions from `err.h`.
- Checked out the following files by tag 1.0, because the previous
  changes made to them possibly introduced game bugs:
  - `algcheck.c`
  - `algcheck.h`
  - `board.c`
  - `movecheck.c`
  - `movecheck.h`
- Cleared sensitive data with `explicit_bzero()`.
- Fixed an empty hostname in the addplayer program.
- Fixed bogus type for storing the return value of `time()`. It should
  really be `time_t`. Multiple occurrences.
- Fixed non-ANSI function declarations of functions. (Multiple
  occurrences.)
- Made functions and variables private where possible.
- Reformatted code according to OpenBSD's KNF:
  - `ratings.c`
  - `variable.c`
  - ...
- Replaced unbounded string handling functions. _Multiple_
  occurrences.

## [1.1] - 2024-03-30 ##
- **Added** `PRINTFLIKE()` and fixed many format errors.
- **Added** argument lists to many function declarations.
- **Added** initialization of many variables.
- **Added** newly written manual pages.
- **Added** usage of `ARRAY_SIZE()`
- **Changed** the addplayer program to generate passwords that are 8
  characters long (was 4).
- **Deleted** non-existent functions from the header files.
- **Deleted** obsolete and unused code
- **Deleted** unused includes.
- **Fixed** format strings that weren't string literals. Potentially
  insecure. (_Multiple_ occurrences.)
- **Fixed** incorrect buffer sizes
- **Fixed** unchecked return values
- **Made** functions and variables private (aka static) where possible.
- **Marked** functions that doesn't return `__dead`.
- **Marked** unused function parameters.
- **Redefined** `ASSERT()`
- **Reformatted code** according to OpenBSD's KNF.
- **Replaced** `rand()` calls with arc4random.
- **Replaced** _multiple_ `sprintf()` calls with `snprintf()` + truncation
  checks.
- **Replaced** _multiple_ occurrences of `strcpy()` and `strcat()` with
  size-bounded versions.
- **Switched to** the usage of the functions from `err.h` in multiple
  places for error handling.

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

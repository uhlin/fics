# Change Log #

All notable changes to this fork of FICS version 1.6.2 will be
documented in this file.

## [Unreleased] ##
- Changed the program to avoid calculating the same string multiple
  times. Multiple occurrences, found by PVS-Studio.
- Fixed `-Wshadow` warnings. Multiple occurrences.
- Fixed double `free()` in `process_login()`.
- Fixed memory leak in `process_login()`.
- Fixed negative array index read in `accept_match()`.
- Fixed null pointer dereferences.
- Fixed possible buffer overflow in `FindHistory2()`.
- Fixed unchecked function return values. Multiple occurrences.
- Fixed uninitialized variables.
- Fixed use of 32-bit `time_t`. Y2K38 safety. Multiple occurrences.

## [1.4.4] - 2024-12-07 ##
- **Added** an autorun script suitable to be run as a cron job.
- **Added** command `sought`, which currently behaves as a no-op. Code is
  to be added in a later version. We also want the seek/unseek
  commands.
- **Added** command-line option `v` (Display version.)
- **Added** missing calls to `fclose()`.
- **Added** null checks.
- **Added** player number checks, i.e. validate that the player number is
  OK and within bounds.
- **Added** usage of `time_t`.
- **Added** usage of macros.
- **Added** variable `seek`.
- **Added** width specifications to multiple `fscanf()` and `sscanf()`
  calls, thus eliminated the risk of overflow. Multiple occurrences.
  (Found by PVS-Studio.)
- Compile using `-D_FORTIFY_SOURCE=3`.
- **Fixed** a bug in `net_send_string()`, where the expression was
  calculated as `A = (B >= C)`. (Found by PVS-Studio.)
- **Fixed** bughouse. (A board was missing.)
- **Fixed** bugs in `game_write_complete()`.
- **Fixed** bugs in `movesToString()`.
- **Fixed** cases of possible out-of-bounds array access.
- **Fixed** ignored return values of important functions such as
  `fgets()`, `fscanf()` and `sscanf()`. Multiple occurrences.
- **Fixed** incorrect format strings.
- **Fixed** uninitialized variables.
- Trimmed newlines after `fgets()` calls with `strcspn()`.
- Usage of begin/end decls in headers.

## [1.4.3] - 2024-08-04 ##
- **Added** command-line option `d` (Run in the background.)
- **Changed** the makefiles to compile with debugging symbols enabled.
- **Changed** the program to handle the return value of `fgets()`,
  `fscanf()` and `sscanf()`. Multiple occurrences.
- **Fixed** a crash due to out of bounds array access.
- **Fixed** multiple possible buffer overflows.

## [1.4.2] - 2024-07-13 ##
- Added command `iset` for compatibility with XBoard, which currently
  behaves as a no-op.
- Added command-line option `l` (Display the legal notice and exit.)
- Added return value checking of multiple `fscanf()` calls.
- Added usage of `ARRAY_SIZE()`.
- Added variable 'interface' (for compatibility with XBoard).
- Fixed unusual struct allocations.
- Made functions and variables private where possible.
- Replaced `bcopy()` with `memmove()`.
- Replaced `index()` with `strchr()`.
- Replaced `rindex()` with `strrchr()`.
- Replaced calls to `rand()` with `brand()` which uses
  `arc4random_uniform()`.
- Replaced unbounded string handling functions. Multiple
  occurrences. Whole tree completed.

## [1.4.1] - 2024-05-26 ##
- **Added** command-line option `a` to the addplayer program. If given, it
  adds a player with admin privileges.
- **Added** usage of `time_t`.
- **Fixed** out of bounds array access in `match_command()`.
- **Fixed** resource leaks, i.e. missing calls to `fclose()`.
- **Fixed** usage of possibly uninitialized variables.

## [1.4] - 2024-05-20 ##
- **Added** usage of `time_t`. Multiple occurrences.
- **Changed** the program to create news index files even if no old ones
  are existent on the disk.
- **Fixed** clang warnings.
- **Fixed** sign compare (`-Wsign-compare`). Multiple occurrences.
- **Reformatted code** according to OpenBSD's KNF. Whole tree completed.
- **Replaced** unbounded string handling functions.

## [1.3] - 2024-05-05 ##
- **Added** parameter lists to many function declarations.
- **Added** usage of `ARRAY_SIZE()`.
- **Added** usage of `reallocarray()`. Multiple occurrences.
- **Added** usage of the functions from `err.h`.
- **Changed** the make install target to not overwrite the data messages
  in case they're already present.
- **Deleted** unused includes.
- **Fixed** passing argument 2 of `ReadGameAttrs()` from incompatible
  pointer type in `jsave_history()`.
- **Made** functions and variables private where possible.
- **Reformatted code** according to OpenBSD's KNF:
  - `algcheck.c`
  - `command.c`
  - `gamedb.c`
  - `movecheck.c`
  - `obsproc.c`
  - `talkproc.c`
  - ...
- **Replaced** unbounded string handling functions. _Multiple_
  occurrences.

## [1.2] - 2024-04-14 ##
- **Added** parameter lists to many function declarations.
- **Added** usage of `reallocarray()` which handles multiplication
  overflow.
- **Added** usage of `msnprintf()`, `mstrlcpy()` and `mstrlcat()` which
  detects and logs truncation.
- **Added** usage of the functions from `err.h`.
- **Checked out** the following files by tag 1.0, because the previous
  changes made to them possibly introduced game bugs:
  - `algcheck.c`
  - `algcheck.h`
  - `board.c`
  - `movecheck.c`
  - `movecheck.h`
- **Cleared** sensitive data with `explicit_bzero()`.
- **Fixed** an empty hostname in the addplayer program.
- **Fixed** bogus type for storing the return value of `time()`. It should
  really be `time_t`. Multiple occurrences.
- **Fixed** non-ANSI function declarations of functions. (Multiple
  occurrences.)
- **Made** functions and variables private where possible.
- **Reformatted code** according to OpenBSD's KNF:
  - `ratings.c`
  - `variable.c`
  - ...
- **Replaced** unbounded string handling functions. _Multiple_
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

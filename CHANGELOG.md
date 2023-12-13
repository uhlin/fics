# Change Log #

All notable changes to this fork of FICS version 1.6.2 will be
documented in this file.

## [Unreleased] ##
- Added a new build system (and deleted the old).
- Deleted disabled code
- Did new revisions of the following files:
  - `fics_addplayer.c`
  - `network.c`
  - `stdinclude.h`
- Fixed a `sscanf()` bug in `com_anews()`.
- Fixed a bug in `fix_time()` (did return a local address)
- Fixed bugs in `stored_mail_moves()`
- Fixed comparison between pointer and integer in `com_inchannel()`.
- Fixed implicit integers
- Fixed _multiple_ cases of use of possibly uninitialized variables.
- Fixed _multiple_ possible buffer overflows
- Fixed unused variables
- Renamed functions in order to avoid conflicts with system
  declarations.

## 2023-12-07 ##
- Forked FICS version 1.6.2 made by Richard Nash.

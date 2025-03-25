# README #

## About ##

This is a fork of FICS(Free Internet Chess Server) version 1.6.2 made
by Richard Nash.
The fork is maintained by the
[RPBLC computing network](https://www.rpblc.net)
and in particular
[Markus Uhlin](mailto:maxxe@rpblc.net).
The main goal of the fork is to modernize the codebase, improve the
security and fix bugs. New features, for example, other chess variants
will be added in a later stage.

### IPv6 ###

IPv6 connections are at the moment not supported.

## SAST Tools ##

[PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

## Installation ##

Begin the installation with creating a new user, dedicated for running
the FICS.

    # useradd -c "FICS user" -m -p "AccountPassword" chess

When the user has been created login to its account and clone the
[Git](https://git-scm.com)
repository:

    $ su -l chess
    $ mkdir git
    $ cd git
    $ git clone https://github.com/uhlin/fics.git
    $ cd fics

### Checkout ###

If you want you can checkout a specific version. For example:

    $ git checkout 1.4.4

To see all tags, type:

    $ git tag

Edit `FICS/config.h` with a text editor and save the file.

    $ emacs FICS/config.h

From the top-level directory of the cloned Git repository begin the
building by running make.

    $ make

If the build was successful it's time to install the chess server by
running `make install`.

    $ make install

Done!

**NOTE**:
Running `make install` multiple times is totally fine and does no harm
when a new version of FICS is available.

### Make variables ###

If you want you can customize the `FICS_HOME` and `PREFIX` make
variables found in `options.mk`. But it isn't needed if you've
followed the installation steps above.

### Manual pages ###

To install the manual pages, type:

    $ sudo make install-manpages

Replace `sudo` with `doas` if you are running OpenBSD.
The destination will always be prefixed to `/usr/local`, i.e. system wide,
therefore extra privileges are required.

Perhaps you don't want to add the dedicated chess user to the sudoers
list. In fact YOU SHOULDN'T if you care about security. Instead login
as root, change directory and install the manual pages.

## Running the server ##

As root, do the following (unless you aren't already logged in as
`chess`):

    # su -l chess
    $ screen fics

This assumes that
[GNU Screen](https://www.gnu.org/software/screen/)
is installed on your computer.

To detach the screen, type:

    $ (CTRL+a+d)

And to resume, use:

    $ screen -r

The same is achievable with the terminal multiplexer `tmux` in case
you prefer that over GNU Screen.

### Install GNU Screen ###

To install
[GNU Screen](https://www.gnu.org/software/screen/)
on
[OpenBSD](http://www.openbsd.org/), run:

    # pkg_add -i screen

### Run in the background ###

Starting from version `rpblc-1.4.3` you can run the server in the
background by typing:

    $ fics -d

### Run as a cron job ###

FICS comes with an autorun script (`fics_autorun.sh`) that is suitable
to be run as a cron job for the dedicated chess user.

Edit the crontab by typing:

    $ crontab -e

And add the following line:

    */5 * * * * /home/chess/bin/fics_autorun.sh

Save the file and exit.

To display the current crontab, type:

    $ crontab -l

## Creating registered players ##

As root, do the following:

    # su -l chess
    $ fics_addplayer UserName FullName EmailAddress

Log out:

    $ (CTRL+d)

The `fics_addplayer` program is located in `/home/chess/bin`,
along with `makerank`.
The dir `/home/chess/bin` should in general be automatically added to
the PATH for the chess user.

    $ man 1 fics_addplayer
    $ man 1 makerank

Happy gaming!

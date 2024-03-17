# README #

## About ##

This is a fork of FICS(Free Internet Chess Server) version 1.6.2 made
by Richard Nash.
The fork is maintained by the
[RPBLC computing network](https://www.rpblc.net)
and in particular
[Markus Uhlin](mailto:maxxe@rpblc.net).
One goal of the fork is to modernize the code and fix bugs.

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

Edit `FICS/config.h` with a text editor and save the file.

    $ emacs FICS/config.h

From the top-level directory of the cloned Git repository begin the
building by running make.

    $ make

If the build was successful it's time to install the chess server by
running `make install`.

    $ make install

Done!

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

Happy gaming!

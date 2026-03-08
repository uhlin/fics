#!/bin/sh

cov-build --dir cov-int make
tar czvf fics.tgz cov-int

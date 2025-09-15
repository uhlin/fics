#ifndef ASSERT_ERROR_H
#define ASSERT_ERROR_H

#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif

__dead void _assert_error(const char *, const long int);

#endif

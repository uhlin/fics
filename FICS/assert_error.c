#include <stdio.h>
#include <stdlib.h>

#include "assert_error.h"

__dead void
_assert_error(const char *file, const long int line)
{
	(void) fprintf(stderr, "Assertion failed: file %s, line %ld.\n",
	    file, line);
	abort();
}

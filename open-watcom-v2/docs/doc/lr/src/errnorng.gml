When the argument is outside the permissible range, the
.reffunc matherr
function is called.
Unless the default
.reffunc matherr
function is replaced, it will set the global variable
.kw errno
to
.kw ERANGE
.ct , and print a "RANGE error" diagnostic message using the
.kw stderr
stream.

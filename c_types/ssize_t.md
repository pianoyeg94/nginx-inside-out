Alias for `long int`, type of a byte count, or error.

In short, `ssize_t` is the same as `size_t`, but is a `signed` type - read `ssize_t` as "signed size_t".
`ssize_t` is able to represent the number -1, which is returned by several system calls and library functions as a way to indicate error. 
For example, the `read` and `write` system calls.
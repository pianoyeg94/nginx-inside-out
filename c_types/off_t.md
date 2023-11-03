```c
/* <bits/types.h> */
#if __WORDSIZE == 32
typedef int __off64_t;
#elif __WORDSIZE == 64
typedef long int __off64_t;	/* Type of file sizes and offsets (LFS). */
#endif
/* <sys/types.h> */
typedef __off64_t off_t;
```
```c
/* <bits/types.h> on 64-bit platforms */ 
#define __STD_TYPE		       typedef
#define __SLONGWORD_TYPE	   long int
#define __SYSCALL_SLONG_TYPE   __SLONGWORD_TYPE
#define __TIME_T_TYPE		   __SYSCALL_SLONG_TYPE
__STD_TYPE __TIME_T_TYPE       __time_t;	        /* Seconds since the Epoch.  */

/* <bits/types/time_t.h> => <sys/types.h> */ 
typedef __time_t               time_t;
```
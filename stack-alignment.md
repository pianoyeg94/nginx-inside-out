// Align stack pointer by 16 for C code (required by System V Application Binary Interface AMD64) 
                                      https://stackoverflow.com/questions/672461/what-is-stack-alignment
                                      https://stackoverflow.com/questions/4175281/what-does-it-mean-to-align-the-stack
                                      https://www.isabekov.pro/stack-alignment-when-mixing-asm-and-c-code/

The point of this is that there are some "SIMD" (Single Instruction, Multiple Data) instructions 
(also known in x86-land as "SSE" for "Streaming SIMD Extensions") which can perform parallel operations 
on multiple words in memory, but require those multiple words to be a block starting at an address which 
is a multiple of 16 bytes.

In general, the compiler can't assume that particular offsets from RSP will result in a suitable address 
(because the state of RSP on entry to the function depends on the calling code). But, by deliberately 
aligning the stack pointer in this way, the compiler knows that adding any multiple of 16 bytes to 
the stack pointer will result in a 16-byte aligned address, which is safe for use with these SIMD 
instructions.

The value of RSP is 0x7fffffffdc68 which is not a multiple of 16 because the value of RSP was decreased 
by 8 bytes after pushing RBX to the stack before calling printf function in the modified version of the code.
This becomes crucial when instructions such as "movaps" (move packed single-precision floating-point values 
from xmm2/m128 to xmm1) require 16 byte alignment of the memory operand. Misaligned stack together with "movaps" 
instruction in the printf function cause segmentation fault.

For this reason, the System V Application Binary Interface AMD64 provides the stack alignment requirement in 
subsection "The Stack Frame": 
"The end of the input argument area shall be aligned on a 16 (32, if __m256 is passed on stack) byte boundary. 
In other words, the value (RSP + 8) is always a multiple of 16 (32) when control is transferred to the function 
entry point."

The value of RSP before calling any function has to be in the format of 0x???????????????0 where ? represents 
any 4-bit hexadecimal number. Zero value of the least significant nibble provides 16 byte alignment of the stack.

Some CPU architectures require specific alignment of various datatypes, and will throw exceptions if you don't honor this rule. In standard mode, x86 doesn't require this for the basic data types, but can suffer performance penalties (check www.agner.org for low-level optimization tips).

However, the SSE instruction set (often used for high-performance) audio/video procesing has strict alignment requirements, and will throw exceptions if you attempt to use it on unaligned data (unless you use the, on some processors, much slower unaligned versions).

Your issue is probably that one compiler expects the caller to keep the stack aligned, while the other expects callee to align the stack when necessary.

EDIT: as for why the exception happens, a routine in the DLL probably wants to use SSE instructions on some temporary stack data, and fails because the two different compilers don't agree on calling conventions.
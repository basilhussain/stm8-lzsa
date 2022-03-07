# Overview

This is a library for the STM8 microcontroller and [SDCC](http://sdcc.sourceforge.net/) compiler providing [LZSA](https://github.com/emmanuel-marty/lzsa) raw block decompression routines that have been written in hand-optimised assembly code for fastest possible execution speed, with a secondary aim of smaller code size. Both LZSA1 and LZSA2 formats are supported.

LZSA is a compression format similar to LZ4 that is specifically designed for very fast decompression on 8-bit systems (such as the STM8). Up to 64 Kb of data can be compressed per block (memory capacity of target device permitting, of course). Of the two format variations, LZSA2 gives a slightly better compression ratio than LZSA1, but at the expense of more complicated (and thus slower) decompression.

In addition to the library, a test and benchmark program (in C) is also included that contains reference implementations of LZSA block decompression, used to verify proper operation and to benchmark against.

# Setup

You may either use a pre-compiled version of the library, or build the library code yourself. See below for further details.

This library has been written to accommodate and provide for both 'medium' (16-bit address space) and 'large' (24-bit address space) STM8 memory models.

* If you are building your project with either no specific SDCC memory model option, or the `--model-medium` option, then use the non-suffixed `lzsa.lib` library file.
* If you are building with `--model-large`, then use the `lzsa-large.lib` library file.

Unsure? If your target STM8 microcontroller model has less than 32KB of flash memory, then choose the former version; if larger flash, then you probably want the latter.

## Pre-compiled Library

1. Extract the relevant `.lib` file (see above) and `lzsa.h` file from the release archive.
2. Copy the two files to your project.

## Building

This library is developed and built with the [Code::Blocks](http://codeblocks.org/) IDE and [SDCC](http://sdcc.sourceforge.net/) compiler.

1. Load the `.cbp` project file in Code::Blocks.
2. Select the appropriate 'Library' build target for your STM8 memory model (see above) from the drop-down list on the compiler toolbar (or the *Build > Select Target* menu).
3. Build the library by pressing the 'Rebuild' icon on the compiler toolbar (or Ctrl-F11 keyboard shortcut, or *Build > Rebuild* menu entry).
4. Upon successful compilation, the resultant `.lib` file will be in the main base folder.
5. Copy the `.lib` file and the `lzsa.h` file to your project.

**Note:** A *rebuild* should always be performed, rather than build, due to dependencies that are unrecognised by Code::Blocks, so it does not know to rebuild some files if others they depend on or include have changed.

# Usage

1. Include the `lzsa.h` file in your C code wherever you want to use the library functions.
2. When linking, provide the path to the `.lib` file with the `-l` SDCC command-line option.

## Function Reference

### `void * lzsa1_decompress_block(void *dst, const void *src)`

Decompresses a raw block of LZSA1 format data.

Takes as arguments two pointers: `dst` is a pointer to a destination buffer that the decompressed data will be written to; `src` is a pointer to the beginning of the source compressed data block.

Returns a pointer to a position in the given destination buffer after the last byte of decompressed data.

### `void * lzsa2_decompress_block(void *dst, const void *src)`

Decompresses a raw block of LZSA2 format data.

Takes as arguments two pointers: `dst` is a pointer to a destination buffer that the decompressed data will be written to; `src` is a pointer to the beginning of the source compressed data block.

Returns a pointer to a position in the given destination buffer after the last byte of decompressed data.

## Notes, Caveats & Warnings

* You must ensure that the destination buffer is large enough to contain the uncompressed data! No checks are performed or limits considered when writing the decompressed data, so buffer overflow may occur if the buffer is of insufficient size.
* The decompression routines do not presently work with blocks that are part of a stream. Such blocks do not contain end-of-data (EOD) markers.
* It is assumed that all compressed data is correctly formed. There is no error detection or handling.
* These functions are not re-entrant, due to the use of static variables. Do not call them from within interrupt service routines when they are also being called elsewhere.
* The size in bytes of the resultant uncompressed data may be ascertained by subtracting the original `dst` pointer from the returned pointer value.

## Example

```c
#include <stddef.h>
#include <stdint.h>
#include "lzsa.h"

void main(void) {
    static const uint8_t in[] = { /* compressed block data... */ };
    static uint8_t out[MAX_UNCOMPRESSED_SIZE]; // define size according to data
    ptrdiff_t out_len;
    
    out_len = lzsa1_decompress_block(out, in) - out;
}
```

## Compressing Data

Raw block data can be compressed using Emmanuel Marty's [LZSA compression tool](https://github.com/emmanuel-marty/lzsa/releases), with the following command line:

`lzsa -f<1|2> -r <input_file> <output_file>`

Make sure to specify either LZSA1 (`-f1`) or LZSA2 (`-f2`) format, and raw block output (`-r`). Note that backwards compression (`-b`) is not supported by this library, nor is a minimum match size (`-m`) of anything other than the default of 3 (although the code could be changed to support other sizes).

# Benchmarks

To benchmark the decompression routines, the execution speed was compared with that of their associated plain C reference implementations (see `lzsa_ref.c`). Each function was run for 100 iterations on a complex sample of compressed data (which should exercise all code paths) and the total number of processor execution cycles measured.

| Function               | Reference C Cycles | Library ASM Cycles | Ratio |
| ---------------------- | -----------------: | -----------------: | ----: |
| lzsa1_decompress_block |          9,096,111 |          4,629,720 |   51% |
| lzsa2_decompress_block |         13,221,811 |          5,732,220 |   43% |

The above benchmark was run using the [μCsim](http://mazsola.iit.uni-miskolc.hu/~drdani/embedded/ucsim/) microcontroller simulator included with SDCC, and measurements were obtained using the timer commands of the simulator.

The same benchmark was also run on physical STM8 hardware, an STM8S208RBT6 Nucleo-64 development board running at 16 MHz, and execution time measured by capturing the toggling of a pin with a logic analyser.

| Function               | Reference C Time (ms) | Library ASM Time (ms) | Ratio |
| ---------------------- | --------------------: | --------------------: | ----: |
| lzsa1_decompress_block |                 626.6 |                 292.3 |   47% |
| lzsa2_decompress_block |                 887.7 |                 363.8 |   41% |

Other notes:

* The count of cycles consumed shown here includes the loop iteration, but for the purposes of comparison, because it is a common overhead and counts equally against both implementations, this can be ignored.
* All C code was compiled using SDCC's default 'balanced' optimisation level (i.e. with neither `--opt-code-speed` or `--opt-code-size`).
* The C code could possibly be faster with some optimisation, but it was chosen to write straightforward and idiomatic implementations based solely on the specification of the compression format, without reference to any other implementations.

# Test Program

A test and benchmark program, `main.c`, is included in the source repository. It is designed to be run with the [μCsim](http://mazsola.iit.uni-miskolc.hu/~drdani/embedded/ucsim/) microcontroller simulator included with SDCC, but should also run as-is on physical STM8 hardware that uses an STM8S208RB (such as ST's Nucleo-64 development board equipped with this chip). It could also be adapted for other STM8 devices, but note that it is unsuitable for running on any lower-end devices with 16 Kb or less of flash, due to the extensive space needed for test case data.

The program will run tests using a variety of compressed input data and compare the decompressed output to the intended plain uncompressed data. A string of "PASS" or "FAIL" is given, depending on whether the data matches. At the conclusion of all tests, total pass/fail counts will be output.

For details of the benchmark part of the program, please see the [Benchmarks](#benchmarks) section. Please note that the benchmark is primarily designed to be run under the μCsim simulator.

When executing in μCsim, all output from the program is directed to the simulator console. When executing on physical hardware, all output is transmitted on UART1.

# Licence

This library is licenced under the MIT Licence. Please see file LICENSE.txt for full licence text.
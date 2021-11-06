# Overview

This is a library for the STM8 microcontroller and [SDCC](http://sdcc.sourceforge.net/) compiler providing an [LZSA1](https://github.com/emmanuel-marty/lzsa) raw block decompression routine that has been written in hand-optimised assembly code for fastest possible execution speed, with a secondary aim of smaller code size.

LZSA1 is a compression format similar to LZ4 that is specifically designed for very fast decompression on 8-bit systems (such as the STM8). Up to 64 Kb of data can be compressed per block (memory capacity of target device permitting, of course).

In addition to the library, a test and benchmark program (in C) is also included that contains a reference implementation of LZSA1 block decompression, used to verify proper operation and to benchmark against.

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

**Note:** A *rebuild* should always be performed, rather than build, due to a quirk in behaviour related to the use of ASM source files. Code::Blocks does not know that `lzsa1.s` is assembled together with one of either `lzsa1_large.s` or `lzsa1_medium.s` files (depending on target), so does not know to rebuild the former if either of the latter has changed.

# Usage

1. Include the `lzsa.h` file in your C code wherever you want to use the library functions.
2. When linking, provide the path to the `.lib` file with the `-l` SDCC command-line option.

## Function Reference

### `void * lzsa1_decompress_block(void *dst, const void *src)`

Takes as arguments two pointers: `dst` is a pointer to a destination buffer that the decompressed data will be written to; `src` is a pointer to the beginning of the source compressed data block.

Returns a pointer to a position in the given destination buffer after the last byte of decompressed data. The size in bytes of the uncompressed data may be ascertained by subtracting the `dst` pointer from the returned pointer.

**Warning:** this function is not re-entrant, due to the use of static variables. Do not call from within interrupt service routines when it is also called elsewhere.

**Note:** you must ensure that the destination buffer is large enough to contain the uncompressed data! No checks are performed or limits considered when writing the decompressed data, so buffer overflow may occur if the buffer is of insufficient size.

**Note:** this function does not work with blocks that are part of a stream. This is because stream blocks do not have final EOD bytes.

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

`lzsa -f1 -r <input_file> <output_file>`

Make sure to specify LZSA1 format (`-f1`) and raw block output (`-r`). Note that backwards compression (`-b`) is not supported by this library, nor is a minimum match size (`-m`) of anything other than the default of 3 (although the code could be changed to support other sizes).

# Benchmarks

To benchmark the decompression routine, the execution speed was compared with that of it's associated plain C reference implementation (see `lzsa_ref.c`). Each function was run for 1,000 iterations and the total number of processor execution cycles measured.

| Function               | Reference C Cycles | Library ASM Cycles | Ratio |
| ---------------------- | -----------------: | -----------------: | ----: |
| lzsa1_decompress_block |          1,984,014 |          1,074,010 |   54% |

The above benchmark was run using the [μCsim](http://mazsola.iit.uni-miskolc.hu/~drdani/embedded/ucsim/) microcontroller simulator included with SDCC, and measurements were obtained using the timer commands of the simulator.

The same benchmark was also run on physical STM8 hardware, an STM8S208RBT6 Nucleo-64 development board, and execution time measured by capturing the toggling of a pin with a logic analyser.

| Function               | Reference C Time | Library ASM Time | Ratio |
| ---------------------- | ---------------: | ---------------: | ----: |
| lzsa1_decompress_block |         171.7 ms |          85.5 ms |   50% |

The results correspond approximately to the results from the simulator benchmark.

Other notes:

* The count of cycles consumed shown here includes the loop iteration, but for the purposes of comparison, because it is a common overhead and counts equally against both implementations, this can be ignored.
* All C code was compiled using SDCC's default 'balanced' optimisation level (i.e. with neither `--opt-code-speed` or `--opt-code-size`).

# Test Program

A test and benchmark program, `main.c`, is included in the source repository. It is designed to be run with the [μCsim](http://mazsola.iit.uni-miskolc.hu/~drdani/embedded/ucsim/) microcontroller simulator included with SDCC, but will also run as-is on physical STM8 hardware that uses an STM8S208RB (such as ST's Nucleo-64 development board equipped with this chip). Also note that the test program is unsuitable for running on any lower-end STM8 devices with 16 Kb or less of flash, due to the extensive space needed for test case data.

The program will run tests using a variety of compressed input data and compare the decompressed output to the intended plain uncompressed data. A string of "PASS" or "FAIL" is given, depending on whether the data matches. At the conclusion of all tests, total pass/fail counts will be output.

For details of the benchmark part of the program, please see the [Benchmarks](#benchmarks) section. Please note that the benchmark is primarily designed to be run under the μCsim simulator.

When executing in μCsim, all output from the program is directed to the simulator console. When executing on physical hardware, all output is transmitted on UART1.

# Licence

This library is licensed under the MIT Licence. Please see file LICENSE.txt for full licence text.
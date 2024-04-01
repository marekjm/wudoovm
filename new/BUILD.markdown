# Building instructions

The compilation process is automated using GNU Make, and in its simples form
looks like this:

    ]$ make -j

--------------------------------------------------------------------------------

## Build presets

There are several presets:

 - `default`: an unoptimised build, with sanitisers and plenty of compiler
   warnings enabled
 - `debug`: an unoptimised build, with plenty of compiler warnings, but without
   sanitisers -- use this if you want to use Valgrind or GDB
 - `release`: an optimised build, with sanitisers and plenty of compiler
   warnings enabled

The presets are defined in `Makefile.d/Preset/` directory.

How to choose a preset?

    ]$ make PRESET=preset -j

--------------------------------------------------------------------------------

## Tweaking flags

Various aspects of the build can be tweaked.
However, it is recommended to use presets unless you know what you are doing.

### C++ compiler and standard

    ]$ make CXX=... CXXSTD=...

### Sanitisers

    ]$ make CXXFLAGS_SANITISER=...

### Compiler options

    ]$ make CXXFLAGS_OPTION=...

### Compiler warnings

    ]$ make CXXFLAGS_NOERROR=... CXXFLAGS_WARNING=...

The `CXXFLAGS_WARNING` variable sets the list of enabled compiler warnings --
which will be treated as errors, since the build *always* runs with the
`-Werror` flag enabled.

The `CXXFLAGS_NOERROR` variable sets the list of warnings which are not treated
as errors.

### Optimisation level

    ]$ make CXXFLAGS_OPTIMISATION=...

### Debug symbols

    ]$ make CXXFLAGS_DEBUG=...

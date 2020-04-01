#===============================================================================
# User Options
#===============================================================================

COMPILER  = gnu
OPTIMIZE  = yes
DEBUG     = yes
PROFILE   = no
MPI       = no
PAPI      = no
VEC_INFO  = no
VERIFY    = yes
PAUSE     = no
BENCHMARK = no

#===============================================================================
# Program name & source code list
#===============================================================================

program = XSBench

source = \
CalculateXS.c \
GridInit.c \
XSutils.c \
Materials.c

obj = $(source:.c=.o)

#===============================================================================
# Sets Flags
#===============================================================================

# Regular gcc Compiler
ifeq ($(COMPILER),gnu)
  GCC = gcc
endif

# BG/Q gcc Cross-Compiler
ifeq ($(MACHINE),bluegene)
  GCC = mpicc
endif

# Standard Flags
GCC_S_FLAGS := -fopenmp -std=gnu99

# Optional/Extra Flags
GCC_O_FLAGS =

# Linker Flags
LDFLAGS = -lm

# Debug Flags
ifeq ($(DEBUG),yes)
  GCC_O_FLAGS += -g
  LDFLAGS  += -g
endif

# Profiling Flags
ifeq ($(PROFILE),yes)
  GCC_O_FLAGS += -pg
  LDFLAGS  += -pg
endif

# Optimization Flags
ifeq ($(OPTIMIZE),yes)
  GCC_O_FLAGS += -O3
endif

# Compiler Vectorization (needs -O3 flag) information
ifeq ($(VEC_INFO),yes)
  GCC_O_FLAGS += -ftree-vectorizer-verbose=6
endif

# PAPI source (you may need to provide -I and -L pointing
# to PAPI depending on your installation
ifeq ($(PAPI),yes)
  source += papi.c
  GCC_O_FLAGS += -DPAPI
  #GCC_S_FLAGS += -I/soft/apps/packages/papi/papi-5.1.1/include
  #LDFLAGS += -L/soft/apps/packages/papi/papi-5.1.1/lib -lpapi
  LDFLAGS += -lpapi
endif

# MPI
ifeq ($(MPI),yes)
  GCC = mpicc
  GCC_S_FLAGS += -DMPI
endif

# Verification of results mode
ifeq ($(VERIFY),yes)
  GCC_S_FLAGS += -DVERIFICATION
endif

# Injects artificial pause between XS lookups
# to represent time for particle tracking
ifeq ($(PAUSE),yes)
  GCC_S_FLAGS += -DPAUSE
endif

# Adds outer 'benchmarking' loop to do multiple trials for
# 1 < threads <= max_threads
ifeq ($(BENCHMARK),yes)
  GCC_S_FLAGS += -DBENCHMARK
endif
#===============================================================================
# Targets to Build
#===============================================================================
all: clean $(program) xs_server xs_client

$(program): $(obj) do_flops.o XSbench_header.h
	$(GCC) $(GCC_S_FLAGS) $(GCC_O_FLAGS) Main.c do_flops.o $(obj)   -o $@ $(LDFLAGS) -L/g/g90/peng8/flash/XSBench/umap/build/lib -lumap -lmargo 

xs_server: $(obj) do_flops.o XSbench_header.h server.c
	$(GCC) $(GCC_S_FLAGS) $(GCC_O_FLAGS) server.c do_flops.o $(obj) -o $@ -I/g/g90/peng8/flash/XSBench/umap/build/include $(LDFLAGS) -L/g/g90/peng8/flash/XSBench/umap/build/lib -lumap -lmargo

xs_client: $(obj) do_flops.o XSbench_header.h client.c
	$(GCC) $(GCC_S_FLAGS) $(GCC_O_FLAGS) client.c do_flops.o $(obj) -o $@ -I/g/g90/peng8/flash/XSBench/umap/build/include $(LDFLAGS) -L/g/g90/peng8/flash/XSBench/umap/build/lib -lumap -lmargo

%.o: %.c
	$(GCC) $(GCC_S_FLAGS) $(GCC_O_FLAGS) -c $< -o $@  -I/g/g90/peng8/flash/XSBench/umap/build/include 

do_flops.o: do_flops.c
	$(GCC) $(GCC_S_FLAGS) -c do_flops.c

clean:
	rm -rf XSBench XSBench.dSYM *.o xs_server xs_client

edit:
	vim -p $(source) do_flops.c papi.c XSbench_header.h

run:
	./XSBench

bgqrun:
	qsub -t 10 -n 1 -O test XSBench

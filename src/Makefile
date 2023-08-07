NETWORK_NAME = nn-7e82d78ef76a870-450.nnue

_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
EVALFILE   = $(_ROOT)/$(NETWORK_NAME)

COMP       =

CXX       := g++
CXXFLAGS  := -std=c++17 -O3 -Wall -Wcast-qual -fno-exceptions  -pedantic -Wextra -DNDEBUG -funroll-loops -flto

TARGET    := smallbrain
NATIVE    := -march=native


NAME      := smallbrain
SUFFIX    := .exe

LDFLAGS   := -pthread -lstdc++ -static -Wl,--no-as-needed

TMPDIR = ../.tmp

# all bmi2 instructions
INSTRUCTIONS_BMI2 = -march=haswell

# Detect Windows
ifeq ($(OS), Windows_NT)
	MKDIR    := mkdir -p
	uname_S  := Windows
	SUFFIX   := .exe
else
ifeq ($(COMP), MINGW)
	MKDIR    := mkdir
	uname_S  := Windows
	SUFFIX   := .exe
else
	MKDIR   := mkdir -p
	LDFLAGS := -lpthread -lstdc++
	uname_S := $(shell uname -s)
	SUFFIX  :=
endif
endif

ifeq ($(CXX), clang++-17)
LDFLAGS   := -pthread
endif

# SPECIFIC BUILDS
ifeq ($(build), native)
	NATIVE   = -march=native
endif

ifeq ($(build), x86-64)
	NATIVE       = -mtune=znver1
	INSTRUCTIONS = -msse -msse2 -mpopcnt
endif

ifeq ($(build), x86-64-avx2)
	NATIVE       = -mtune=znver3
	INSTRUCTIONS = -march=bdver4 -mno-tbm -mno-sse4a -mno-bmi2
endif

ifeq ($(build), x86-64-bmi2)
	NATIVE       = -mtune=znver3
	INSTRUCTIONS = $(INSTRUCTIONS_BMI2)
endif

ifeq ($(build), x86-64-avx512)
	NATIVE       = -mtune=alderlake
	INSTRUCTIONS = $(INSTRUCTIONS_BMI2) -mavx512f -mavx512bw
endif

ifeq ($(build), x86-64-vnni256)
	NATIVE       = -mtune=alderlake
	INSTRUCTIONS = $(INSTRUCTIONS_BMI2) -mavx512f -mavx512bw -mavx512vnni -mavx512dq -mavx512vl -mprefer-vector-width=256
endif

ifeq ($(build), x86-64-vnni512)
	NATIVE       = -mtune=alderlake
	INSTRUCTIONS = $(INSTRUCTIONS_BMI2) -mavx512vnni -mavx512dq -mavx512vl
endif

# Different native flag for macOS
ifeq ($(uname_S), Darwin)
	NATIVE  = -mcpu=apple-a14	
	LDFLAGS = 
endif

# Native build with debug symbols
ifeq ($(build), symbols)
	CXXFLAGS  += -g3 -fno-omit-frame-pointer
	NATIVE     = -march=native
	LDFLAGS    = -lpthread -lstdc++
endif

# OS independent build with full debug
ifeq ($(build), debug)
	CXXFLAGS   = -std=c++17 -O3 -g3 -fno-omit-frame-pointer
	NATIVE     = -msse -msse3 -mpopcnt
	LDFLAGS    = -lpthread -lstdc++
endif

# Prepend - to the build name
ifeq ($(build),)
	ARCH_NAME := 
else
	ARCH_NAME := -$(build)
endif

# Try to include git commit sha for versioning
GIT_SHA = $(shell git rev-parse --short HEAD 2>/dev/null)
ifneq ($(GIT_SHA), )
	CXXFLAGS += -DGIT_SHA=\"$(GIT_SHA)\"
endif

# Try to include git commit date for versioning
GIT_DATE = $(shell git show -s --date=format:'%Y%m%d' --format=%cd HEAD 2>/dev/null)
ifneq ($(GIT_DATE), )
	CXXFLAGS += -DGIT_DATE=\"$(GIT_DATE)\"
endif

# PGO
ifneq (,$(findstring g++, $(CXX)))
	PGO_GEN   = -fprofile-generate
	PGO_USE   = -fprofile-use
endif

ifneq (,$(findstring clang++, $(CXX)))
	PGO_MERGE = llvm-profdata merge -output=profdata *.profraw
	PGO_GEN   = -fprofile-instr-generate
	PGO_USE   = -fprofile-instr-use=profdata
endif

# Add network name and Evalfile
CXXFLAGS += -DNETWORK_NAME=\"$(NETWORK_NAME)\" -DEVALFILE=\"$(EVALFILE)\"

SOURCES := $(wildcard *.cpp) syzygy/Fathom/src/tbprobe.cpp tests/tests.cpp
OBJECTS := $(patsubst %.cpp,$(TMPDIR)/%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,$(TMPDIR)/%.d,$(SOURCES))
EXE     := $(NAME)$(ARCH_NAME)$(SUFFIX)

.PHONY: all clean FORCE

all: $(TARGET)
clean:
	@rm -rf $(TMPDIR) *.o  $(DEPENDS) *.d profdata *.gcda *.profraw

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(NATIVE) $(INSTRUCTIONS) -MMD -MP -o $(EXE) $^ $(LDFLAGS)

$(TMPDIR)/%.o: %.cpp | $(TMPDIR)
	$(CXX) $(CXXFLAGS) $(NATIVE) $(INSTRUCTIONS) -MMD -MP -c $< -o $@ $(LDFLAGS)

$(TMPDIR):
	$(MKDIR) "$(TMPDIR)" "$(TMPDIR)/src" "$(TMPDIR)/syzygy" "$(TMPDIR)/syzygy/Fathom" "$(TMPDIR)/syzygy/Fathom/src" "$(TMPDIR)/tests"

-include $(DEPENDS)

# Force recompilation of ucicommands to include latest version string
$(TMP_DIR)/uci.o: FORCE
FORCE:

# Profile Guided Optimizations for gcc, largely depends on compiler version
pgo:
	$(CXX) $(CXXFLAGS) $(PGO_GEN) $(NATIVE) $(INSTRUCTIONS) -MMD -MP -o $(EXE) $(SOURCES) $(LDFLAGS)
	./$(EXE) bench
	$(PGO_MERGE)
	$(CXX) $(CXXFLAGS) $(NATIVE) $(INSTRUCTIONS) $(PGO_USE) -MMD -MP -o $(EXE) $(SOURCES) $(LDFLAGS)
	@rm -f *.gcda *.profraw *.o $(DEPENDS) *.d  profdata

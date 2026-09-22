# dumpfloppy — C++23 floppy-image secret dumper
# Default: debug + ASan/UBSan.  make release  /  make profile  /  make test

CC  ?= gcc
CXX ?= g++
CC  := $(CC) -Wl,--as-needed
CXX := $(CXX) -Wl,--as-needed

MAKEFLAGS += --no-print-directory

export PKG_CONFIG_PATH ?= $(HOME)/.local/share/pkgconfig:$(HOME)/.local/lib64/pkgconfig:$(HOME)/.local/lib/pkgconfig:$(PKG_CONFIG_PATH)

LIBSF_INC := /usr/local/include/libsf
PREFIX    ?= /usr/local

CRYPTO_CFLAGS := $(shell pkg-config --cflags libcrypto 2>/dev/null)
CRYPTO_LIBS   := $(shell pkg-config --libs libcrypto 2>/dev/null)
ifeq ($(CRYPTO_LIBS),)
  CRYPTO_LIBS := -lcrypto
endif

XXH_CFLAGS := $(shell pkg-config --cflags libxxhash 2>/dev/null)
XXH_LIBS   := $(shell pkg-config --libs libxxhash 2>/dev/null)
ifeq ($(XXH_LIBS),)
  XXH_LIBS := -lxxhash
endif

CATCH_CFLAGS := $(shell pkg-config --cflags catch2-with-main 2>/dev/null)
CATCH_LIBS   := $(shell pkg-config --libs catch2-with-main 2>/dev/null)

WARN := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 \
        -Wnull-dereference -fstack-protector-strong

INCLUDES := -Iinclude -I$(LIBSF_INC)

CFLAGS_COMMON := -std=gnu23 $(WARN) $(INCLUDES) -fPIC -MMD -MP
CXXFLAGS_COMMON := -std=gnu++23 $(WARN) $(INCLUDES) $(CRYPTO_CFLAGS) $(XXH_CFLAGS) -fPIC -MMD -MP

LDFLAGS_COMMON := -Wl,-O1 -Wl,--hash-style=gnu -Wl,-z,relro -pthread $(CRYPTO_LIBS) $(XXH_LIBS)

CXXFLAGS_OPTIMIZED := -O3 -march=x86-64 -mtune=generic -fno-omit-frame-pointer

CXXFLAGS_DEBUG := $(CXXFLAGS_COMMON) -g3 -O0 \
                  -fsanitize=address,undefined \
                  -fno-omit-frame-pointer -rdynamic
CFLAGS_DEBUG := $(CFLAGS_COMMON) -g3 -O0 \
                -fsanitize=address,undefined \
                -fno-omit-frame-pointer -rdynamic
LDFLAGS_DEBUG := -fsanitize=address,undefined -rdynamic $(LDFLAGS_COMMON)

CXXFLAGS_RELEASE := $(CXXFLAGS_COMMON) -DNDEBUG $(CXXFLAGS_OPTIMIZED)
CFLAGS_RELEASE := $(CFLAGS_COMMON) -DNDEBUG $(CXXFLAGS_OPTIMIZED)
LDFLAGS_RELEASE := $(LDFLAGS_COMMON)

CXXFLAGS_PROFILE := $(CXXFLAGS_COMMON) -DNDEBUG $(CXXFLAGS_OPTIMIZED) \
                    -g -pg -fno-inline
CFLAGS_PROFILE := $(CFLAGS_COMMON) -DNDEBUG $(CXXFLAGS_OPTIMIZED) -g -pg -fno-inline
LDFLAGS_PROFILE := -pg $(LDFLAGS_COMMON)

CXXFLAGS ?= $(CXXFLAGS_DEBUG)
CFLAGS   ?= $(CFLAGS_DEBUG)
LDFLAGS  ?= $(LDFLAGS_DEBUG)

BUILD_FLAGS := -s V=0 -j$(shell nproc 2>/dev/null || echo 1)

TARGET := dumpfloppy
TEST_BIN := tests/run_tests

SRC_C := src/fat12_codec.c
SRC_CXX := src/util.cpp src/geometry.cpp src/image.cpp src/bpb.cpp \
           src/boot.cpp src/fat.cpp src/directory.cpp src/analyze.cpp \
           src/report.cpp src/cli.cpp src/extract.cpp src/update.cpp \
           src/format_registry.cpp src/ibm_mfm.cpp src/catalog.cpp
SRC_MAIN := src/main.cpp
TEST_SRC := tests/test_fat12.cpp tests/test_cli.cpp tests/test_geometry.cpp \
            tests/test_image.cpp tests/test_bin.cpp tests/test_extract.cpp \
            tests/test_format.cpp tests/test_update.cpp tests/test_deleted.cpp

OBJ_C := $(SRC_C:.c=.o)
OBJ_CXX := $(SRC_CXX:.cpp=.o)
OBJ_MAIN := $(SRC_MAIN:.cpp=.o)
DEP := $(OBJ_C:.o=.d) $(OBJ_CXX:.o=.d) $(OBJ_MAIN:.o=.d)

GEN_FMT := include/dumpfloppy/formats/generated_formats.h
FMT_HDRS := $(wildcard include/dumpfloppy/formats/archiveteam/*.h) \
            $(wildcard include/dumpfloppy/formats/shikadi/*.h)

$(GEN_FMT): scripts/gen_format_registry.py $(FMT_HDRS)
	python3 scripts/gen_format_registry.py

src/format_registry.o: $(GEN_FMT)

.PHONY: all clean test tests verify release profile install tags docs man-lint

all: $(TARGET)

$(TARGET): $(OBJ_C) $(OBJ_CXX) $(OBJ_MAIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJ_C) $(OBJ_CXX) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -Itests $(CATCH_CFLAGS) $(OBJ_C) $(OBJ_CXX) $(TEST_SRC) \
	  -o $@ $(LDFLAGS) $(CATCH_LIBS)

test: $(TARGET) $(TEST_BIN)
	DUMPFLOPPY_BIN=$(CURDIR)/$(TARGET) ./$(TEST_BIN)

tests: test

verify: test
	$(HOME)/.local/bin/cbmc src/fat12_codec.c formal/harness_fat12.c \
	  -I include --bounds-check --pointer-check --unwind 4 \
	  --unwinding-assertions

release:
	$(MAKE) $(BUILD_FLAGS) clean \
	  CFLAGS="$(CFLAGS_RELEASE)" CXXFLAGS="$(CXXFLAGS_RELEASE)" \
	  LDFLAGS="$(LDFLAGS_RELEASE)" all
	@echo "Release binary built: $(TARGET)"

profile:
	$(MAKE) $(BUILD_FLAGS) clean \
	  CFLAGS="$(CFLAGS_PROFILE)" CXXFLAGS="$(CXXFLAGS_PROFILE)" \
	  LDFLAGS="$(LDFLAGS_PROFILE)" all
	@echo "Profile binary built: $(TARGET)"

install: $(TARGET) man/dumpfloppy.1
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 644 man/dumpfloppy.1 $(DESTDIR)$(PREFIX)/share/man/man1/dumpfloppy.1

tags:
	ctags -R --languages=C,C++ --exclude=.git --exclude=build -f tags include src

docs:
	doxygen Doxyfile

man-lint:
	mandoc -T lint man/dumpfloppy.1

clean:
	rm -f $(TARGET) $(TEST_BIN) $(OBJ_C) $(OBJ_CXX) $(OBJ_MAIN) $(DEP) \
	      gmon.out profile.txt tags
	rm -rf html latex

-include $(DEP)

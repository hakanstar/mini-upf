# mini-upf build
#   make            build ./build/mini-upf
#   make test       unit tests (C)
#   make functest   functional tests (pytest + scapy, no root needed)
#   make check      test + functest
#   make SANITIZE=1 ...   build everything with ASan + UBSan
#   make lint       cppcheck + clang-format check
#   make fuzz       libFuzzer target (clang); seed corpus in tests/fuzz/corpus
#   make clean

CC      ?= gcc
BUILD   ?= build
CSTD    := -std=c11
WARN    := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion \
           -Wstrict-prototypes -Wmissing-prototypes -Werror
CPPFLAGS += -Iinclude -D_POSIX_C_SOURCE=200809L
CFLAGS  ?= -O2 -g
CFLAGS  += $(CSTD) $(WARN)

ifeq ($(SANITIZE),1)
  CFLAGS  += -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
  LDFLAGS += -fsanitize=address,undefined
endif

LIB_SRC := src/gtpu.c src/stats.c
LIB_OBJ := $(LIB_SRC:%.c=$(BUILD)/%.o)
APP_OBJ := $(BUILD)/src/main.o

UNIT_SRC := $(wildcard tests/unit/test_*.c)
UNIT_BIN := $(UNIT_SRC:tests/unit/%.c=$(BUILD)/%)

.PHONY: all test functest check lint format fuzz clean

all: $(BUILD)/mini-upf

$(BUILD)/mini-upf: $(LIB_OBJ) $(APP_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

# unit tests do not need -Wmissing-prototypes for TEST() functions
$(BUILD)/test_%: tests/unit/test_%.c $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wno-missing-prototypes -Itests/unit -o $@ $^ $(LDFLAGS)

test: $(UNIT_BIN)
	@set -e; for t in $(UNIT_BIN); do echo "== $$t"; ./$$t; done

functest: $(BUILD)/mini-upf
	MINI_UPF_BIN=$(abspath $(BUILD)/mini-upf) python3 -m pytest -q tests/func

check: test functest

lint:
	cppcheck --enable=warning,style,performance,portability --error-exitcode=1 \
	  --inline-suppr --std=c11 -Iinclude --suppress=missingIncludeSystem src tests/unit
	clang-format --dry-run --Werror src/*.c include/upf/*.h tests/unit/*.c

format:
	clang-format -i src/*.c include/upf/*.h tests/unit/*.c tests/fuzz/*.c

fuzz: $(LIB_SRC) tests/fuzz/fuzz_gtpu.c
	@mkdir -p $(BUILD)
	clang $(CPPFLAGS) $(CSTD) -g -O1 -fsanitize=fuzzer,address,undefined \
	  -o $(BUILD)/fuzz_gtpu src/gtpu.c tests/fuzz/fuzz_gtpu.c

clean:
	rm -rf $(BUILD) .pytest_cache tests/func/__pycache__

-include $(LIB_OBJ:.o=.d) $(APP_OBJ:.o=.d)

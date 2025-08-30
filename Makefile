SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c, obj/%.o, $(SRC))
DEP := $(OBJ:.o=.d)

CC := gcc
EXE := a.exe
CFLAGS := -Iinclude -O3 -flto=auto -DMAINLOOP_WINDOWS
DEBUG_FLAGS := -pg -no-pie
LIBS := -Llib -lSDL2 -lopengl32 -lshlwapi -lcomdlg32 -lole32

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(OBJ) $(CFLAGS) $(LIBS) -o $(EXE)

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -MMD -MP $(CFLAGS) $< -o $@

nes-mappers:
	gcc codegen/nes/mappers.c -o nes-mappers.exe
	./nes-mappers.exe
	rm nes-mappers.exe

clean:
	rm -rf obj $(EXE)

loc:
	find src -name \*.c | xargs wc -l

.PHONY: all clean loc

-include $(DEP)

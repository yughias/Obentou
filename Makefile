SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c, obj/%.o, $(SRC))
DEP := $(OBJ:.o=.d)

CC := gcc
EXE := a.exe
CFLAGS := -Iinclude -O3 -flto=auto
DEBUG_FLAGS := -pg -no-pie
LIBS := -Llib -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lshlwapi -lcomdlg32 -lole32

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(OBJ) $(CFLAGS) $(LIBS) -o $(EXE)

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -MMD -MP $(CFLAGS) $< -o $@

clean:
	rm -rf obj $(EXE)

loc:
	find src -name \*.c | xargs wc -l

.PHONY: all clean loc

-include $(DEP)

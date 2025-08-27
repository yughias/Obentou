rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
SRC := $(call rwildcard,src/,*.c)
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

-include $(DEP)
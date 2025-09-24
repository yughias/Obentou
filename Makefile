SRC := $(shell find src -name '*.c') \
       $(shell find ext -name '*.c')
OBJ := $(patsubst %.c, obj/%.o, $(SRC))
DEP := $(OBJ:.o=.d)


CC := gcc
EXE := a.exe
CFLAGS := -Iinclude -Iext/include -O3 -flto=auto
DEBUG_FLAGS := -pg -no-pie
LIBS := -Llib -lSDL3.dll -lopengl32 -ldwmapi -lshlwapi -lcomdlg32 -lole32

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(OBJ) $(CFLAGS) $(LIBS) -o $(EXE)

obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -MMD -MP $(CFLAGS) $< -o $@

nes-mappers:
	gcc codegen/nes/mappers.c -o nes-mappers.exe
	./nes-mappers.exe
	rm nes-mappers.exe

emcc:
	emcc -Iinclude -Iext/include $(SRC) -O3 -flto=full \
	-sUSE_SDL=3 \
	-sINVOKE_RUN=0 \
	-sEXPORTED_FUNCTIONS=[_main] \
	-o website/emulator.js

clean:
	rm -rf obj $(EXE)

loc:
	find src -name \*.c | xargs wc -l

.PHONY: all clean loc emcc

-include $(DEP)

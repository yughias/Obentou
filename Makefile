SRC := $(shell find src ext -name '*.c')
OBJ := $(patsubst %.c, obj/%.o, $(SRC))
DEP := $(OBJ:.o=.d)

CC := gcc
CFLAGS_COMMON := -Iinclude -Iext/include -O3
DEBUG_FLAGS := -pg -no-pie

ifeq ($(OS),Windows_NT)
EXE := obentou.exe
LIBS := -Llib -lSDL3 -lSDL3_net -lopengl32 -ldwmapi -lshlwapi -lcomdlg32 -lole32
PLATFORM_CFLAGS := -flto=8 -Wall -Wno-unused-function -Werror  -mwindows
RES_OBJ := app.res

else ifeq ($(shell uname -s),Darwin)
EXE := obentou
LIBS := $(shell pkg-config --static --libs sdl3 sdl3-net) -lm -liconv -lobjc -framework Cocoa
PLATFORM_CFLAGS := -flto=thin $(shell pkg-config --cflags sdl3 sdl3-net) -arch arm64
CFLAGS_COMMON := -Iinclude -Iext/include -O3
RES_OBJ :=

MAINLOOP_SRC := $(filter %/SDL_MAINLOOP.c, $(SRC))
MAINLOOP_OBJ := $(patsubst %.c, obj/%.o, $(MAINLOOP_SRC))

# Force Objective-C compilation for this file only
$(MAINLOOP_OBJ): CFLAGS += -x objective-c

else
EXE := obentou
LIBS := $(shell pkg-config --static --libs sdl3 sdl3-net) -lGL -lm
PLATFORM_CFLAGS := -flto=8 $(shell pkg-config --cflags sdl3 sdl3-net)
RES_OBJ :=
endif

CFLAGS := $(CFLAGS_COMMON) $(PLATFORM_CFLAGS)

all: $(EXE) config.ini

config.ini: base_config.ini
	cp base_config.ini config.ini

$(EXE): $(OBJ) $(RES_OBJ)
	$(CC) $(OBJ) $(RES_OBJ) $(CFLAGS) $(LIBS) -o $(EXE)

obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -MMD -MP $(CFLAGS) $< -o $@

app.res: config.rc logo.ico
	windres config.rc -O coff -o app.res

nes-mappers:
	gcc codegen/nes/mappers.c -o nes-mappers.exe
	./nes-mappers.exe
	rm nes-mappers.exe

emcc:
	emcc -Iinclude -Iext/include $(SRC) -O3 -flto=full \
	-sUSE_SDL=3 \
	-sINVOKE_RUN=0 \
	-sSTACK_SIZE=2MB \
	-sINITIAL_MEMORY=128MB -sALLOW_MEMORY_GROWTH=1 \
	-sASYNCIFY \
	-lidbfs.js \
	--preload-file base_config.ini@config.ini \
	-sEXPORTED_FUNCTIONS="['_main', '_obentou_exit', '_notify_widget_closed']" \
	-sEXPORTED_RUNTIME_METHODS="['FS', 'callMain']" \
	-o website/obentou.js
	cp logo.ico website/favicon.ico

clean:
	rm -rf obj obentou obentou.exe app.res config.ini

loc:
	find src -name \*.c | xargs wc -l

.PHONY: all clean loc emcc

-include $(DEP)
SRC := $(shell find src ext -name '*.c')
OBJ := $(patsubst %.c, obj/%.o, $(SRC))
DEP := $(OBJ:.o=.d)

CC := gcc
CFLAGS_COMMON := -Iconverted_assets -Iinclude -Iext/include -O3
DEBUG_FLAGS := -pg -no-pie

OBJ_LIB := $(filter-out obj/src/SDL_MAINLOOP.o, $(OBJ))
OBJ_MAINLOOP_TEST := obj/test_build/SDL_MAINLOOP.o

TEST_SRC      := $(shell find test/src -name 'test_*.c')
TEST_UTIL_SRC := $(filter-out $(TEST_SRC), $(shell find test/src test/ext -name '*.c'))
OBJ_TEST      := $(patsubst %.c, obj/%.o, $(TEST_SRC))
OBJ_TEST_UTIL := $(patsubst %.c, obj/%.o, $(TEST_UTIL_SRC))
DEP_TEST      := $(OBJ_TEST:.o=.d) $(OBJ_TEST_UTIL:.o=.d)

RAW_ASSETS        := $(shell find assets -type f 2>/dev/null)
GENERATED_HEADERS := $(patsubst assets/%, converted_assets/%.h, $(RAW_ASSETS))

ifeq ($(OS),Windows_NT)
    EXE_EXT         := .exe
    LIBS            := -Llib -lSDL3 -lopengl32 -ldwmapi -lshlwapi -lcomdlg32 -lole32
    PLATFORM_CFLAGS := -flto=$(shell nproc) -Wall -Wno-unused-function -Werror -mwindows
    TEST_LDFLAGS    := -mconsole
    RES_OBJ         := app.res
else
    EXE_EXT         :=
    TEST_LDFLAGS    := 

    ifeq ($(shell uname -s),Darwin)
        LIBS            := $(shell pkg-config --static --libs sdl3) -lm -liconv -lobjc -framework Cocoa
        PLATFORM_CFLAGS := -flto=thin $(shell pkg-config --cflags sdl3) -arch arm64
        RES_OBJ         :=

        MAINLOOP_SRC := $(filter %/SDL_MAINLOOP.c, $(SRC))
        MAINLOOP_OBJ := $(patsubst %.c, obj/%.o, $(MAINLOOP_SRC))
        $(MAINLOOP_OBJ): CFLAGS += -x objective-c
    else
        LIBS            := $(shell pkg-config --static --libs sdl3) -lGL -lm
        PLATFORM_CFLAGS := -flto=$(shell nproc) $(shell pkg-config --cflags sdl3)
        RES_OBJ         :=
    endif
endif

EXE      := obentou$(EXE_EXT)
TEST_EXE := $(patsubst test/src/test_%.c, ./test_%$(EXE_EXT), $(TEST_SRC))
CFLAGS   := $(CFLAGS_COMMON) $(PLATFORM_CFLAGS)

all: $(EXE) config.ini

test: $(TEST_EXE)

config.ini: base_config.ini
	cp base_config.ini config.ini

$(EXE): $(OBJ) $(RES_OBJ)
	$(CC) $(OBJ) $(RES_OBJ) $(CFLAGS) $(LIBS) -o $(EXE)

$(OBJ) $(OBJ_TEST) $(OBJ_TEST_UTIL) obj/test_build/SDL_MAINLOOP.o: $(GENERATED_HEADERS)

obj/test_build/SDL_MAINLOOP.o: src/SDL_MAINLOOP.c
	@mkdir -p $(dir $@)
	$(CC) -c -MMD -MP -DTEST $(CFLAGS) $< -o $@

obj/test/%.o: test/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -MMD -MP -DTEST -Itest/ext/include $(CFLAGS) $< -o $@

./test_%$(EXE_EXT): obj/test/src/test_%.o $(OBJ_TEST_UTIL) $(OBJ_LIB) $(OBJ_MAINLOOP_TEST)
	@mkdir -p $(dir $@)
	$(CC) -DTEST $(CFLAGS) $(TEST_LDFLAGS) $< $(OBJ_TEST_UTIL) $(OBJ_LIB) $(OBJ_MAINLOOP_TEST) $(LIBS) -o $@

obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -MMD -MP $(CFLAGS) $< -o $@

app.res: config.rc logo.ico
	windres config.rc -O coff -o app.res

converted_assets/%.h: assets/%
	@mkdir -p $(dir $@)
	xxd -i $< > $@
    
nes-mappers:
	gcc codegen/nes/mappers.c -o nes-mappers.exe
	./nes-mappers.exe
	rm nes-mappers.exe

emcc: $(GENERATED_HEADERS)
	emcc -Iconverted_assets -Iinclude -Iext/include $(SRC) -O3 -flto=full \
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
	rm -rf obj obentou obentou.exe app.res config.ini ./test_*$(EXE_EXT) converted_assets

loc:
	find src -name \*.c | xargs wc -l

.PHONY: all clean loc emcc test
.PRECIOUS: obj/test/%.o

-include $(DEP)
-include $(DEP_TEST)
-include obj/test_build/SDL_MAINLOOP.d
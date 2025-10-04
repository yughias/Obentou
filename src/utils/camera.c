#include "utils/camera.h"

#include "SDL3/SDL.h"

#include <stdio.h>
#include <stdlib.h>

static SDL_Camera* camera;
static int camera_w;
static int camera_h;
static int* camera_frame;

void camera_open(int w, int h){
    camera_frame = malloc(sizeof(int)*w*h);
    camera_w = w;
    camera_h = h;

    SDL_CameraSpec spec;
    SDL_zero(spec);
    spec.width = w;
    spec.height = h;
    spec.format = SDL_PIXELFORMAT_XRGB8888;
    SDL_CameraID* cameras = SDL_GetCameras(NULL);
    camera = SDL_OpenCamera(cameras[0], &spec);
}

void camera_close(){
    free(camera_frame);
    SDL_CloseCamera(camera);
    camera = NULL;
    camera_frame = NULL;
}

void camera_update(){
    if(!camera)
        return;
    SDL_Surface* s;
    // retrieve all frames
    while(s = SDL_AcquireCameraFrame(camera, NULL)){
        memcpy(camera_frame, s->pixels, sizeof(int)*camera_w*camera_h);
        SDL_ReleaseCameraFrame(camera, s);
    }
}

void camera_copy_frame(int* frame){
    if(!camera)
        return;
    memcpy(frame, camera_frame, sizeof(int)*camera_w*camera_h);
}
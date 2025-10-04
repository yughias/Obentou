#ifndef __UTILS_CAMERA_H__
#define __UTILS_CAMERA_H__

void camera_open(int w, int h);
void camera_close();
void camera_update();
void camera_copy_frame(int* frame);

#endif
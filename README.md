# SunShotCpp

- Install GTK4 via MSYS2
- Compile FFmpeg via MSYS2 / MinGW

Windows:
```
gcc main.cpp -o main `pkg-config --cflags --libs gtk4` -I C:/FFmpeg -L C:/FFmpeg/libavutil -lavutil -L C:/FFmpeg/libavformat -lavformat -L C:/FFmpeg/libavcodec -lavcodec -L C:/FFmpeg/libswscale -lswscale
```
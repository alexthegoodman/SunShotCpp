# SunShotCpp

- Install GTK4 via MSYS2
- Compile FFmpeg via MSYS2 / MinGW
`pacman -S mingw-w64-x86_64-x264` in MSYS2
`./configure --enable-shared --enable-libx264 --enable-gpl`

Windows:
```
gcc main.cpp -o main `pkg-config --cflags --libs gtk4` -I C:/FFmpeg -L C:/FFmpeg/libavutil -lavutil -L C:/FFmpeg/libavformat -lavformat -L C:/FFmpeg/libavcodec -lavcodec -L C:/FFmpeg/libswscale -lswscale
```
# 3D-Signed-Distance-Shapes-Demo
A ShaderToy's Demo by Inigo Quilez in a single main.c file
![3d-signed-distance-shapes](https://cloud.githubusercontent.com/assets/9608982/26532499/7c4a4c9a-4402-11e7-8010-2a8e9f3642b5.png)

### Dependencies
* glut (or freeglut)
* glew (Windows only)

### How to compile
* **Linux**: gcc -O2 main.c -o 3D_Signed_Distance_Shapes_Demo -lglut -lGL -lX11 -lm
* **Windows**: cl /O2 /MT /Tc main.c /D"GLEW_STATIC" /link /out:3D_Signed_Distance_Shapes_Demo.exe glut32.lib glew32s.lib opengl32.lib gdi32.lib Shell32.lib comdlg32.lib user32.lib kernel32.lib
* **Emscripten**: emcc -O2 -fno-rtti -fno-exceptions -o 3D_Signed_Distance_Shapes_Demo.html main.c --preload-file signed_distance_shapes.glsl -I"./" -s LEGACY_GL_EMULATION=0 --closure 1
* **Mac**: ???

### Useful links
[Inigo Quilez related page with this demo at the bottom](http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm)


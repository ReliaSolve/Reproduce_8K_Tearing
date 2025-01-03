# Reproduce_8K_Tearing

This is a simplified OpenGL program that demonstrates tearing on half of an 8K display when shown in
either windowed or full-screen mode on an nVidia card under Linux.  By default, it creates a window
that is 7864x4320 pixels in size.

The following arguments control its behavior:
- --fullScreenDisplay N : N is the index of the display to use for full-screen mode.  If not specified, full-screen mode is not used (N = -1).
- --width W : W is the width of the window in pixels.  If not specified, the default is 7864.
- --height H : H is the height of the window in pixels.  If not specified, the default is 4320.
- --fps F : F is the desired frame rate.  If not specified, the default is 60.

The program uses the GLFW library to create the window and OpenGL to render the scene.  On Windows,
it builds GLFW from source and relies on the user to specify the location of GLEW.
On Linux, it uses the system-installed GLFW and GLEW libraries.
The tearing only happens on Linux.

The tearing.mp4 video shows the tearing on the 8K display in a full-screen window.  Observe the bottom
portion of the right half of the display.  The tearing is especially visible at bottom of the
blue rectangle.

Other notes:
- An nVidia GeForce RTX 4090 card in a desktop system with AMD Ryzen Threadripper PRO 5955WX 16-Cores.
- Driver 550 on Linux (also an earlier one, probably 535).
- Does not happen when displaying 4K 240 Hz.
- Happens whether or not another display is plugged in.
- HDMI 2.1 single cable.
- Does not happen on Windows on the same computer using the same code base and cable.

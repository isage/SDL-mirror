# SD2-vita

This repo contains full source for vita SDL2 port (vita-* branches).

There are two render backends for Vita (and two makefiles):  
* scePiglet-based gles2 renderer
* sceGxm-based renderer

### scePiglet-based gles2 renderer

This one is incredibly slow in 2d, but you can use it if you'd like to use raw gles2 context, but with ability to bind SDL_Texture

### sceGxm-based renderer

This one is fast and should be used for sdl-based 2d. It can't bind SDL_Texture to gles2 context.  
You can still use raw gles2, though.


## TODO (e.g. not supported yet)

* Texture render-target for gxm-based render
* Sensors support
* Ability to set memory pool size via SDL Hint
* Unified build with both renders (gxm by default, piglet-gles2 via SDL Hint)



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


## Differences between this and vita2d version

* First of all, it's based on a 2.0.12 instead of 2.0.8-seomething.
* It fully supports everything that an SDL render should (while vita2d version doesn't support blending and RenderCopyEx)
* It also supports filesystem functions, while vita2d version doesn't
* It is and always be based on a fixed SDL release, not on some middle-point commit.

## Thanks
* @xerpi for initial (vita2d) port.
* vitasdk/dolcesdk devs
* CBPS discord
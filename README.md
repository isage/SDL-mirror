# Note: sdl2 vita support was upstreamed. Vitasdk contains latest package. Use that.
This repo kept for sdl1 support and historical purposes.

# SDL/SDL2-vita

This repo contains full source for vita SDL1 and SDL2 ports (vita-* branches).

## SDL1

This port supports both doublebuffered (with SDL_Flip) and non-doublebuffered drawing.  
It draws directly to framebuffer, without GXM (same as sdl1 works on other platforms)

## SDL2

There are two render backends for Vita:  
* sceGxm-based renderer (default)
* scePiglet-based gles2 renderer (must be chosen via hint or renderer index, DolceSDK only)

### sceGxm-based renderer

This is a default renderer.  
This one is fast and should be used for sdl-based 2d. It can't bind SDL_Texture to gles2 context.  
You can still use raw gles2, though.

### scePiglet-based gles2 renderer

This one is incredibly slow in 2d, but you can use it if you'd like to use raw gles2 context, with ability to bind SDL_Texture


## Where's code?

Code is in vita-[sdl-release-version] branches. Master branch is intentionally kept empty.

## Requirements

* [DolceSDK](https://github.com/DolceSDK/doc) (Or VitaSDK, but VitaSDK lacks Pigs-In-A-Blanket and thus gles2 support)
* (SDL2 only, optional) [Pigs-in-a-blanket](https://github.com/SonicMastr/Pigs-In-A-Blanket) (Included in DolceSDK)

## Building

### DolceSDK:

#### CMake:

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="${DOLCESDK}/share/dolce.toolchain.cmake" ..
make install

```

#### Makefile:
`make -f Makefile.vita.dolce install`

### VITASDK:

#### CMake:

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="${VITASDK}/share/vita.toolchain.cmake" ..
make install

```

#### Makefile:
`make -f Makefile.vita.vita install`


## Migrating from older SDL1-vita (vita2d one)
* Drop links to vita2d and sceGxm

## Migrating from older SDL2-vita (vita2d one)
* Link your app with pib instead of vita2d
* Link with SceMotion_stub
* Everything else should be the same. If you used vita2d_init_advanced - drop it.

## SDL1 Limitations

* No opengl support. Only 960x544 at 32bpp screen supported (sdl will do needed conversions anyway, though).

## SDL2 Limitations (in gxm renderer)

* Only SDL_TEXTUREFORMAT_ABGR8888 is supported.
* Memory pool (used for vertexes) has fixed size of 2 * 1024 * 1024. That shouldn't be an issue, unless your draw count if insanely big.
* You can bind SDL_Texture to gles2 context only when using gles2 renderer.
* SDL_RenderReadPixels supports reading only from display rendertarget (no one sane should read from texture rendertarget anyway, you already have texture, god dammit).

## SDL1 Differences between this and vita2d version

* vita2d doesn't support (and can't support) non-doublebuffered drawing that many sdl1-based software uses. This port does.

## SDL2 Differences between this and vita2d version

* First of all, it's based on a 2.0.12 instead of 2.0.8-something.
* It fully supports everything that an SDL render should (while vita2d version doesn't support blending modes, RenderCopyEx and texture render target)
* It also supports filesystem functions, while vita2d version doesn't
* It is and always will be based on a fixed SDL release, not on some middle-point commit.
* Sensors (gyro / accelerometer) support.

## Thanks
* xerpi and rsn8887 for initial (vita2d) port
* vitasdk/dolcesdk devs
* CBPS discord (Namely Graphene and SonicMastr)
* Northfear for inspiration to do sdl1 port.

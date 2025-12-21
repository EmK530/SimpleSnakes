# Simple Snakes
A SDL2 snake game in C with singleplayer and local/online multiplayer.

### Notice
This is very rushed and misses freeing some dynamically allocated memory, ran out of time to do a sweep of things that were never freed.

<img width="300" height="300" alt="snakes-image" src="https://github.com/user-attachments/assets/d725cd86-be3c-4a16-83e9-d538d9552739" />

# How to run
## Windows
Your system needs to be 64-bit, other than that the program is bundled with all the DLLs necessary to run the program.

## Linux
**If you get permission denied trying to run the game, make sure you have execute permission:**
```
chmod +x snake
```

**If you are getting errors about libSDL2 files not being found, make sure you have these libSDL2 libraries installed:**
```
sudo apt install libsdl2-dev
sudo apt install libsdl2-ttf-dev
sudo apt install libsdl2-mixer-dev
```

# How to compile
## Windows
The makefile uses `mingw-w64` to compile, make sure you have it installed.
Running `make windows` will set up a ready-to-run build in `build/out/win64`.

## Linux
Compiling to Linux uses `make` or `make run`, but does not set up a proper build.
The executable is dumped in the root repository folder and all you need to bundle with it is the repository's `assets` folder (see releases for reference).

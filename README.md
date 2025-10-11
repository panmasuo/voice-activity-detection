# VAD algorithm implemented using ALSA library in C programming language

Voice Activity Detection (VAD) algorithm made based on Moattar and Homayounpour's publication
[A simple but efficient real-time voice activity detection algorithm](https://www.researchgate.net/publication/255667085_A_simple_but_efficient_real-time_voice_activity_detection_algorithm).

Code is written in C language using:

- `ALSA` (for sound proccesing)
- `pthreads` (for multithreading)

## Building

> **NOTE**
>
> Built and tested with `gcc-13` using `c23` standard.

Make sure to install all needed dependencies:

```bash
sudo apt install cmake ninja libasound2-dev
```

Build using `CMakePreset` and `CMakeTools` extension or by hand:

```bash
mkdir build && cd build
cmake ..
cd ..
cmake --build build/
```

and run the executable from the `CMakePreset` build directory:

```bash
./build/release/vad
```

or your own:

```bash
./build/vad
```

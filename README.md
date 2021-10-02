# VAD algorithm implemented using ALSA library in C programming language

Voice Activity Detection (VAD) algorithm made based on Moattar and Homayounpour's publication [A simple but efficient real-time voice activity detection algorithm](https://www.researchgate.net/publication/255667085_A_simple_but_efficient_real-time_voice_activity_detection_algorithm)

All of the code is written in C language. Using:
1. ALSA (for sound proccesing)
2. Pthreads (for multithreading)
4. WiringPI (for led blinking on Raspberry Pi) <optional>
  
## Building
Make sure to install all needed dependencies:
```
sudo apt-get install libasound2-dev 
```
```
sudo apt-get install wiringpi
```

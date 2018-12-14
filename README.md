# Echo Chamber /// OpenGL Music Visualizer

## Getting it to Run
- This app only runs on Windows computers
- IMPORTANT: Go to the sound window in the control panel, click on the "Recording" tab, and enable the Stereo Mix (without this, the app can't read computer audio)

## VR
- VR functionality is currently a WIP. If you wish to enable it, uncomment the #define VR_ENABLED

## Audio Sensitivity
- You will need to modify the AUDIO_SENSITIVITY #define based on how loud your music is (currently working on fixing this issue).
- Incorrect audio sensitivity may result in visuals that are too fast or too slow relative to the music
- Generally, if found that a value of 2 is good for playback on headphones, and a value of 0.1 is good for playback on speakers
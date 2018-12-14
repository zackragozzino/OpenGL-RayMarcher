# Echo Chamber /// OpenGL Music Visualizer

## Getting it to Run
- In for the app to read audio data, you need to run it on a Windows computer
- IMPORTANT: Go to the sound window in the control panel, click on the "Recording" tab, and enable the Stereo Mix

## VR
- VR functionality is currently a WIP. If you wish to enable it, uncomment the #define VR_ENABLED

## Audio Sensitivity
- You will need to modify the AUDIO_SENSITIVITY #define based on how loud your music is (currently working on fixing this issue).
- Generally, if found that a value of 2 is good for playback on headphones, and a value of 0.1 is good for playback on speakers
- If you don't do this, you may find that the visuals are too fast or too slow relative to the music
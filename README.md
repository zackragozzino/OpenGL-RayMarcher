# Echo Chamber /// OpenGL Music Visualizer

<a href="http://www.youtube.com/watch?feature=player_embedded&v=wiJvpwkGFrM
" target="_blank"><img src="http://img.youtube.com/vi/wiJvpwkGFrM/0.jpg" 
alt="IMAGE ALT TEXT HERE" width="240" height="180" border="10" /></a>

![alt text](https://github.com/zackragozzino/OpenGL-RayMarcher/blob/master/Output_Images/Screencap.png)

## Getting it to Run
- This app only runs on Windows computers
- IMPORTANT: Go to the sound window in the control panel, click on the "Recording" tab, and enable the Stereo Mix (without this, the app can't read computer audio)

## VR
- VR functionality is currently a WIP. If you wish to enable it, uncomment the #define VR_ENABLED

## Audio Sensitivity
- You will need to modify the AUDIO_SENSITIVITY #define based on how loud your music is (currently working on fixing this issue).
- Incorrect audio sensitivity may result in visuals that are too fast or too slow relative to the music
- Generally, if found that a value of 2 is good for playback on headphones, and a value of 0.1 is good for playback on speakers
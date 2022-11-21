# Unreal-Fast-Dynamic-Texture
Runtime writing to huge (4K+) textures at realtime framerates achived with render region chunks.

Based on [Parallelcube's article](https://www.parallelcube.com/2018/01/08/dynamic-texture-introduction-drawing-canvas/)
Inspired by: [iUltimateLP's gist](https://gist.github.com/iUltimateLP/baca7aee4b28585b5fd2d0d46b541d95), give them a read.

The demo project lets you draw to a 8K texture at realtime.

`Click` with the mouse to draw, Press `C` to clear the canvas, press `Space` to toggle chunked rendering on/off

## Practical Example (not included):

Writing the whole map's groundwater levels to a texture to make it "wet"
![image](https://user-images.githubusercontent.com/1968543/203129788-bfe109f3-a82d-4240-ade0-987a451feb14.png)


## How it works:
1. Create a desired size Texture2D as a Drawing Canvas
2. Divide the texture's area into desired number of chunks (render regions) for faster rendering
3. Put the chunks into a 2D array where we can look up every pixel position's corresponding chunk via `array[pixelX][pixelY]`
4. Write to our texture's pixel data and mark it's chunk dirty
5. Update collects all the dirty chunks since last frame and renders them to a Texture2D we can use in materials&stuff
6. PROFIT!!

`Performance:
GTX1080Ti,
Ryzen 9 5900X,
64GB 3200Mhz DDR4`
![image](https://user-images.githubusercontent.com/1968543/203107033-1e92d2c8-731e-44a9-84f6-d82191f4f94b.png)


`Performance:
Steam Deck`
![image](https://user-images.githubusercontent.com/1968543/203115108-97ff6cb0-420d-4c94-b004-78c01b522e2d.png)

## Possible improvements:

Currently the brush's radius is not taken into account when checking for dirty chunks.
I'll only ever write single pixels to the texture and a pixel is either in one chunk or another so it's good enough for me but if you want to make a drawing game or something here's some hints :)

![image](https://user-images.githubusercontent.com/1968543/203119233-cca73d70-a95b-4012-a64d-574a79a42a5b.png)

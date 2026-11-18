# Image-Processing

This project demonstrates Parallel Image processing in C++.

It accepts the path to an input image and performs one of the 4 user-selected operations - 
* Edge detection
* Reflection
* Rotation
* Brightening

The processed output is stored in imageproccessing_Output.pgm

This application uses **PGM (Portable GrayMap)** format for both input and output. This avoids the processing overhead of image compression algorithms, keeping the focus strictly on parallel execution performance.

The program also outputs the total time taken for reading and processing the image in the terminal.
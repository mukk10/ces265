# ces265: Light-weight, multi-threaded HEVC video encoder

A C++ HEVC (also sometimes called H.265) video encoder that I wrote during
my Ph.D. The original sources could be found at
[sourceforge link](https://sourceforge.net/projects/ces265). 

Using `pthreads` library, this encoder can be used to process raw YCbCr
4:2:0 planar videos.

The official page of ces265 is [here](http://ces.itec.kit.edu/ces265.php)

The ces265 is written in C++ and developed at the 
[Chair for Embedded Systems (CES)](http://ces.itec.kit.edu),
Karlsruhe Institute of Technology (KIT), Germany. It's purpose was to
facilitates the embedded systems, architecture, and video coding
communities to perform:
- research, development, and testing of various video related concepts
at both hardware and software levels.
- fast simulation and evaluation.
- accurate complexity analysis.

## Citation

In case of usage, please refer to our corresponding DATE 2014 publication:
M. U. K. Khan, M. Shafique, J. Henkel, "Software Architecture of High
Efficiency Video Coding for Many-Core Systems with Power-Efficient
Workload Balancing",
In IEEE/ACM Design Automation and Test in Europe (DATE), 2014.

## Features

Some prominent features of this alpha release are:
- A highly efficient implementation with light-weight data structures
that provides a significantly high throughput compared to the HEVC
reference software (a ces265 thread is about 13 times faster than that
of the HM-9.2 reference software).
- Parallelized using HEVC's novel Tile concept and `pthread` multi-threading
API.
- User-configurable Video Tile structure.
- Comprehensive statistics generation at different granularities,
i.e. at GOP-, frame-, Tile-, and CTU- level.
- Standard-compliant Intra-Only Encoder (the inter-path is under
development and coming soon).
- Windows and Linux compatible.
Note: the feature list will be updated according to the regular
releases of ces265.

## Acknowledgment

This work was partly supported by the German Research Foundation (DFG)
as part of the Transregional Collaborative Research Centre
[Invasive Computing (SFB/TR 89)](http://invasic.de).


## Software Guide

User can provide input arguments via command line to this program.
The command line arguments are given below with a brief detail.
Note that the argument which is preceded by + is optional. The
order in which the arguments appear is not important. For more
information, see `EncTop.cpp` file.


| Argument   | Description   |
|---|---|
| -i YUV420PFileName | The "-i" option is used to provide the yuv420 planar format file name to the program, which will be compressed by the program to generate the compressed bitstream as a file named "Video.h265" |
| -w FrameWidth | The "-w" option specifies width of a frame in pixels |
| -h FrameHeight | The "-h" option specifies height of a frame in pixels | 
| (+)-gop GopSize | The "-gop" option specifies the length of the GOP. The starting frame of a GOP is an Intra frame and all the rest are P-frames. Note that for the current implementation, GopSize must be equal to 1 as only Intra frame compression is supported. The default value of GopSize is equal to 1 |
| -Nframes NumFrames | The "-Nframes" option specifies the total number of frames to compress |
| (+)-fps FramesPerSec | The "-fps" option is used to specify the frame-rate. Note that this is only used while computing the RD-parameter and has no impact on compression or timing efficiency. The default value of FramesPerSec is 1 |
| (+)-QP QPValue | The "-QP" options specifies the QP of all the frames. The default value of QPValue is 32 |
| (+)-Ngopth NumGopThreads | The "-Ngopth" option specifies the total number of GOP threads used. For the current implementation, NumGopThreads must be equal to 1 |
| (+)-Nsliceth NumSliceThreads | The "-Nsliceth" option specifies the total number of slice threads used. For the current implementation, NumSliceThreads must be equal to 1 |
| (+)-Ntiles NumTilesPerFrame FrameWidthInTiles FrameHeightInTiles | The "-Ntiles" option specifies the total number of tiles that will reside in one full frame. Moreover, it also specifies the tile arrangement where FrameWidthInTiles argument gives the total tiles encompassing the width of the frame and FrameHeightInTiles argument does the same for the height of the frame. For example, "-Ntiles 20 5 4" will generate 20 tiles, 5 tile columns and 4 tile rows. For ces265, the sizes of the tiles are equal. Default value of NumTilesPerFrame is equal to 1 |
| (+)-Ntileth NumTileThreads | The "-Ntileth" option specifies the total number of tile threads that will be used. The default value of NumTileThreads is 1 |
| (+)--ver | The "--ver" option denotes verbosity and providing this argument to the program will produce verbose output. By default, verbosity is turned off |
| (+)--rec | The "--rec" option denotes reconstructed output generation. The name of the reconstructed yuv420 planar file is YUV420PFileName_HEVCRecon (see "-i" option). By default, no reconstructed output is generated |
| (+)--stat | The "--stat" option denotes writing output statistics in a "Statistics.txt" file. By default, no output statistics are written |


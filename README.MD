Herder
============
This project was started to teach me the C programming language, with an emphasis on multi threading, networking, advanced usage of macros, low level memory management and how to publish and maintain packages for different Linux distribution.

Herder is a media organization tool which allows you to have an ingest station that digitalises your owned media, store the digitalised files locally or on a network location and keep track of all your data to allow easy playback with a media player of your choice.

## Usage/Installation:

##### There is currently no automated way available to install this project, but a manual installation is possible (See below for how to).

### Installation:
##### Prerequisite:
* GCC (GNU Compiler Collection).
* time command/package (optional, gives more detailed information during compilation when available).
* Valgrind (optional, when available a memory leak check will be done when building a 'test' build).

1. Clone the repository.
2. (Optional, but advised) Compile and run a test build with **"./build test debug"**.
3. Build the executables with your desired level of optimisation **"./build all Optimisation:[0-3]"**.
4. Run the installation script. [Currently not available, this would move the executables and install bash-completion scripts.]


#### Usage:
This section is not yet complete, run **"./herder --help"** and **"./herderServerDaemon --help"** to get some usage tips.
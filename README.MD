Herder
============
A media organization tool.

Herder makes it possible to split the process of ingestion, storage and archiving of digitalised media files. 

E.g. have an ingestion station on your local network to digitalise your media, a server to store media information and keep track of your favourite shows and a Network-attached storage (NAS) to store the large media files on.
#
This project was started as an exercice to help me understand the ins and outs of the C programming language, with an emphasis on multi threading, networking, advanced usage of macros, low level memory management. With the side goal of learning how to publish and maintain packages for different Linux distribution and try out different braning modelsin git e.g. GitFlow and OneFlow options #1-3.

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
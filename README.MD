Herder
============
A media organization tool. Designed to easily store and archive digitalised copies of your favourite shows.

This project was started as an exercice to study the ins and outs of the C programming language, with an emphasis on multi threading, networking, advanced usage of macros and low level memory management. With the side goal of learning how to publish and maintain packages for different Linux distribution and try out different git branching models.

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
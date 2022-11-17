
# Herder v0.1.0-indev

A feature rich media organization tool. Designed to store and archive digitised copies of your favourite medias.

> **"Excuse me. I'm actually a... herder."<br/>
"It's pronounced hoarder young man..."**

*South Park, s14e10 Insheeption.*

## Motivation:
This project was started as a study exercise of the ins and outs of the C programming language, with an emphasis on multithreading, scalable I/O event notification via *'epoll'*, secure network comunication via *'TLS'*, advanced usage of preprocessor macros and low level memory management. With the side goal of trying out different git branching models.

## Current branching model:
This project currently follows Adam Rukas [OneFlow](https://www.endoflineblog.com/oneflow-a-git-branching-model-and-workflow) branching model with the variation *'develop + master'* with Option \#3 *'rebase + merge –no–ff'*.

## Installation:
### Prerequisite:
* GCC (GNU Compiler Collection).
* [LibreSSL](https://github.com/PowerShell/LibreSSL#building-libressl) (Make sure that lib 'sll' and 'crypto' are accessible by the compiler/linker. Easily done via symlinks to the dlls created by the 'LibreSSL' install script.)

### Optional:
* 'time' command/package (prints detailed timing information during compilation).
* Valgrind ( when available a memory leak check will be done when building a 'test' build).
* GNU Debugger gdb.
* bash-completion package.
* journalctl
* Adding an alias for the build script and then registering the script for bash-completion.
```sh
alias build="./scripts/build"

complete -C build build
```

## Building & running tests:
From the project root run the build script **'./scripts/build'** via the **'build'** alias to see all available build options.
* **'build test'** runs all tests defined in **'src/test.c'**
* **'build debug'** creates a debuggable executable in the **'./out'** directory. The executable name can be changed in the **'build'** script via the **'debugTargets'**.

When running a **debug** or **test** build debug information will be logged using **syslog**, to view these messages, use **journalctl** or any other method supported by your distribution.

## Intended usage:
Herder is meant to be run as five (5) separate components, an ingestation station from which media gets importet, a network attached storag server that handles media storage, a server that runs a web-service for clients to interact with, a Database server which stores all media metainformation and diffent clients be it terminal, gui or web based.

### Examples:
This section is not yet complete.

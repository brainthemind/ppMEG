# ppMEG
Parallel port manager for MEG stimulation using MATLAB. This script provides an interface for interacting with three parallel ports on a Linux PC to send triggers and read button responses.

Author: Raphael Bordas (<raphael.bordas@universite-paris-saclay.fr>)

## Disclaimer
This code is adapted from https://github.com/benoitberanger/parport_cenir. 
Original author's comments on the delay:
"On my computer, "ppmex('w',msg)" takes between 10µs and 40µs to be executed => checked at the oscilloscope.
So yes, there is a jitter...".

Main changes:
  - add read feature on the STATUS pins
  - add multiple parallel ports support
  - minor refactors

**However, this new version has not been tested with an oscilloscope.**

## Installation

> A pre-compiled version is available on this repository: simply ensure the `ppMEG.mexa64` file is in the desired working directory. The `ppMEG` function is directly available in MATLAB as described below.

In case you changed something in the `.c` file:

1. Compile the `.c` file in the MATLAB/Octave terminal:
```bash
mex -O -v ppMEG.c
```
2. Ensure the `ppMEG.mexa64` is in the desired working directory. The `ppMEG` function is directly available in MATLAB.

Compilation is only required once (after each update of the `.c` file).

## Usage

All commands can be abbreviated with the first letter.

### Using a single port
This mode is convenient if you only need to send triggers. Do not use it to interact with the button responses.

```matlab
ppMEG('open', '/dev/parport1')    % open and claim access to one port for writing/reading
value = ppMEG('read')             % read current value on the STATUS pins of '/dev/parport1'
ppMEG('write', 200)               % write 200 on the DATA pins of '/dev/parport1')
ppMEG('close')                    % release and close the port
```

### Using multiple ports
This is required if you need the responses from the buttons, which send the triggers on 3 parallel ports.
```matlab
ppMEG('o')                                % open all ports
[val_pp1 val_pp2 val_pp3] = ppmanager('r')    % read all the ports previously opened
ppMEG('w', 200)                           % write on the writing port (default = '/dev/paport1')
ppMEG('c')                                % close all ports
```

### Resetting the writing port
When `ppMEG` writes on the trigger port, it does not automatically reset. Thus, you need to send a 0 trigger manually:
```matlab
ppMEG('w', 200)                           % write on the writing port (default = '/dev/paport1')
WaitSecs(0.008)                           % wait a few milliseconds
ppMEG('w', 0)                             % reset the writing port
```

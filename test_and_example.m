%% Testing the triggers in Matlab
% This script requires PTB to be installed

clear all % to be sure to clean all MEX files
clc

%% testing multiple ports

% open and prepare the port
ppMEG('open');
ppMEG('write', 0);

% Send a pusle : all LEDs should flash
ppMEG('write', 255);
WaitSecs(0.008);
ppMEG('write', 0);

% Check the read values
% default values can be different from 0
[pp1 pp2 pp3] = ppMEG('read')

ppMEG('close');

%% testing single ports

% open and prepare the port
ppMEG('open', '/dev/parport1');
ppMEG('write', 0);

% Send a pusle : all LEDs should flash
ppMEG('write', 255);
WaitSecs(0.008);
ppMEG('write', 0);

ppMEG('c');

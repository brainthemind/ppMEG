/** Code to communicate (read/write) with multiple parallel ports in MATLAB in Linux
 *
 * Author: Raphael Bordas, raphael.bordas@universite-paris-saclay.fr
 *
 * To compile in MATLAB/Octave terminal: "mex -O -v ppMEG.c"
 * Once the ppMEG.mexa64 file is in the working directory, the ppMEG function is available in Matlab 
 *
 * Disclaimer
 * ==========
 * Adaptaed from https://github.com/benoitberanger/parport_cenir. 
 * Comments of the original author on the delay:
 * "On my computer, "ppmex('w',msg)" takes between 10µs and 40µs to be executed => checked at the oscilloscope.
 * So yes, there is a jitter..."
 * 
 * However, this new version was not tested with an oscilloscope.
 *
 * Examples (in Matlab/Octave)
 * ===========================
 * 
 * a) Using one port
 * >> ppMEG('open', '/dev/parport1')    % open and claim access to one port for writing/reading
 * >> value = ppMEG('read')             % read current value on the STATUS pins of '/dev/parport1'
 * >> ppMEG('write', 200)               % write 200 on the DATA pins of '/dev/parport1')
 * >> ppMEG('close')                    % release and close the port
 * 
 * b) Using multiple ports (all commands can be abbreviated with the first letter)
 * >> ppMEG('o')                                % open all ports
 * >> [val_pp1 val_pp2 val_pp3] = ppMEG('r')    % read all the ports previously opened
 * >> ppMEG('w', 200)                           % write on the writing port (default = '/dev/paport1')
 * >> ppMEG('c')                                % close all ports
 * */
#include <sys/io.h>
#include <unistd.h> /* For open() */
#include <fcntl.h>  /* For O_RDWR */
#include <errno.h>
#include <string.h>
#include <linux/ppdev.h>
#include <linux/parport.h>
#include <sys/ioctl.h> /* For PPWDATA and PPRSTATUS */
#include "mex.h"
#include "matrix.h"

// global variables to keep track of the ports
// by default, the code reads all parallel ports but write on only one (writing_port_idx)
static int pports[] = {0, 0, 0};
static char addresses[3][15] = {"/dev/parport0", "/dev/parport1", "/dev/parport4"};
static int use_multiple_ports = 1;
static int writing_port_idx = 1;

void PrintHelp()
{
    mexPrintf("parallelport usage : \n");
    mexPrintf("parallelport('open', port_address)  : opens the device at the specified address \n");
    mexPrintf("parallelport('write',message)       : sends the message = {0, 1, 2, ..., 255} uint8 \n");
    mexPrintf("parallelport('read')                : reads the value currently set in the port \n");
    mexPrintf("parallelport('close')               : closes the device \n");
    mexPrintf("\n");
}

/**
 * Returns the file descriptor of the port
 *
 * Open the device/file, then claim access, so we are ready to send messages
 * Takes the port address as an argument
 * */
int openPort(char *pp_address)
{
    int pport;
    pport = open(pp_address, O_RDWR); /* Open the device/file */

    /* File opened ? */
    if (pport < 0)
        mexErrMsgTxt("Couldn't open parallel port (user have permission on the device ? user in the good group ?) \n");

    /* Claim access */
    if (ioctl(pport, PPCLAIM) < 0)
    {
        mexPrintf("PPCLAIM ioctl Error : %s (%d)\n", strerror(errno), errno);
        mexErrMsgTxt("PPCLAIM ioctl Error");
    }

    mexPrintf("Parallel %s opened successfully \n", pp_address);

    return pport;
}

/*************************************************************************/
/* Send message : an int between 0 and 255 (i.e. a char in C)
/*************************************************************************/
void writePort(const unsigned char *message, int pport)
{
    if (pport > 0)
    {
        if (ioctl(pport, PPWDATA, message) < 0)
        {
            mexPrintf("PPWDATA ioctl Error : %s (%d)\n", strerror(errno), errno);
            mexErrMsgTxt("PPWDATA ioctl Error \n");
        }
    }
    else
        mexErrMsgTxt("Parallel port was not opened \n");
}

/**
 * Read message : an int between 0 and 255 (i.e. a char in C)
 * Use the STATUS pins
 *
 * No use returned value, use pointers for consistency with the writePort function 
 * */
void readPort(unsigned char *data, int pport)
{
    if (pport > 0)
    {
        // ioctl is using pointers: no returned value, argument is passed by ref.
        if (ioctl(pport, PPRSTATUS, data) < 0)
        {
            mexPrintf("PPRSTATUS ioctl Error : %s (%d)\n", strerror(errno), errno);
            mexErrMsgTxt("PPRSTATUS ioctl Error \n");
        }
    }
    else
        mexErrMsgTxt("Parallel port was not opened \n");
}

/**
 * Use the port descriptor to clean it up (on exit or to avoid repeted openings)
 *
 * Always return 0 (or raise an error if the port was not successfully closed).
 * Do nothing if the port was not opened. 
 * */
int unloadPort(int pport)
{
    if (pport > 0)
    {
        if (ioctl(pport, PPRELEASE) < 0) /* Release access */
        {
            mexPrintf("PPRELEASE ioctl Error (with pport = %d): %s (%d)\n", pport, strerror(errno), errno);
            mexErrMsgTxt("PPRELEASE ioctl Error\n");
        }

        if (close(pport) < 0) /* Close file */
        {
            mexPrintf("Close Error (with pport = %d): %s (%d)\n", pport, strerror(errno), errno);
            mexErrMsgTxt("Close Error\n");
        }

        pport = 0;
        mexPrintf("Parallel port has been closed \n");
    }

    return pport;
}

void unloadAll(void)
{
    for (int i = 0; i < 3; i++)
        pports[i] = unloadPort(pports[i]);
}

/**
 * Entry point (equivalent to the main() function in regular C)
 *
 * The left hand side parameters (lhs) are the output of the function directly in Matlab
 * The right hand side parameters (rhs) are the inputs arguments from the Matlab call
 * */
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    char *action;
    unsigned char message = 0;
    char *user_address; // used only if user gives an address to the open mex function

    // if no input argument, display help
    if (nrhs == 0)
    {
        PrintHelp();
        return;
    }

    // Determine what the user is requesting
    // Only the first letter is used to allow abbreviation
    action = mxArrayToString(prhs[0]);

    switch (action[0])
    {
    case 'o': // ppMEG('open'[, 'port_address'])
        if (nrhs == 1)
        {
            // no port address specified : closing then reopening all ports
            for (int i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++)
            {
                pports[i] = unloadPort(pports[i]);
                pports[i] = openPort(addresses[i]);
            }
        }
        else if (nrhs == 2)
        {
            // the user specifies an address that overwrites the default declared in static
            use_multiple_ports = 0;
            user_address = mxArrayToString(prhs[1]);
            writing_port_idx = 0;
            pports[0] = unloadPort(pports[0]);
            pports[0] = openPort(user_address);
        }
        else
        {
            mexErrMsgTxt("Incorrect number of arguments.");
        }
        break;

    case 'w': // ppMEG('write', message)
        if (nrhs != 2)
            mexErrMsgTxt("You need to specify the message to send [0-255]");

        message = (unsigned char)mxGetScalar(prhs[1]); // Fetch the input value
        writePort(&message, pports[writing_port_idx]);

        break;

    case 'r': // ppMEG('read')
        if (nrhs != 1)
            mexErrMsgTxt("Error calling read: no argument should be given");

        // assign output message to the array of left-side arguments
        // for sake of simplicity, char is cast to double value
        if (use_multiple_ports)
        {
            for (int i = 0; i < 3; i++)
            {
                readPort(&message, pports[i]);
                // fill-in the left-hand side array with each value read
                plhs[i] = mxCreateDoubleScalar(message);
            }
        }
        else
        {
            readPort(&message, pports[0]);
            plhs[0] = mxCreateDoubleScalar(message);
        }

        break;

    case 'c': // ppMEG('close')
        if (nrhs != 1)
            mexErrMsgTxt("Error calling close: no argument should be given");
        unloadAll();
        // resetting default values of global variables in case ppMEG is called again in the same
        // MATLAB workspace
        use_multiple_ports = 1;
        writing_port_idx = 1;
        break;

    default:
        mexErrMsgTxt("No valid action specified : o / w / r / c");
    }

    /* Make sure device is released when MEX-file is cleared */
    mexAtExit(unloadAll);
}

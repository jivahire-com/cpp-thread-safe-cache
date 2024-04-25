#include <stdint.h>

/*
 * The SCP boot loader is expected to perform the following actions:
 * 1. Hold the compressed SCP firmware in mainimage section of boot binary
 * 2. Uncompress the SCP firmware into SCP ITCM and DTCM based on the sections
 * 3. Jump to ITCM and execute the SCP firmware.
 */

/* TODO: Create a placeholder variable for the mainimage section - the MCP firmware will be copied into this section
 * ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484340
*/

/* TODO: ADO: Invoke APIs to uncompress the firmware in the mainimage section
*  ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484340
*/

/* TODO: ADO: Update program counter to point to ITCM and start executing the MCP firmware
*  ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484340
*/

int main(void)
{
    return 0;
}
/* Handle for the trigger group of events. */
AMXID amxidTrigger;

/* Constants for use in the group. */

#define TRIGGER_MASK  0x0001
#define TRIGGER_SET   0x0001
#define TRIGGER_RESET 0x0000
#define KEY_MASK      0x0002
#define KEY_SET       0x0002
#define KEY_RESET     0x0000

void main (void)
{
    .
    .
    .
    /* Create an event group with
       the trigger and keyboard events reset */
    ajevcre (&amxidTrigger, 0, "EVTR");
    .
    .
    .
}

void interrupt vTriggerISR (void)
{
    /* The user pulled the trigger. Set the event. */
    ajevsig (amxidTrigger, TRIGGER_MASK, TRIGGER_SET);
}

void interrupt vKeyISR (void)
{
    /* The user pressed a key. Set the event. */
    ajevsig (amxidTrigger, KEY_MASK, KEY_SET);

    !! Figure out which key the user pressed and store that value
}

void vScanTask (void)
{
    .
    .
    .
    while (TRUE)
    {
        /* Wait for the user to pull the trigger. */
        ajevwat (amxidTrigger, TRIGGER_MASK, TRIGGER_SET,
           WAIT_FOR_ANY, WAIT_FOREVER);

        /* Reset the trigger event. */
        ajevsig (amxidTrigger, TRIGGER_MASK, TRIGGER_RESET);

        !! Turn on the scanner hardware and look for a scan.
        .
        .
        .
        !! When the scan has been found, turn off the scanner.
    }
}

void vRadioTask (void)
{
    .
    .
    .
    while (TRUE)
    {
        /* Wait for the user to pull the trigger or press a key. */
        ajevwat (amxidTrigger, TRIGGER_MASK | KEY_MASK,
           TRIGGER_SET | KEY_SET, WAIT_FOR_ANY,
           WAIT_FOREVER);

        /* Reset the key event.  (The trigger event will be reset
           by the ScanTask.) */
        ajevsig (amxidTrigger, KEY_MASK, KEY_RESET);

        !! Turn on the radio.
        .
        .
        .
        !! When data has been sent, turn off the radio.
    }
}



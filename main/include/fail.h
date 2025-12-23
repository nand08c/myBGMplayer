#ifndef FAIL_H
#define FAIL_H

/**
 * @brief Enters a fatal error state.
 * 
 * This function is intended to be called when the system encounters an unrecoverable error.
 * It will:
 * 1. Log the error message.
 * 2. Configure the error LED GPIO.
 * 3. Enter an infinite loop blinking the LED.
 * 4. Manually feed the watchdog timer to prevent a system reset, keeping the device in this trapped state.
 * 
 * @param msg A descriptive error message to log before entering the trap.
 */
void system_fatal_error(const char *msg);

#endif // FAIL_H

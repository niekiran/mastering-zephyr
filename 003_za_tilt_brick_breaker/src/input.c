/*
 * input.c — Device-agnostic input interface
 *
 * Delegates to the active backend selected via Kconfig.
 * Contains no hardware-specific code.
 */

#include "input.h"
#include "input_backend.h"

int input_init(void)
{
	return input_backend_init();
}

int input_read(void)
{
	return input_backend_read();
}

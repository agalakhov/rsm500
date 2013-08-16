#pragma once

/*
 * LED codes:
 *  ---.   = stack smash detected
 *  --.    = ARM Invalid hardware operation
 *  --..   = ARM memory manager error (out of memory)
 *  --...  = ARM Bus error (null-pointer assignment)
 *  --.... = ARM Invalid argument (software error)
 *  -      = user panic(0)
 *  -.     = user panic(1)
 *  -..    = user panic(2)
 *  -...   = user panic(3)
 *  -....  = user panic(4)
 *  -..... = user panic(5) etc.
 */

void panic(unsigned num_blinks);

#pragma once

/*
* Using the Mersenne Twister Random number generator
* http://www.qbrundage.com/michaelb/pubs/essays/random_number_generation
* This code is licensed as "Public Domain" (mt_init(), mt_random())
*/

void mt_init(void);
unsigned long mt_random(void);

/*
 * ky040.h
 *
 *  Created on: Mar 14, 2026
 *      Author: Lenovo
 */

#ifndef INC_KY040_H_
#define INC_KY040_H_

#define KY040_BRIGHTNESS_LIMIT_PCT   20U

extern int count;
extern int light_on;

void KY040_Init();
void KY040_Update();
void KY040_Key();

#endif /* INC_KY040_H_ */

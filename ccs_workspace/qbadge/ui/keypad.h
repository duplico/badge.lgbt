/*
 * keypad.h
 *
 *  Created on: Jun 17, 2019
 *      Author: george
 */

#ifndef UI_KEYPAD_H_
#define UI_KEYPAD_H_

#define KB_STAT_MASK   0300
#define KB_RC_MASK     0077
#define KB_ROW_MASK    0070
#define KB_COL_MASK    0007
#define KB_PRESSED     0100
#define KB_ROW_1       0010
#define KB_ROW_2       0020
#define KB_ROW_3       0030
#define KB_ROW_4       0040
#define KB_COL_1       0001
#define KB_COL_2       0002
#define KB_COL_3       0003
#define KB_COL_4       0004
#define KB_COL_5       0005
#define kb_active_key_masked (kb_active_key & KB_RC_MASK)

#define KB_NONE 0000
#define KB_UP (KB_ROW_1 | KB_COL_1)
#define KB_DOWN (KB_ROW_1 | KB_COL_2)
#define KB_LEFT (KB_ROW_1 | KB_COL_3)
#define KB_RIGHT (KB_ROW_1 | KB_COL_4)
#define KB_F1_LOCK (KB_ROW_2 | KB_COL_1)
#define KB_F2_COIN (KB_ROW_2 | KB_COL_2)
#define KB_F3_CAMERA (KB_ROW_2 | KB_COL_4)
#define KB_F4_PICKER (KB_ROW_2 | KB_COL_5)
#define KB_RED (KB_ROW_3 | KB_COL_1)
#define KB_ORG (KB_ROW_3 | KB_COL_2)
#define KB_YEL (KB_ROW_3 | KB_COL_3)
#define KB_GRN (KB_ROW_3 | KB_COL_4)
#define KB_BLU (KB_ROW_3 | KB_COL_5)
#define KB_PUR (KB_ROW_4 | KB_COL_1)
#define KB_ROT (KB_ROW_4 | KB_COL_2)
#define KB_BACK (KB_ROW_4 | KB_COL_3)
#define KB_OK (KB_ROW_4 | KB_COL_5)

void kb_init();

#endif /* UI_KEYPAD_H_ */

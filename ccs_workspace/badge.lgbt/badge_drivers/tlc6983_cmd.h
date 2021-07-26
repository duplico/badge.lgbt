/*
 * tlc6983_cmd.h
 *
 *  Created on: Jul 24, 2021
 *      Author: george
 */

#ifndef BADGE_DRIVERS_TLC6983_CMD_H_
#define BADGE_DRIVERS_TLC6983_CMD_H_

enum WRITE_COMMAND_ID{
    W_FC0 = 0xAA00,
    W_FC1,
    W_FC2,
    W_FC3,
    W_FC4,
    W_FC5,
    W_FC6,
    W_FC7,
    W_FC8,
    W_FC9,
    W_FC10,
    W_CHIP_INDEX = 0xAA10,
    W_VSYNC = 0xAAF0,
    W_SOFT_RESET = 0xAA80,
    W_SRAM = 0xAA30
};

enum READ_COMMAND_ID{
    R_FC0 = 0xAA60,
    R_FC1,
    R_FC2,
    R_FC3,
    R_FC4,
    R_FC5,
    R_FC6,
    R_FC7,
    R_FC8,
    R_FC9,
    R_FC10,
    R_FC11,
    R_FC12,
    R_FC13,
    R_FC14,
    R_FC15,
    R_CHIP_INDEX = 0xAA70,
};

#define FC_0_0_RESERVED 0x00
#define FC_0_0_CHIP_NUM__1 0b00000
#define FC_0_0_PDC_EN__EN (0b1 << 8) // Pre-discharge
#define FC_0_0_PS_EN__EN (0b1 << 12) // Power saving
#define FC_0_0_PSP_MOD__HIGH (0b01 << 13) // Power saving mode plus
#define FC_0_0_PSP_MOD__MID (0b10 << 13)
#define FC_0_0_PSP_MOD__LOW (0b11 << 13)
#define FC_0_0_LODREM_EN (0b1 << 15) // Enable LED open load removal

#define FC_0_1_RESERVED 0x00
#define FC_0_1_SCAN_NUM__7 (6 << 0)
#define FC_0_1_SUBP_NUM__16  (0b000 << 5)
#define FC_0_1_SUBP_NUM__32  (0b001 << 5)
#define FC_0_1_SUBP_NUM__48  (0b010 << 5)
#define FC_0_1_SUBP_NUM__64  (0b011 << 5)
#define FC_0_1_SUBP_NUM__80  (0b100 << 5)
#define FC_0_1_SUBP_NUM__96  (0b101 << 5)
#define FC_0_1_SUBP_NUM__112 (0b110 << 5)
#define FC_0_1_SUBP_NUM__128 (0b111 << 5)
#define FC_0_1_FREQ_MOD__DISABLE_DIVIDER (0b1 << 11)
#define FC_0_1_FREQ_MUL__1  (0b0000 << 12)
#define FC_0_1_FREQ_MUL__2  (0b0001 << 12)
#define FC_0_1_FREQ_MUL__8  (0b0111 << 12) // Default
#define FC_0_1_FREQ_MUL__16 (0b1111 << 12)

#define FC_0_2_RESERVED 0b0001000000000000
#define FC_0_2_GRP_DLY_R__1 (0b001 << 3) // 0 is default
#define FC_0_2_GRP_DLY_G__1 (0b001 << 6) // 0 is default
#define FC_0_2_GRP_DLY_B__1 (0b001 << 9) // 0 is default
#define FC_0_2_MOD_SIZE__1 (0b01 << 14) // 1 device

#define FC_1_0_DEFAULT 0b1010010000000000 // only defines brightness steps
#define FC_1_0_SEG_LENGTH_128 127
#define FC_1_0_SEG_LENGTH_BITS 0x01FF

#define FC_1_1_DEFAULT 0b0000000010010100 // only defines brightness steps

#define FC_1_2_RESERVED 0x0000
// Blank time between each line:
#define FC_1_2_LINE_SWT__45  0x0000
#define FC_1_2_LINE_SWT__60  0b0001 << 5
#define FC_1_2_LINE_SWT__90  0b0010 << 5
#define FC_1_2_LINE_SWT__120 0b0011 << 5
#define FC_1_2_LINE_SWT__150 0b0100 << 5
#define FC_1_2_LINE_SWT__180 0b0101 << 5
#define FC_1_2_LINE_SWT__210 0b0110 << 5
#define FC_1_2_LINE_SWT__240 0b0111 << 5
#define FC_1_2_LINE_SWT__480 0b1111 << 5
// Blank time at the end of each subperiod:
// TODO: BLK_ADJ
#define FC_1_2_BLK_ADJ__0 (0 << 9)
#define FC_1_2_BLK_ADJ__4 (4 << 9)
#define FC_1_2_BLK_ADJ__31 (31 << 9)
#define FC_1_2_BLK_ADJ__63 (63 << 9)

#endif /* BADGE_DRIVERS_TLC6983_CMD_H_ */

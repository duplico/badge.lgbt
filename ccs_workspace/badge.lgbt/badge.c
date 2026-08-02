/*
 * badge.c
 *
 *  Created on: Jun 25, 2021
 *      Author: george
 */

#include "badge.h"
#include "version.h"

/// Kept in flash so a binary in hand can be identified without running it.
/// RETAIN keeps it past section elimination while nothing references it.
#pragma RETAIN(badge_fw_build)
const char badge_fw_build[] = BADGE_FW_BUILD;


/*
 * version.h
 *
 * Firmware identity: a curated release number that goes out on the wire, and
 * an exact build string for identifying a specific binary.
 */

#ifndef VERSION_H_
#define VERSION_H_

#include <stdint.h>

/// Release year, matching the release tag series (2021, 2026, ...).
#define BADGE_FW_YEAR 2026
/// Release number within the year, counting from 1.
#define BADGE_FW_REV 1

/// Release year as it goes on the wire: years since 2000, so one byte covers
/// through 2255. Peers compare (year, rev) as an ordered pair.
#define BADGE_FW_YEAR_WIRE ((uint8_t)(BADGE_FW_YEAR - 2000))
#define BADGE_FW_REV_WIRE ((uint8_t)(BADGE_FW_REV))

/// Exact build provenance from `git describe`, set by the build. Too long for
/// an advertisement; it identifies a binary in hand, via the map file, a
/// `strings` dump, or a debugger.
#ifndef BADGE_FW_BUILD
#define BADGE_FW_BUILD "unknown"
#endif

extern const char badge_fw_build[];

#endif /* VERSION_H_ */

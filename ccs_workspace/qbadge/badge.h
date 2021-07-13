/*
 * badge.h
 *
 *  Created on: Jun 29, 2019
 *      Author: george
 */

#ifndef BADGE_H_
#define BADGE_H_

#include <stdint.h>

#include <qc16.h>

#include <qc16_serial_common.h>

#include <ui/leds.h>

extern qbadge_conf_t badge_conf;

extern volatile uint32_t qc_clock;


extern uint8_t qbadges_near[BITFIELD_BYTES_QBADGE];
extern uint8_t qbadges_near_curr[BITFIELD_BYTES_QBADGE];
extern uint16_t qbadges_near_count;

extern uint16_t handler_near_id;
extern uint8_t handler_near_rssi;
extern element_type handler_near_element;
extern char handler_near_handle[QC16_BADGE_NAME_LEN+1];

extern uint16_t element_nearest_id[3];
extern uint8_t element_nearest_level[3];
extern uint8_t element_nearest_rssi[3];

extern uint16_t element_nearest_id_curr[3];
extern uint8_t element_nearest_level_curr[3];
extern uint8_t element_nearest_rssi_curr[3];

extern uint8_t dcfurs_nearby;
extern uint8_t dcfurs_nearby_curr;

extern uint8_t mission_accepted[3];
extern mission_t missions[3];

extern const uint8_t exp_required_per_level[6];

extern uint8_t badge_paired;
extern pair_payload_t paired_badge;
extern char handler_name_missionpicking[QC16_BADGE_NAME_LEN+1];
uint8_t mission_getting_possible();
uint8_t handler_human_nearby();
mission_t generate_mission();
void mission_begin_by_id(uint8_t mission_id);
uint8_t mission_begin();
void game_process_new_cbadge();
void complete_mission(mission_t *mission);
void complete_mission_id(uint8_t mission_id);
uint8_t mission_possible(mission_t *mission);
uint8_t mission_local_qualified_for_element_id(mission_t *mission, uint8_t element_position);
uint8_t mission_remote_qualified_for_element_id(mission_t *mission, uint8_t element_position);
uint8_t mission_element_id_we_fill(mission_t *mission);
uint8_t mission_element_id_remote_fills(mission_t *mission);
uint8_t mission_element_id_fulfilled_by(mission_t *mission, uint8_t *this_element_level, element_type this_element_selected, uint16_t this_id,
                                        uint8_t *other_element_level, element_type other_element_selected, uint16_t other_id, uint8_t paired);

void load_conf();
void write_conf();
void config_init();
uint8_t badge_seen(uint16_t id);
uint8_t badge_connected(uint16_t id);
uint8_t badge_near(uint16_t id);
uint8_t badge_near_curr(uint16_t id);
void set_badge_seen(uint16_t id, uint8_t type, uint8_t levels, char *name, uint8_t rssi);
uint8_t set_badge_connected(uint16_t id, uint8_t type, uint8_t levels, char *name);
void write_anim_curr();
void save_anim(char *name);
void load_anim_abs(char *pathname);
void load_anim(char *name);
void generate_config();
void process_seconds();
void radar_init();
void unlock_color_mod(led_tail_anim_mod mod);
void unlock_color_type(led_tail_anim_type color_type);

#endif /* BADGE_H_ */

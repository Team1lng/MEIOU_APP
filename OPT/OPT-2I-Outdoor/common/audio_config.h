#ifndef _AK_AUDIO_CONFIG_H_
#define _AK_AUDIO_CONFIG_H_
#include "ircut.h"
#define ENABLE (1)
#define DISABLE (0)

#if (IRCUT_MODE == 0) // 小门口机
/**************************************/
/* user vqe param */
/**************************************/
/* ai agc */
#define USER_AI_AGC_LEVEL (16384) // default (0.75) * (1 << 15)
#define USER_AI_AGC_MIN_GAIN (0)
#define USER_AI_AGC_MAX_GAIN (4)
#define USER_AI_NEAR_SENSITIVE (20)

/*         #### how to set agc ###
struct ak_audio_agc_attr user_ai_agc = {0};
user_ai_agc.agc_level = USER_AI_AGC_LEVEL;
user_ai_agc.agc_max_gain = USER_AI_AGC_MAX_GAIN;
user_ai_agc.agc_min_gain = USER_AI_AGC_MIN_GAIN;
user_ai_agc.near_sensitivity = USER_AI_NEAR_SENSITIVE;
user_ai_agc.enable = ENABLE;
ak_ai_set_agc_attr(ai_handle_id,&user_ai_agc);*/

/* ai nr */
#define USER_AI_NOISE_SUP_DB (-25) // noise suppress db

/*         #### how to set ai nr ###
struct ak_audio_nr_attr user_ai_nr_attr = {0};
user_ai_nr_attr.noise_suppress_db = USER_AI_NOISE_SUP_DB;
user_ai_nr_attr.enable = ENABLE;
ak_ai_set_nr_attr(ai_handle_id,&user_ai_nr_attr);*/

/* ai aec */
#define USER_AEC_OUT_DIGI_GAIN (1024)
#define USER_AEC_IN_DIGI_GAIN (1024)
#define USER_AEC_TAIL (512)
// #define USER_AEC_OUT_THRESHLD    (9830)

/*         #### how to set aec ###
struct ak_audio_aec_attr user_aec_attr = {0};
user_aec_attr.audio_in_digi_gain = USER_AEC_IN_DIGI_GAIN;
user_aec_attr.audio_out_digi_gain = USER_AEC_OUT_DIGI_GAIN;
user_aec_attr.audio_out_threshold = USER_AEC_OUT_THRESHLD;
user_aec_attr.enable = ENABLE;
user_aec_attr.tail = 512; //don't need to set this value now
ak_ai_set_aec_attr(ai_handle_id, &user_aec_attr);*/

/* ai aslc */
#define USER_AI_LIMIT (32768)
#define USER_AI_DB (3)

/*         #### how to set ai aslc ###
struct ak_audio_aslc_attr user_ai_aslc_attr = {0};
user_ai_aslc_attr.aslc_db = USER_AI_DB;
user_ai_aslc_attr.limit = USER_AI_LIMIT;
ak_ai_set_aslc_attr(ai_handle_id, &user_ai_aslc_attr);*/

/* ao nr */
#define USER_AO_NOISE_SUP_DB (-25) // noise suppress db
/*         #### how to set ao nr ###
struct ak_audio_nr_attr user_ao_nr_attr = {0};
user_ao_nr_attr.noise_suppress_db = USER_AO_NOISE_SUP_DB;
user_ao_nr_attr.enable = ENABLE;
ak_ao_set_nr_attr(ao_handle_id,&user_ao_nr_attr);*/

/* ao aslc */
#define USER_AO_LIMIT (7208 /* 9830 */) //(3276)
#define USER_AO_DB (8)
/*         #### how to set ao aslc ###
struct ak_audio_aslc_attr user_ao_aslc_attr = {0};
user_ao_aslc_attr.aslc_db = USER_AO_DB;
user_ao_aslc_attr.limit = USER_AO_LIMIT;
ak_ao_set_aslc_attr(ao_handle_id, &user_ao_aslc_attr);*/

/* set ai voice quality enhancement param */
// struct ak_audio_nr_attr user_ai_nr_attr ={USER_AI_NOISE_SUP_DB,0,ENABLE};
// struct ak_audio_agc_attr user_ai_agc_attr = {USER_AI_AGC_LEVEL,USER_AI_AGC_MAX_GAIN,USER_AI_AGC_MIN_GAIN,USER_AI_NEAR_SENSITIVE, 0, ENABLE};
// struct ak_audio_aec_attr user_ai_aec_attr = {0, USER_AEC_OUT_DIGI_GAIN, USER_AEC_IN_DIGI_GAIN, 0, USER_AEC_TAIL, ENABLE};
// struct ak_audio_aslc_attr user_ai_aslc_attr = {USER_AI_LIMIT, USER_AI_DB, ENABLE};

/* set ao voice quality enhancement param */
// struct ak_audio_nr_attr user_ao_nr_attr = {USER_AO_NOISE_SUP_DB, 0, ENABLE};
// struct ak_audio_aslc_attr user_ao_aslc_attr = {USER_AO_LIMIT, USER_AO_DB, ENABLE};
/* gain */

#define USER_AI_GAIN (2)

#ifdef NUMERIC_KEY_MODE
#define USER_AI_VOLUME (4)
#else
#define USER_AI_VOLUME (-2)
#endif

/*			### how to set ai gain ###
ak_ai_set_gain(ai_handle_id,USER_AI_GAIN);*/

#define USER_AO_GAIN (3) // 增益

#ifdef NUMERIC_KEY_MODE
#define USER_AO_VOLUME (20) // 音量
#else
#define USER_AO_VOLUME (10) // 音量
#endif
/*			### how to set ao gain ###
ak_ao_set_gain(ao_handle_id,USER_AO_GAIN);*/

#define DEFAULT_VOLUME_L_INDEX (0)
#define MIN_VOLUME_L_INDEX (1)
#define MAX_VOLUME_L_INDEX (6)
/*			### Tone volume level ###
 */
#define VOLUME_ADD 3

#define TONE_VOLUME_L_DEFAULT (-9)
#define TONE_VOLUME_L_1 (-15 + VOLUME_ADD)
#define TONE_VOLUME_L_2 (-12 + VOLUME_ADD)
#define TONE_VOLUME_L_3 (-9 + VOLUME_ADD)
#define TONE_VOLUME_L_4 (-6 + VOLUME_ADD)
#define TONE_VOLUME_L_5 (-3 + VOLUME_ADD)
#define TONE_VOLUME_L_6 (0 + VOLUME_ADD)
#define TONE_VOLUME_L_MAX_INDEX (8)

/*			### Talk volume level ###
 */
#define TALK_VOLUME_L_DEFAULT (7)
#define TALK_VOLUME_L_1 (-10 + VOLUME_ADD)
#define H (-5 + VOLUME_ADD)
#define TALK_VOLUME_L_3 (0 + VOLUME_ADD)
#define TALK_VOLUME_L_4 (5 + VOLUME_ADD)
#define TALK_VOLUME_L_5 (10 + VOLUME_ADD)
#define TALK_VOLUME_L_6 (15 + VOLUME_ADD)
#define TALK_VOLUME_L_MAX_INDEX (8)

/**********************  以上為門口機接通室內機時的參數  *********************/

/**********************  以下為門口機接通 tuya App 時的參數  *********************/
#define TUYA_USER_AI_AGC_LEVEL (16384) // default (0.75) * (1 << 15)
#define TUYA_USER_AI_AGC_MIN_GAIN (0)
#define TUYA_USER_AI_AGC_MAX_GAIN (4)
#define TUYA_USER_AI_NEAR_SENSITIVE (1)

#define TUYA_USER_AI_NOISE_SUP_DB (-25)

#define TUYA_USER_AEC_OUT_DIGI_GAIN (1024)
#define TUYA_USER_AEC_IN_DIGI_GAIN (512)
#define TUYA_USER_AEC_TAIL (512)

#define TUYA_USER_AI_LIMIT (32768) // 極限音量
#define TUYA_USER_AI_DB (10)
#define TUYA_USER_AI_GAIN (3)    // 增益
#define TUYA_USER_AI_VOLUME (20) // 音量

#define TUYA_USER_AO_NOISE_SUP_DB (-25)

#define TUYA_USER_AO_LIMIT (16384) // 極限音量
#define TUYA_USER_AO_DB (20)
#define TUYA_USER_AO_GAIN (2)    // 增益
#define TUYA_USER_AO_VOLUME (15) // 音量

#else                             // 大门口机

/**************************************/
/* user vqe param */
/**************************************/
/* ai agc */
#define USER_AI_AGC_LEVEL (16384) // default (0.75) * (1 << 15)
#define USER_AI_AGC_MIN_GAIN (0)
#define USER_AI_AGC_MAX_GAIN (4)
#define USER_AI_NEAR_SENSITIVE (40)

/*         #### how to set agc ###
struct ak_audio_agc_attr user_ai_agc = {0};
user_ai_agc.agc_level = USER_AI_AGC_LEVEL;
user_ai_agc.agc_max_gain = USER_AI_AGC_MAX_GAIN;
user_ai_agc.agc_min_gain = USER_AI_AGC_MIN_GAIN;
user_ai_agc.near_sensitivity = USER_AI_NEAR_SENSITIVE;
user_ai_agc.enable = ENABLE;
ak_ai_set_agc_attr(ai_handle_id,&user_ai_agc);*/

/* ai nr */
#define USER_AI_NOISE_SUP_DB (-25) // noise suppress db

/*         #### how to set ai nr ###
struct ak_audio_nr_attr user_ai_nr_attr = {0};
user_ai_nr_attr.noise_suppress_db = USER_AI_NOISE_SUP_DB;
user_ai_nr_attr.enable = ENABLE;
ak_ai_set_nr_attr(ai_handle_id,&user_ai_nr_attr);*/

/* ai aec */
#define USER_AEC_OUT_DIGI_GAIN (1024)
#define USER_AEC_IN_DIGI_GAIN (1024)
#define USER_AEC_TAIL (512)
// #define USER_AEC_OUT_THRESHLD    (9830)

/*         #### how to set aec ###
struct ak_audio_aec_attr user_aec_attr = {0};
user_aec_attr.audio_in_digi_gain = USER_AEC_IN_DIGI_GAIN;
user_aec_attr.audio_out_digi_gain = USER_AEC_OUT_DIGI_GAIN;
user_aec_attr.audio_out_threshold = USER_AEC_OUT_THRESHLD;
user_aec_attr.enable = ENABLE;
user_aec_attr.tail = 512; //don't need to set this value now
ak_ai_set_aec_attr(ai_handle_id, &user_aec_attr);*/

/* ai aslc */
#define USER_AI_LIMIT (3276)
#define USER_AI_DB (8)

/*         #### how to set ai aslc ###
struct ak_audio_aslc_attr user_ai_aslc_attr = {0};
user_ai_aslc_attr.aslc_db = USER_AI_DB;
user_ai_aslc_attr.limit = USER_AI_LIMIT;
ak_ai_set_aslc_attr(ai_handle_id, &user_ai_aslc_attr);*/

/* ao nr */
#define USER_AO_NOISE_SUP_DB (-25) // noise suppress db
/*         #### how to set ao nr ###
struct ak_audio_nr_attr user_ao_nr_attr = {0};
user_ao_nr_attr.noise_suppress_db = USER_AO_NOISE_SUP_DB;
user_ao_nr_attr.enable = ENABLE;
ak_ao_set_nr_attr(ao_handle_id,&user_ao_nr_attr);*/

/* ao aslc */
#define USER_AO_LIMIT (9830)       //(3276)
#define USER_AO_DB (8)
/*         #### how to set ao aslc ###
struct ak_audio_aslc_attr user_ao_aslc_attr = {0};
user_ao_aslc_attr.aslc_db = USER_AO_DB;
user_ao_aslc_attr.limit = USER_AO_LIMIT;
ak_ao_set_aslc_attr(ao_handle_id, &user_ao_aslc_attr);*/

/* set ai voice quality enhancement param */
// struct ak_audio_nr_attr user_ai_nr_attr ={USER_AI_NOISE_SUP_DB,0,ENABLE};
// struct ak_audio_agc_attr user_ai_agc_attr = {USER_AI_AGC_LEVEL,USER_AI_AGC_MAX_GAIN,USER_AI_AGC_MIN_GAIN,USER_AI_NEAR_SENSITIVE, 0, ENABLE};
// struct ak_audio_aec_attr user_ai_aec_attr = {0, USER_AEC_OUT_DIGI_GAIN, USER_AEC_IN_DIGI_GAIN, 0, USER_AEC_TAIL, ENABLE};
// struct ak_audio_aslc_attr user_ai_aslc_attr = {USER_AI_LIMIT, USER_AI_DB, ENABLE};

/* set ao voice quality enhancement param */
// struct ak_audio_nr_attr user_ao_nr_attr = {USER_AO_NOISE_SUP_DB, 0, ENABLE};
// struct ak_audio_aslc_attr user_ao_aslc_attr = {USER_AO_LIMIT, USER_AO_DB, ENABLE};
/* gain */

#define USER_AI_GAIN (4)
#define USER_AI_VOLUME (8)

/*			### how to set ai gain ###
ak_ai_set_gain(ai_handle_id,USER_AI_GAIN);*/

#define USER_AO_GAIN (2)   // 增益
#define USER_AO_VOLUME (8) // 音量

/*			### how to set ao gain ###
ak_ao_set_gain(ao_handle_id,USER_AO_GAIN);*/

#define DEFAULT_VOLUME_L_INDEX (0)
#define MIN_VOLUME_L_INDEX (1)
#define MAX_VOLUME_L_INDEX (6)
/*			### Tone volume level ###
 */
#define VOLUME_ADD 3

#define TONE_VOLUME_L_DEFAULT (-9)
#define TONE_VOLUME_L_1 (-15 + VOLUME_ADD)
#define TONE_VOLUME_L_2 (-12 + VOLUME_ADD)
#define TONE_VOLUME_L_3 (-9 + VOLUME_ADD)
#define TONE_VOLUME_L_4 (-6 + VOLUME_ADD)
#define TONE_VOLUME_L_5 (-3 + VOLUME_ADD)
#define TONE_VOLUME_L_6 (0 + VOLUME_ADD)
#define TONE_VOLUME_L_MAX_INDEX (8)

/*			### Talk volume level ###
 */
#define TALK_VOLUME_L_DEFAULT (7)
#define TALK_VOLUME_L_1 (-10 + VOLUME_ADD)
#define H (-5 + VOLUME_ADD)
#define TALK_VOLUME_L_3 (0 + VOLUME_ADD)
#define TALK_VOLUME_L_4 (5 + VOLUME_ADD)
#define TALK_VOLUME_L_5 (10 + VOLUME_ADD)
#define TALK_VOLUME_L_6 (15 + VOLUME_ADD)
#define TALK_VOLUME_L_MAX_INDEX (8)

/**********************  以上為門口機接通室內機時的參數  *********************/

/**********************  以下為門口機接通 tuya App 時的參數  *********************/
#define TUYA_USER_AI_AGC_LEVEL (16384) // default (0.75) * (1 << 15)
#define TUYA_USER_AI_AGC_MIN_GAIN (0)
#define TUYA_USER_AI_AGC_MAX_GAIN (4)
#define TUYA_USER_AI_NEAR_SENSITIVE (25)

#define TUYA_USER_AI_NOISE_SUP_DB (-25)

#define TUYA_USER_AEC_OUT_DIGI_GAIN (1024)
#define TUYA_USER_AEC_IN_DIGI_GAIN (1024)
#define TUYA_USER_AEC_TAIL (512)

#define TUYA_USER_AI_LIMIT (3276) // 極限音量
#define TUYA_USER_AI_DB (5)
#define TUYA_USER_AI_GAIN (1)   // 增益
#define TUYA_USER_AI_VOLUME (5) // 音量

#define TUYA_USER_AO_NOISE_SUP_DB (-25)

#define TUYA_USER_AO_LIMIT (9830) // 極限音量
#define TUYA_USER_AO_DB (2)
#define TUYA_USER_AO_GAIN (2)   // 增益
#define TUYA_USER_AO_VOLUME (2) // 音量

#endif

#endif

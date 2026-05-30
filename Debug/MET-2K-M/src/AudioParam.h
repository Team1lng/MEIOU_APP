/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-07-27 08:29:42
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-07-31 14:15:37
 * @FilePath: /Doorbell/src/AudioParam.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _AUDIO_PARAM_H_
#define _AUDIO_PARAM_H_
/* set ai param */
struct ak_audio_nr_attr default_ai_nr_attr;
struct ak_audio_agc_attr default_ai_agc_attr;
struct ak_audio_aec_attr default_ai_aec_attr;
struct ak_audio_aslc_attr default_ai_aslc_attr;
struct ak_audio_eq_attr default_ai_eq_attr;

/* set ao param */
struct ak_audio_nr_attr default_ao_nr_attr;
struct ak_audio_aslc_attr default_ao_aslc_attr;
/* gain */

int default_ai_gain;

/*			### how to set ai gain ###
ak_ai_set_gain(ai_handle_id,default_ai_gain);*/

int default_ao_gain;

/*			### how to set ao gain ###
ak_ao_set_gain(ao_handle_id,default_ao_gain);*/

struct ak_audio_eq_attr default_ao_eq_attr;
#endif

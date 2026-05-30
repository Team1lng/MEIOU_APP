#ifndef _AK_AUDIO_CONFIG_H_
#include "ak_common_audio.h"
/* set ai param */
struct ak_audio_nr_attr default_ai_nr_attr = {-25, 0, 1};
struct ak_audio_agc_attr default_ai_agc_attr = {24576, 6, 0, 40, 0, 1};
struct ak_audio_aec_attr default_ai_aec_attr = {0, 716, 1024, 0, 512, 1, 0};
struct ak_audio_aslc_attr default_ai_aslc_attr = {26214, 10, 0};
struct ak_audio_eq_attr default_ai_eq_attr = {
    0,
    0,
    {5000, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
    {-6144, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {716, 716, 716, 716, 716, 716, 716, 716, 716, 716},
    {TYPE_HSF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

/* set ao param */
struct ak_audio_nr_attr default_ao_nr_attr = {-25, 0, 1};
struct ak_audio_aslc_attr default_ao_aslc_attr = {19660, 15, 0};
/* gain */

int default_ai_gain = 2;

/*			### how to set ai gain ###
ak_ai_set_gain(ai_handle_id,default_ai_gain);*/

int default_ao_gain = 2;

/*			### how to set ao gain ###
ak_ao_set_gain(ao_handle_id,default_ao_gain);*/

struct ak_audio_eq_attr default_ao_eq_attr = {
    0,
    2,
    {1200, 4000, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},
    {-6144, -6144, 0, 0, 0, 0, 0, 0, 0, 0},
    {716, 716, 717, 717, 717, 717, 717, 717, 717, 717},
    {TYPE_HPF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0}};
#endif

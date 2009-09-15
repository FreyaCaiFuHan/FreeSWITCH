/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2009, Mathieu Parent <math.parent@gmail.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Mathieu Parent <math.parent@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Mathieu Parent <math.parent@gmail.com>
 *
 * mod_tts_commandline.c -- System command ASR TTS Interface
 *
 */

#include <unistd.h>
#include <switch.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_tts_commandline_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_tts_commandline_shutdown);
SWITCH_MODULE_DEFINITION(mod_tts_commandline, mod_tts_commandline_load, mod_tts_commandline_shutdown, NULL);

static struct {
	char *command;
} globals;

SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_command, globals.command);

struct tts_commandline_data {
    switch_file_handle_t fh;
	char *voice_name;
	char file[512];
};

typedef struct tts_commandline_data tts_commandline_t;

static switch_status_t tts_commandline_speech_open(switch_speech_handle_t *sh, const char *voice_name, int rate, switch_speech_flag_t *flags)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "tts_commandline_speech_open(?,%s,%d,%u)\n",voice_name,rate, *flags);
    
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
        "speech_handle: name = %s, rate = %d, speed = %d, samples = %d, voice = %s, engine = %s, param = %s\n",
        sh->name, sh->rate, sh->speed, sh->samples, sh->voice, sh->engine, sh->param); 

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "voice = %s, rate = %d\n", voice_name, rate);

	tts_commandline_t *info = switch_core_alloc(sh->memory_pool, sizeof(*info));

	info->voice_name = switch_core_strdup(sh->memory_pool, voice_name);

	switch_snprintf(info->file, sizeof(info->file), "%s%s.wav", SWITCH_GLOBAL_dirs.temp_dir, "tts_commandline");
    
    sh->private_info = info;
    
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t tts_commandline_speech_close(switch_speech_handle_t *sh, switch_speech_flag_t *flags)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "tts_commandline_speech_close(?,%u)\n", *flags);

	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);
    
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t tts_commandline_speech_feed_tts(switch_speech_handle_t *sh, char *text, switch_speech_flag_t *flags)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "tts_commandline_speech_feed_tts(?,%s,%u)\n", text, *flags);
	
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);

    char *message;
	message = switch_core_strdup(sh->memory_pool, globals.command);
    message = switch_string_replace(message, "${text}", text);
    message = switch_string_replace(message, "${voice}", info->voice_name);
    message = switch_string_replace(message, "${file}", info->file);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Executing: %s\n", message);
    
    if (switch_system(message, SWITCH_TRUE) < 0) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Failed to execute command: %s\n", message);
       return SWITCH_STATUS_FALSE;
    }
	
    if (switch_core_file_open(&info->fh,
                            info->file,
                            0, //number_of_channels,
                            0, //samples_per_second,
                            SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT, NULL) != SWITCH_STATUS_SUCCESS) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Failed to open file: %s\n", info->file);
       return SWITCH_STATUS_FALSE;
    }
    
	// sh->rate = info->fh.samplerate;
	// sh->speed = info->fh.speed;
	// sh->samples = info->fh.samples;
	// sh->samplerate = info->fh.samplerate / 2;
	// sh->native_rate = info->fh.native_rate;
    
	// info->fh.speed = sh->speed;
	// info->fh.samples = sh->samples;
    // info->fh.samplerate = sh->samplerate;

    
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
    "File handle info: flags=%u, samples=%u,samplerate=%u,native_rate=%u,channels=%u,format=%u,sections=%u,seekable=%u,sample_count=%u,speed=%u\n",
    info->fh.flags, info->fh.samples, info->fh.samplerate, info->fh.native_rate, info->fh.channels,
    info->fh.format, info->fh.sections, info->fh.seekable, info->fh.sample_count, info->fh.speed);

    sh->private_info = info;

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t tts_commandline_speech_read_tts(switch_speech_handle_t *sh, void *data, size_t *datalen, switch_speech_flag_t *flags)
{
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);
    
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "tts_commandline_speech_read_tts(?,?,%u,%u)\n", *datalen, *flags);
    
    if (switch_core_file_read(&info->fh, data, datalen) != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "done\n");
        return SWITCH_STATUS_FALSE;
    }
    if(datalen == 0) {
        return SWITCH_STATUS_BREAK;
    } else {
        return SWITCH_STATUS_SUCCESS;
    }
}

static void tts_commandline_speech_flush_tts(switch_speech_handle_t *sh)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "tts_commandline_speech_flush_tts(?)\n");
    
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);
    
    switch_core_file_close(&info->fh);
    if (unlink(info->file) != 0) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Sound file [%s] delete failed\n", info->file);
    }
}

static void tts_commandline_text_param_tts(switch_speech_handle_t *sh, char *param, const char *val)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "tts_commandline_text_param_tts(?,%s,%s)\n", param, val);
    
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);

	if (!strcasecmp(param, "voice")) {
        info->voice_name = switch_core_strdup(sh->memory_pool, val);
    }
}

static void tts_commandline_numeric_param_tts(switch_speech_handle_t *sh, char *param, int val)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "tts_commandline_numeric_param_tts(?,%s,%d)\n", param, val);
}

static void tts_commandline_float_param_tts(switch_speech_handle_t *sh, char *param, double val)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "tts_commandline_numeric_param_tts(?,%s,%e)\n", param, val);
}

SWITCH_MODULE_LOAD_FUNCTION(mod_tts_commandline_load)
{
	switch_speech_interface_t *speech_interface;

	// set_global_command("tts \"${text}\" \"${file}\"");
	set_global_command("cp \"/opt/freeswitch/sounds/en/us/callie/ivr/8000/ivr-sample_submenu.wav\" \"${file}\"");
	
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	speech_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_SPEECH_INTERFACE);
	speech_interface->interface_name = "tts_commandline";
	speech_interface->speech_open = tts_commandline_speech_open;
	speech_interface->speech_close = tts_commandline_speech_close;
	speech_interface->speech_feed_tts = tts_commandline_speech_feed_tts;
	speech_interface->speech_read_tts = tts_commandline_speech_read_tts;
	speech_interface->speech_flush_tts = tts_commandline_speech_flush_tts;
	speech_interface->speech_text_param_tts = tts_commandline_text_param_tts;
	speech_interface->speech_numeric_param_tts = tts_commandline_numeric_param_tts;
	speech_interface->speech_float_param_tts = tts_commandline_float_param_tts;

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_tts_commandline_shutdown)
{
	switch_safe_free(globals.command);

	return SWITCH_STATUS_UNLOAD;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
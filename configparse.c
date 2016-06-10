#include "common.h"
#include <gst/gst.h>
#include <glib.h> 
#include <string.h>

static gboolean config_decode
(GKeyFile *file, struct config_head *config);

struct config_head *config_init(const gchar *path);
static gboolean config_set_default(struct config_head *config);

struct config_head *config_init(const gchar *path)
{
	GKeyFile *file = NULL;
	file = g_key_file_new();

	struct config_head *head = NULL;
	head = g_malloc(sizeof(struct config_head));
	head->general = g_malloc(sizeof(struct record_config));
	head->record_list = g_sequence_new(NULL);

	if (NULL != path) {
		g_key_file_load_from_file
			(file, path, G_KEY_FILE_NONE, NULL);
		config_decode(file, head);
		return head;
	} else {
		if (g_key_file_load_from_file
		    (file, "/etc/shek_edr.conf", G_KEY_FILE_NONE,
		     NULL))
			config_decode(file, head);

		if (g_key_file_load_from_file
		     (file, "~/.shek_edr.conf", G_KEY_FILE_NONE,
		     NULL))
			config_decode(file, head);

		if (g_key_file_load_from_file
		    (file, "./shek_edr.conf", G_KEY_FILE_NONE,
		     NULL))
			config_decode(file, head);

		return head;
	}

	return NULL;
}

static gboolean config_decode(GKeyFile *file, struct config_head *config){
	if (NULL == file || NULL == config)
		return FALSE;
	gchar **groups = NULL;
	gsize length;

	GRegex *regex;

	groups = g_key_file_get_groups(file, &length);
	if (0 == length)
		return FALSE;

	if (g_key_file_has_group(file, "general")){
		memset(config->general, 0, sizeof(struct record_config));
		config->general->location =
			g_key_file_get_value
			(file, "general", "location", NULL);
		config->general->period =
			g_key_file_get_integer
			(file, "general", "switch_time", NULL);
		config->general->dst_format =
			g_key_file_get_value
			(file, "general", "file_name", NULL);
		config->general->video.encoder =
			g_key_file_get_value
			(file, "general", "video_encoder", NULL);
		config->general->audio.encoder =
			g_key_file_get_value
			(file, "general", "audio_encoder", NULL);
		config->general->video.width =
			g_key_file_get_integer
			(file, "general", "width", NULL);
		config->general->video.height =
			g_key_file_get_integer
			(file, "general", "height", NULL);
		config->general->enable = TRUE;

		config->general->name = "general";
	}

	regex = g_regex_new("camera[0-9]{1,}", 0, 0, NULL);
	do {
		struct record_config *data =
		g_malloc0(sizeof(struct record_config));

		if (g_regex_match(regex, *groups, 0, 0)){
			data->name = g_strdup(*groups);

			data->video.cam_name =
				g_key_file_get_value
				(file, *groups, "camera_device", NULL);
			if (NULL == data->video.cam_name) {
				g_free(data);
				groups++;
				continue;
			}

			data->enable = 
				g_key_file_get_integer
				(file, *groups, "enable", NULL);

			data->video.encoder =
				g_key_file_get_value
				(file, *groups, "video_encoder", 
				 NULL);
			data->video.width =
				g_key_file_get_integer
				(file, *groups, "width", 
				 NULL);
			data->video.height =
				g_key_file_get_integer
				(file, *groups, "height", 
				 NULL);

			data->audio.mic_name =
				g_key_file_get_value
				(file, *groups, "audio_device", NULL);
			data->audio.encoder =
				g_key_file_get_value
				(file, *groups, "audio_encoder", 
				 NULL);

			data->r.pipeline = NULL;

			g_sequence_append(config->record_list, (gpointer)data);
		}
		else {
			g_free(data);
		}

		groups++;
	} while(NULL != *groups);

	config_set_default(config);

	g_regex_unref(regex);

	return TRUE;
}

static gboolean config_set_default(struct config_head *config){
        GSequenceIter *begin, *current, *next;

        begin = g_sequence_get_begin_iter(config->record_list);
        current = begin;
        next = begin;
	struct record_config *data = NULL;

        while(!g_sequence_iter_is_end(next)){
        	current = next;
                data = g_sequence_get(current);

		if (NULL == data->video.encoder)
			data->video.encoder = config->general->video.encoder;
		if (NULL == data->audio.encoder)
			data->audio.encoder = config->general->audio.encoder;
		if (NULL == data->location)
			data->location = config->general->location;
		if (0 == data->video.width)
			data->video.width = config->general->video.width;
		if (0 == data->video.height)
			data->video.height = config->general->video.height;
		if (0 == data->period)
			data->period = config->general->period;
		if (NULL == data->dst_format)
			data->dst_format = config->general->dst_format;

                next = g_sequence_iter_next(current);
	}
	return TRUE;
}

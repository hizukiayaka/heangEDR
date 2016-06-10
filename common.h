#ifndef _COMMON_H_
#define _COMMON_H_
#include <glib.h>
#include <gst/gst.h>

GST_DEBUG_CATEGORY_EXTERN(shek_edr_debug);
#define GST_CAT_DEFAULT shek_edr_debug

#define ERR_MSG_SIZE 100

struct config_head;
struct record_config;

struct config_head {
	struct record_config *general;
	GSequence *record_list;
};

struct record_config {
	/* get from config file*/
	gboolean enable;
	gchar *name;
	gchar *location;
	gchar *dst_format;
	guint period;
	struct {
		gchar *cam_name;
		gchar *encoder;
		gint width;
		gint height;
	} video;
	struct {
		gchar *mic_name;
		gchar *encoder;
	} audio;
	/* some runtime data */
	struct {
		gchar *err_msg;
		guint bus_watch_id;
		gint status;
		gpointer pipeline;
		gpointer sink;
	}r;
};
#endif

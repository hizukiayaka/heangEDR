#include "common.h"
#include <math.h>
#include "record.h"
#include "cam_encoding.h"

#define NS_TO_SEC 60000000000

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);

static gboolean uriplug
    (GstElement * bin, GstPad * pad, GstCaps * caps, gpointer user_data);
static void on_pad_added
    (GstElement * element, GstPad * pad, gpointer data);

static gchar *update_record_dest
	(GstElement * element, guint fragment_id, gpointer data);
static gchar *create_path(gchar * format, gchar * name, gchar * location);

static gchar *create_path(gchar * format, gchar * name, gchar * location)
{
	gchar *dst = NULL, *sub_dst = NULL;
	GDateTime *date = g_date_time_new_now_local();
	sub_dst = g_date_time_format(date, format);
	gchar *directory = g_strjoin(NULL, location, sub_dst, NULL);
	dst = g_strdup_printf("%s/%s.mkv", directory, name);

	g_mkdir_with_parents(directory, 0755);
	g_free(sub_dst);
	g_free(directory);

	return dst;
}

static gchar *update_record_dest(GstElement * element, guint fragment_id, gpointer data)
{
	struct record_config *priv = (struct record_config *) data;
	
	return create_path(priv->dst_format, priv->name, priv->location);
}

gpointer record_pipeline(gpointer data)
{
	gchar *dst;
	GstElement *pipeline, *bin, *source, *sink, *muxer;
	GstBus *bus;
	struct record_config *priv = (struct record_config *) data;
	gboolean link_ok;

	dst = create_path(priv->dst_format, priv->name, priv->location);
	/* Create gstreamer elements */
	pipeline = gst_pipeline_new(priv->name);

	bin =  create_video_audio_bin(data);
	source = gst_element_factory_make("decodebin", "decodebin");

	muxer = gst_element_factory_make("matroskamux", "muxer");
	sink = gst_element_factory_make("splitmuxsink", "sink");

	if (!pipeline || !bin || !source || !sink || !muxer) {
		GST_ERROR("One element could not be created. Exiting.");
		return NULL;
	}

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	priv->r.bus_watch_id = gst_bus_add_watch(bus, bus_call, data);

	gst_object_unref(bus);

	/* Set up the pipeline */
	g_object_set(G_OBJECT(sink), "muxer", muxer, NULL);
	g_object_set(G_OBJECT(sink), "location", dst, NULL);
	g_free(dst);
	/* Never forget type conversion here, or it won't work */
	g_object_set(G_OBJECT(sink), "max-size-time", (guint64)(priv->period * NS_TO_SEC), NULL);
	/*this singal decide the fomart is accept */
	g_signal_connect(source, "autoplug-continue", G_CALLBACK(uriplug),
			 NULL);
	/* we link the elements together */
	g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added),
			 sink);
	/* offer the new dest location */
	g_signal_connect(sink, "format-location", G_CALLBACK(update_record_dest),
			 (gpointer)priv);
	/* we add all elements into the pipeline */
	gst_bin_add_many(GST_BIN(pipeline), bin, source, sink, NULL);
	//gst_element_link_pads(bin, "video", source, "sink");
	link_ok = gst_element_link(bin, source);
	if (!link_ok)
		GST_ERROR("can't link bin with source");

	g_object_set (G_OBJECT (pipeline), "message-forward", TRUE, NULL);

	priv->r.pipeline = (void *) pipeline;
	priv->r.sink = (void *) sink;

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	priv->r.status = 1;

	return NULL;
}

const gchar *supported_video_caps[] = {
	"video/x-h264, parsed=true",
	"video/x-h265, parsed=true",
	"video/x-vp8",
	"image/jpeg",
	NULL
};

const gchar *supported_audio_caps[] = {
	"audio/x-ac3",
	"audio/x-vorbis",
	"audio/x-flac",
	NULL
};

gboolean uriplug
    (GstElement * bin, GstPad * pad, GstCaps * caps, gpointer user_data) {
	GST_LOG("caps are %" GST_PTR_FORMAT, caps);
	for (gint i = 0; i < (sizeof(supported_video_caps)/sizeof(gchar *)); i++)  {
		if (NULL == supported_video_caps[i])
			continue;
		if (gst_caps_is_subset(caps, gst_caps_from_string(supported_video_caps[i])))
			return FALSE;

	}
	for (gint i = 0; i < (sizeof(supported_audio_caps)/sizeof(gchar *)); i++)  {
		if (NULL == supported_audio_caps[i])
			continue;
		if (gst_caps_is_subset(caps, gst_caps_from_string(supported_audio_caps[i])))
			return FALSE;

	}

	return TRUE;
}

static void on_pad_added(GstElement * element, GstPad * pad, gpointer data)
{
	gchar *sinkpad_name;
	GstPad *sinkpad;
	GstElement *sink = (GstElement *) data;
	GstPadTemplate *sinkpad_template;
	GstCaps * caps;

	caps = gst_pad_get_current_caps(pad);
	GST_LOG("caps are %" GST_PTR_FORMAT, caps);

	for (gint i = 0; i < (sizeof(supported_video_caps)/sizeof(gchar *)); i++)  {
		if (NULL == supported_video_caps[i])
			continue;
		if (gst_caps_is_subset(gst_pad_get_current_caps(pad),
			gst_caps_from_string(supported_video_caps[i])))
			sinkpad_name = "video";

	}
	for (gint i = 0; i < (sizeof(supported_audio_caps)/sizeof(gchar *)); i++)  {
		if (NULL == supported_audio_caps[i])
			continue;
		if (gst_caps_is_subset(gst_pad_get_current_caps(pad),
			gst_caps_from_string(supported_audio_caps[i])))
			sinkpad_name = "audio_%u";
	}
			
	sinkpad_template = gst_element_class_get_pad_template
		(GST_ELEMENT_GET_CLASS (sink), sinkpad_name);

	sinkpad = gst_element_request_pad (sink, sinkpad_template, NULL, NULL);

	if (GST_PAD_LINK_OK != gst_pad_link(pad, sinkpad))
		GST_DEBUG("link failed %d", __LINE__);

	gst_object_unref(sinkpad);
}

static gboolean bus_call(GstBus * bus, GstMessage * msg, gpointer data)
{

	struct record_config *priv = (struct record_config *)data;

	gchar *debug;
	GError *error;
	GstMessage *origmsg;
	const GstStructure *s = gst_message_get_structure(msg);

	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_ELEMENT:
		gst_structure_get(s,"message", GST_TYPE_MESSAGE, &origmsg,
		  NULL);
		break;
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(msg, &error, &debug);
		g_free(debug);

		if (!priv->r.err_msg) {
			priv->r.err_msg = g_malloc(100 * sizeof(gchar));
			if (!priv->r.err_msg) {
				priv->r.status = -2;
				break;
			}
		}
		g_strlcpy(priv->r.err_msg, error->message, ERR_MSG_SIZE);

		GST_ERROR("Error: %s\n", error->message);
		g_error_free(error);

		priv->r.status = -1;
		gst_element_set_state(priv->r.pipeline, GST_STATE_NULL);
		break;

	default:
		break;
	}

	return TRUE;
}


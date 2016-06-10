#include "common.h"
#include "cam_encoding.h"

gpointer
create_video_audio_bin(gpointer data)
{
	struct record_config *priv = (struct record_config *) data;
	GstElement *bin;
	GstElement *camera, *video_enc, *video_cov;
#if 0
	GstElement *microphone, *audio_enc, *audio_cov;
#endif
	GstPad *pad;
	GstCaps *caps;
	gboolean link_ok;
	gchar *tmp;

	bin = NULL;
	
	/* Video capture part */
	if (NULL != priv->video.cam_name) {
		bin = gst_bin_new(priv->video.cam_name);

		camera = gst_element_factory_make ("v4l2src", "video_source");
		video_cov = gst_element_factory_make ("videoconvert", "video_converter");
		video_enc = gst_element_factory_make (priv->video.encoder, "video_encoder");

		if (!camera || !video_cov || !video_enc)
			return NULL;

		tmp = g_strdup_printf("/dev/%s", priv->video.cam_name);
		g_object_set(G_OBJECT(camera), "device", tmp, NULL);
		g_free(tmp);

		gst_bin_add_many(GST_BIN(bin), camera, video_cov, video_enc, NULL);
		
		caps = gst_caps_new_full (
				gst_structure_new ("video/x-raw",
				"width", G_TYPE_INT, priv->video.width,
				"height", G_TYPE_INT, priv->video.height,
				NULL),
			NULL);

		link_ok = gst_element_link_filtered(camera, video_cov, caps);
		gst_caps_unref(caps);
		if (!link_ok) {
			gst_object_unref(bin);
			GST_DEBUG("link failed %d", __LINE__);
			return NULL;
		}
		link_ok = gst_element_link(video_cov, video_enc);
		if (!link_ok) {
			gst_object_unref(bin);
			GST_DEBUG("link failed %d", __LINE__);
			return NULL;
		}

		pad = gst_element_get_static_pad(video_enc, "src");
		gst_element_add_pad (bin, gst_ghost_pad_new ("video", pad));
		gst_object_unref(GST_OBJECT(pad));

	}

	#if 0
	/* Audio capture part */
	if (NULL != priv->audio.mic_name) {
		/* only if there is no video input, audio will create a bin itself */
		if (NULL == bin)
			bin = gst_bin_new(priv->audio.mic_name);

		 mircophone = gst_element_factory_make ("alsasrc", "audio_source");
		 audio_cov = gst_element_factory_make ("audioconvert", "audio_converter");
		 audio_enc = gst_element_factory_make (priv->audio.encoder, "audio_encoder");
		
		 if (NULL == mircophone || NULL == audio_cov || NULL == audio_enc)
			 return NULL;

		g_object_set(G_OBJECT(mircophone), "device", priv->audio.mic_name, NULL);

		gst_bin_add_many(GST(bin), mircophone, audio_cov, audio_enc, NULL);

		gst_element_link(mircophone, audio_cov);
		gst_element_link(audio_cov, audio_enc);

		pad = gst_element_get_request_pad(audio_enc, "src");
		gst_element_add_pad (bin, gst_ghost_pad_new ("audio_0", pad));
		gst_object_unref(GST_OBJECT(pad));
	}
	#endif

	return bin;
}


#include "common.h"
#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>
#include "configparse.h"
#include "record.h"

GST_DEBUG_CATEGORY(shek_edr_debug);

static gpointer record_thread(gpointer data)
{
	GThread *thread;

	thread = g_thread_new("shekedr", record_pipeline, data);
	/* it won't stop the record work */
	return g_thread_join(thread);
}

static gpointer clean_up_recorder(gpointer data)
{
	gboolean it_done = FALSE;
	GValue it_value = G_VALUE_INIT;
	GstPad *sinkpad;
	GstIterator *it;
	struct record_config *priv = (struct record_config *) data;

	/* end record file first */
	it = gst_element_iterate_sink_pads(priv->r.sink);
	while (!it_done) {
		switch (gst_iterator_next (it, &it_value)) {
		case GST_ITERATOR_OK:
			sinkpad = g_value_get_object(&it_value);
			gst_pad_send_event(sinkpad, gst_event_new_eos());
			gst_object_unref(GST_OBJECT(sinkpad));
			break;
		case GST_ITERATOR_RESYNC:
			gst_iterator_resync (it);
			break;
		case GST_ITERATOR_ERROR:
		case GST_ITERATOR_DONE:
			it_done = TRUE;
			break;
		}
	}
	/* here really stop the record work */
	gst_element_set_state(priv->r.pipeline, GST_STATE_NULL);

	gst_object_unref(GST_OBJECT(priv->r.pipeline));

	g_source_remove(priv->r.bus_watch_id);

	g_free(priv->r.err_msg);
	g_free(data);
	return NULL;
}

inline static gint traverse_seq
    (GSequence * seq, GThreadFunc func) {
	GSequenceIter *begin, *current, *next;
	gint traversed = 0;
	gpointer data;

	begin = g_sequence_get_begin_iter(seq);
	current = begin;
	next = current;

	while (!g_sequence_iter_is_end(next)) {
		current = next;

		data = g_sequence_get(current);
		func(data);
		traversed++;

		next = g_sequence_iter_next(current);
	}
	return traversed;
}

int main(int argc, char *argv[])
{
	GMainLoop *loop;
	struct config_head *config;
	/* Initialisation */
	gst_init(&argc, &argv);

	GST_DEBUG_CATEGORY_INIT(shek_edr_debug, "shekedr", 0,
				"Shek EDR service");
	/* you don't need to a daemon as system service */
	loop = g_main_loop_new(NULL, FALSE);
	/* config file */
	config = config_init(NULL);
	/* signal handler*/
	g_unix_signal_add(SIGTERM, (GSourceFunc)g_main_loop_quit, loop);
	g_unix_signal_add(SIGINT, (GSourceFunc)g_main_loop_quit, loop);

	GST_LOG("starting thread");
	traverse_seq(config->record_list, record_thread);
	/* Iterate */
	g_main_loop_run(loop);
	/* Out of the main loop, clean up nicely */
	traverse_seq(config->record_list, clean_up_recorder);
	g_sequence_free(config->record_list);
	g_free(config->general);
	g_free(config);

	g_main_loop_unref(loop);

	return 0;
}

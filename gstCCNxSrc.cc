/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2012 Jiwen Cai <jwcai@cs.ucla.edu>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-ccnxsrc
 *
 * FIXME:Describe ccnxsrc here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch ccnxsrc location=song.ogg ! decodebin2 ! autoaudiosink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <gst/gst.h>

#include "gstCCNxSrc.h"

#ifndef VERSION
#define VERSION "0.01"
#endif 

GST_DEBUG_CATEGORY_STATIC (gst_ccnx_src_debug);
#define GST_CAT_DEFAULT gst_ccnx_src_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CCNX_NAME
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

static GstStaticPadTemplate src_factory = 
  GST_STATIC_PAD_TEMPLATE ("src",
                           GST_PAD_SRC,
                           GST_PAD_ALWAYS,
                           GST_STATIC_CAPS_ANY);

GST_BOILERPLATE (GstCCNxSrc, gst_ccnx_src, GstElement,
    GST_TYPE_ELEMENT);

static void gst_ccnx_src_finalize (GObject * object);

static void gst_ccnx_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_ccnx_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstCaps* gst_ccnx_src_get_caps (GstBaseSrc* basesrc);
static gboolean gst_ccnx_src_start (GstBaseSrc * basesrc);
static gboolean gst_ccnx_src_stop (GstBaseSrc * basesrc);
static gboolean gst_ccnx_src_is_seekable (GstBaseSrc * basesrc);
static gboolean gst_ccnx_src_unlock (GstBaseSrc * basesrc);
static GstFlowReturn gst_ccnx_src_create (
    GstBaseSrc * basesrc, guint64 offset, guint length, GstBuffer ** buffer);
static gboolean gst_ccnx_src_query (GstBaseSrc * src, GstQuery * query);
static gboolean gst_ccnx_src_do_seek (GstBaseSrc *src, GstSegment *segment);
static gboolean gst_ccnx_src_check_get_range (GstBaseSrc *src);
static gboolean gst_ccnx_src_unlock_stop (GstBaseSrc * src);

static gboolean gst_ccnx_src_query_duration (GstBaseSrc * src);

static void
gst_ccnx_src_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "ccnxsrc",
    "Source/Network",
    "Receives video or audio data over a CCNx network",
    "Jiwen Cai <jwcai@cs.ucla.edu>>");
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
}

/* initialize the ccnxsrc's class */
static void
gst_ccnx_src_class_init (GstCCNxSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gst_ccnx_src_set_property;
  gobject_class->get_property = gst_ccnx_src_get_property;

  g_object_class_install_property (gobject_class, PROP_CCNX_NAME,
      g_param_spec_string ("name", "CCNx Name", "Name of the video", NULL,
                           (GParamFlags) (G_PARAM_READWRITE | 
                                          G_PARAM_STATIC_STRINGS |
                                          GST_PARAM_MUTABLE_READY)));

  gobject_class->finalize = gst_ccnx_src_finalize;

  gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR(gst_ccnx_src_get_caps);
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR(gst_ccnx_src_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR(gst_ccnx_src_stop);
  gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR(gst_ccnx_src_is_seekable);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR(gst_ccnx_src_create);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR(gst_ccnx_src_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR(gst_ccnx_src_query);
  gstbasesrc_class->check_get_range = GST_DEBUG_FUNCPTR(
      gst_ccnx_src_check_get_range);
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR(gst_ccnx_src_unlock);
  gstbasesrc_class->unlock_stop = GST_DEBUG_FUNCPTR(gst_ccnx_src_unlock_stop);
}

/*
 * initialize the new element
 * initialize instance structure
 */
static void
gst_ccnx_src_init (GstCCNxSrc * src, GstCCNxSrcClass * gclass)
{
  src->mName = NULL;
  src->mDepkt = NULL;
  src->mNoLocking = FALSE;
  src->mSeeking = -1;
}

static void
gst_ccnx_src_finalize (GObject * object)
{
  GstCCNxSrc *src;
  src = GST_CCNX_SRC (object);

  g_free (src->mName);
  gst_ccnx_depkt_destroy (&src->mDepkt);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* FIXME: Some kind of member functions */
static gboolean
gst_ccnx_src_set_name (GstCCNxSrc* src, const gchar* name)
{
  GstState state;

  /* the element must be stopped in order to do this */
  GST_OBJECT_LOCK(src);
  state = GST_STATE(src);
  if (state != GST_STATE_READY && state != GST_STATE_NULL)
    goto wrong_state;
  GST_OBJECT_UNLOCK(src);

  g_free(src->mName);

  /* clear the name if we get a NULL (is that possible?) */
  if (name == NULL) {
    src->mName = NULL;
  } else {
    /* we store the filename as received by the application */
    src->mName = g_strdup(name);
    GST_INFO("name : %s", src->mName);
  }
  g_object_notify(G_OBJECT(src), "name");

  return TRUE;

  /* ERROR */
wrong_state:
  {
    g_warning ("Changing the `name' property on ccnxsrc when data is "
               "open is not supported.");
    GST_OBJECT_UNLOCK (src);
    return FALSE;
  }
}

static void
gst_ccnx_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCCNxSrc *src = GST_CCNX_SRC (object);

  switch (prop_id) {
    case PROP_CCNX_NAME:
      {
        gboolean ret = gst_ccnx_src_set_name (src, g_value_get_string(value));
        if (ret) {
          src->mDepkt = gst_ccnx_depkt_create (
              src->mName,
              GST_CCNX_DATA_WINDOW_SIZE,
              GST_CCNX_INTEREST_LIFETIME,
              GST_CCNX_INTEREST_RETRIES);
        }
      break;
      }
    default:
      {
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
      }
  }
}

static void
gst_ccnx_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCCNxSrc *src = GST_CCNX_SRC (object);

  switch (prop_id) {
    case PROP_CCNX_NAME:
      g_value_set_string (value, src->mName);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstCaps* gst_ccnx_src_get_caps (GstBaseSrc* basesrc)
{
  GstCCNxSrc *src = GST_CCNX_SRC (basesrc);
  if (src->mDepkt == NULL)
    return FALSE;

  return gst_ccnx_depkt_get_caps (src->mDepkt);
}

static gboolean
gst_ccnx_src_start (GstBaseSrc * basesrc)
{
  GstCCNxSrc *src = GST_CCNX_SRC (basesrc);
  return gst_ccnx_depkt_start (src->mDepkt);
}

static gboolean
gst_ccnx_src_stop (GstBaseSrc * basesrc)
{
  GstCCNxSrc *src = GST_CCNX_SRC (basesrc);
  return gst_ccnx_depkt_stop (src->mDepkt);
}

static gboolean
gst_ccnx_src_is_seekable (GstBaseSrc * basesrc)
{
  return TRUE;
}

static gboolean
gst_ccnx_src_unlock (GstBaseSrc * basesrc)
{
  GstCCNxSrc *src = GST_CCNX_SRC (basesrc);
  src->mNoLocking = TRUE;
  return TRUE;
}

static GstFlowReturn
gst_ccnx_src_create (
    GstBaseSrc * basesrc, guint64 offset, guint length, GstBuffer ** buffer)
{
  GstCCNxSrc *src = GST_CCNX_SRC (basesrc);
  GstBuffer *buf;
  
  if (src->mNoLocking) {
    return GST_FLOW_WRONG_STATE;
  }
  
  while (TRUE) {
    if (g_queue_is_empty (src->mDepkt->mDataQueue)) {
      if (src->mNoLocking) {
        return GST_FLOW_WRONG_STATE;
      }
      else {
        /* starving for data, sleep for a while */
        sleep(GST_CCNX_DEPKT_QUEUE_TIMEOUT);
        continue;
      }
    }

    GstCCNxDataQueueEntry * data_entry = 
        (GstCCNxDataQueueEntry *) g_queue_peek_head (src->mDepkt->mDataQueue);
    g_queue_pop_head (src->mDepkt->mDataQueue);
    // TODO release data_entry later !!!
    
    // TODO multithreading on the queue ???
    if (src->mNoLocking)
      return GST_FLOW_WRONG_STATE;

    if (src->mSeeking >= 0) {
      if (data_entry->mState != GST_CMD_SEEK) {
        /* skipping prefetched junk */
        /* ??? queue.task_done() */
        continue;
      }
      /* pushing seek buufer */
      GstEvent * event = gst_event_new_new_segment(
          FALSE, 1.0, GST_FORMAT_TIME,
          src->mSeeking, -1, src->mSeeking);
      GstPad * src_pad = gst_element_get_static_pad (
          &src->mBase.element, "src");
      gst_pad_push_event (src_pad, event);
      src->mSeeking = -1;
      buf = gst_buffer_new ();
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
      return GST_FLOW_OK;
    }
  }
  return GST_FLOW_ERROR;
}

static gboolean
gst_ccnx_src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  if (basesrc == NULL || query == NULL)
    return FALSE;

  GstCCNxSrc *src = GST_CCNX_SRC (basesrc);

  if (query->type != GST_QUERY_DURATION)
    return ((GstBaseSrcClass *) parent_class)->query (&src->mBase, query);

  gint64 duration = src->mDepkt->mDurationNs;
  gst_query_set_duration (query, GST_FORMAT_TIME, duration);

  return TRUE;
}

static gboolean
gst_ccnx_src_do_seek (GstBaseSrc *basesrc, GstSegment *segment)
{
  GstCCNxSrc *src = GST_CCNX_SRC (basesrc);

  src->mSeeking = segment->start;
  gst_ccnx_depkt_seek (src->mDepkt, segment->start);
  return TRUE;
}

static gboolean
gst_ccnx_src_check_get_range (GstBaseSrc * basesrc)
{
  return FALSE;
}

static gboolean
gst_ccnx_src_unlock_stop (GstBaseSrc * basesrc)
{
  GstCCNxSrc *src = GST_CCNX_SRC (basesrc);
  src->mNoLocking = FALSE;
  return FALSE;
}

static gboolean
gst_ccnx_src_query_duration (GstCCNxSrc * src)
{
  if (src->mDepkt)
    return gst_ccnx_depkt_check_duration_initial(src->mDepkt);

  return FALSE;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
ccnxsrc_init (GstPlugin * ccnxsrc)
{
  /* debug category for fltering log messages */
  GST_DEBUG_CATEGORY_INIT (gst_ccnx_src_debug, "ccnxsrc",
      0, "Receives video and audio data over a CCNx network");

  return gst_element_register (ccnxsrc, "ccnxsrc", GST_RANK_NONE,
      GST_TYPE_CCNX_SRC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "ccnxelement"
#endif

/* gstreamer looks for this structure to register ccnxsrcs */
GST_PLUGIN_DEFINE (
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  "ccnxsrc",
  "Receives video and audio data over a CCNx network",
  ccnxsrc_init,
  VERSION,
  "LGPL",
  "GStreamer",
  "http://gstreamer.net/"
)

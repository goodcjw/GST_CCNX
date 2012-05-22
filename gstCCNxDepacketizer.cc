/*
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

extern "C" {
#include <ccn/uri.h>
#include <ccn/charbuf.h>
}

#include "gstCCNxUtils.h"
#include "gstCCNxDepacketizer.h"

static void gst_ccnx_depkt_set_window (
    GstCCNxDepacketizer *obj, unsigned int window);
static void gst_ccnx_depkt_fetch_stream_info (GstCCNxDepacketizer *obj);
static void gst_ccnx_depkt_fetch_start_time (GstCCNxDepacketizer *obj);
static gint64 gst_ccnx_depkt_fetch_seek_query ( 
    GstCCNxDepacketizer *obj, gint64 to_seek);

static void gst_ccnx_depkt_finish_ccnx_loop (GstCCNxDepacketizer *obj);
static gboolean gst_ccnx_depkt_check_duration (GstCCNxDepacketizer *obj);

// Bellow methods are called by thread
static void * gst_ccnx_depkt_run (void *obj);
static void gst_ccnx_depkt_process_cmds (GstCCNxDepacketizer *obj);
// def duration_process_result(self, kind, info):
static enum ccn_upcall_res gst_ccnx_depkt_duration_result (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info);

static gboolean gst_ccnx_depkt_express_interest (
    GstCCNxDepacketizer *obj, gint64 seg);
static void gst_ccnx_depkt_process_response (
    GstCCNxDepacketizer *obj, ContentObject* pco);
static void gst_ccnx_depkt_push_data (
    GstCCNxDepacketizer *obj, const GstBuffer * buf);

static enum ccn_upcall_res gst_ccnx_depkt_upcall (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info);

static void gst_ccnx_depkt_num2seg (guint64 num, struct ccn_charbuf* seg);
static guint64 gst_ccnx_depkt_seg2num (const struct ccn_charbuf* seg);

static void
gst_ccnx_depkt_set_window (GstCCNxDepacketizer *obj, unsigned int window)
{
  obj->mFetchBuffer->mWindowSize = window;
}

static void
gst_ccnx_depkt_fetch_stream_info (GstCCNxDepacketizer *obj)
{
  // TODO
  // self._caps = gst.caps_from_string(co.content)
}

static void
gst_ccnx_depkt_fetch_start_time (GstCCNxDepacketizer *obj)
{
  // TODO
}

static gint64
gst_ccnx_depkt_fetch_seek_query (GstCCNxDepacketizer *obj, gint64 to_seek)
{
  // TODO
  return 0;
}

GstCaps*
gst_ccnx_depkt_get_caps (GstCCNxDepacketizer *obj)
{
  if (obj->mCaps == NULL) {
    gst_ccnx_depkt_fetch_stream_info (obj);
  }
  return obj->mCaps;
}

static void
gst_ccnx_depkt_finish_ccnx_loop (GstCCNxDepacketizer *obj)
{
  ccn_set_run_timeout (obj->mCCNx, 0);
}

static gboolean
gst_ccnx_depkt_check_duration (GstCCNxDepacketizer *obj)
{
  // TODO
  return FALSE;
}

static void*
gst_ccnx_depkt_run (void* arg)
{
  GstCCNxDepacketizer * obj = (GstCCNxDepacketizer *) arg;
  while (obj->mRunning) {
    gst_ccnx_depkt_check_duration (obj);
    ccn_run (obj->mCCNx, 10000);
    gst_ccnx_depkt_process_cmds (obj);
  }
  return NULL;
}

static void
gst_ccnx_depkt_process_cmds (GstCCNxDepacketizer *obj)
{
  if (g_queue_is_empty (obj->mCmdsQueue))
    return;

  GstCCNxCmdsQueueEntry * cmds_entry
      = (GstCCNxCmdsQueueEntry *) g_queue_pop_tail (obj->mCmdsQueue);

  if (cmds_entry->mState == GST_CMD_SEEK) {
    gint64 to_seek = gst_ccnx_depkt_fetch_seek_query (obj, to_seek);
    obj->mSeekSegment = TRUE;
    gst_ccnx_segmenter_pkt_lost (obj->mSegmenter);
    // FIXME _cmd_q.task_done ()
  }
}

static enum ccn_upcall_res
gst_ccnx_depkt_duration_result (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info)
{
  // TODO
  return CCN_UPCALL_RESULT_OK;
}

static gboolean
gst_ccnx_depkt_express_interest (GstCCNxDepacketizer *obj, gint64 seg)
{
  // TODO
  // mNameSegments
  return FALSE;
}

static void
gst_ccnx_depkt_process_response (GstCCNxDepacketizer *obj, ContentObject* pco)
{
  if (pco == NULL) {
    gst_ccnx_segmenter_pkt_lost (obj->mSegmenter);
    return;
  }
  // TODO
  gst_ccnx_segmenter_process_pkt (obj->mSegmenter, NULL); // FIXME: NULL
}

static void
gst_ccnx_depkt_push_data (GstCCNxDepacketizer *obj, const GstBuffer * buf)
{
  // TODO
}

static enum ccn_upcall_res
gst_ccnx_depkt_upcall (struct ccn_closure *selfp,
                       enum ccn_upcall_kind kind,
                       struct ccn_upcall_info *info)
{
  // TODO
  return CCN_UPCALL_RESULT_OK;
}

static guint64
gst_ccnx_depkt_seg2num (const struct ccn_charbuf* seg)
{
  guint64 num = 0;
  gst_ccnx_unpack_be_guint (&num, seg);
  return num;
}

static void
gst_ccnx_depkt_num2seg (guint64 num, struct ccn_charbuf* seg)
{
  ccn_charbuf_reset(seg);
  char aByte;
  while (num != 0) {
    aByte = num % 256;
    num = num / 256;
    ccn_charbuf_append_value(seg, (char) aByte, 1);
  }
  ccn_charbuf_append_value(seg, 0, 1);
  // reverse the buffer
  for (size_t i = 0; i < seg->length / 2; i++) {
    aByte = seg->buf[i];
    seg->buf[i] = seg->buf[seg->length - i - 1];
    seg->buf[seg->length - i - 1] = aByte;
  }
}

/* public methods */

GstCCNxDepacketizer *
gst_ccnx_depkt_create (
    const gchar *name, gint32 window_size, gint32 time_out, gint32 retries)
{
  GstCCNxDepacketizer * obj =
      (GstCCNxDepacketizer *) malloc (sizeof(GstCCNxDepacketizer));
  obj->mWindowSize = window_size;
  obj->mInterestLifetime = time_out;
  obj->mInterestRetries = retries;

  obj->mDataQueue = g_queue_new ();
  obj->mDurationNs = -1;
  obj->mRunning = FALSE;
  obj->mCaps = NULL;
  obj->mStartTime = NULL;
  obj->mSeekSegment = FALSE;
  obj->mDurationLast = -1;
  obj->mCmdsQueue = g_queue_new ();

  obj->mCCNx = ccn_create ();
  if (ccn_connect (obj->mCCNx, NULL) == -1) {
    /* LOG: cannot connect to CCN */
    gst_ccnx_depkt_destroy (&obj);
    return NULL;
  }
  
  obj->mName = ccn_charbuf_create ();
  obj->mNameSegments = ccn_charbuf_create ();
  obj->mNameFrames = ccn_charbuf_create ();
  obj->mNameStreamInfo = ccn_charbuf_create ();
  ccn_name_from_uri (obj->mName, name);
  ccn_name_from_uri (obj->mNameSegments, name);
  ccn_name_from_uri (obj->mNameSegments, "segments");
  ccn_name_from_uri (obj->mNameFrames, name);
  ccn_name_from_uri (obj->mNameFrames, "index");
  ccn_name_from_uri (obj->mNameStreamInfo, name);
  ccn_name_from_uri (obj->mNameStreamInfo, "stream_info");

  obj->mFetchBuffer = gst_ccnx_fb_create (
      obj, obj->mWindowSize,
      gst_ccnx_depkt_express_interest,
      gst_ccnx_depkt_process_response);

  obj->mSegmenter = gst_ccnx_segmenter_create (
      obj, gst_ccnx_depkt_push_data, GST_CCNX_CHUNK_SIZE);

  return obj;
}

void
gst_ccnx_depkt_destroy (GstCCNxDepacketizer ** obj)
{
  GstCCNxDepacketizer * depkt = *obj;
  GstCCNxDataQueueEntry * data_entry;
  GstCCNxCmdsQueueEntry * cmds_entry;

  if (depkt != NULL) {
    /* destroy data queue and cmds queue */
    while ((data_entry = (GstCCNxDataQueueEntry *) g_queue_pop_tail (
               depkt->mDataQueue)) != NULL)
      free (data_entry);
    while ((cmds_entry = (GstCCNxCmdsQueueEntry *) g_queue_pop_tail (
               depkt->mCmdsQueue)) != NULL)
      free (cmds_entry);

    /* destroy dynamic allocated structs */
    ccn_destroy (&depkt->mCCNx);
    // FIXME check whether this is correct
    if (depkt->mCaps != NULL)
      gst_caps_unref (depkt->mCaps);
    ccn_charbuf_destroy (&depkt->mStartTime);
    ccn_charbuf_destroy (&depkt->mName);
    ccn_charbuf_destroy (&depkt->mNameSegments);
    ccn_charbuf_destroy (&depkt->mNameFrames);
    ccn_charbuf_destroy (&depkt->mNameStreamInfo);
    gst_ccnx_fb_destroy (&depkt->mFetchBuffer);
    gst_ccnx_segmenter_destroy (&depkt->mSegmenter);
    /* destroy the object itself */
    free (depkt);
    *obj = NULL;
  }
}

gboolean
gst_ccnx_depkt_start (GstCCNxDepacketizer *obj)
{
  int rc;
  rc = pthread_create (&obj->mReceiverThread, NULL,
                       gst_ccnx_depkt_run, obj);
  if (rc != 0)
    return FALSE;

  obj->mRunning = TRUE;
  return TRUE;
}

gboolean
gst_ccnx_depkt_stop (GstCCNxDepacketizer *obj)
{
  int rc;
  obj->mRunning = FALSE;
  gst_ccnx_depkt_finish_ccnx_loop (obj);
  rc = pthread_join (obj->mReceiverThread, NULL);
  if (rc != 0)
    return FALSE;
  return TRUE;
}

gboolean
gst_ccnx_depkt_init_duration (GstCCNxDepacketizer *obj)
{
  // TODO
  return FALSE;
}

void
gst_ccnx_depkt_seek (GstCCNxDepacketizer *obj, gint64 seg_start)
{
  GstCCNxCmdsQueueEntry * cmds_entry =
      (GstCCNxCmdsQueueEntry *) malloc (sizeof(GstCCNxCmdsQueueEntry));
  cmds_entry->mState = GST_CMD_SEEK;
  cmds_entry->mTimestamp = seg_start;
  g_queue_push_head (obj->mCmdsQueue, cmds_entry);
  gst_ccnx_depkt_finish_ccnx_loop (obj);
}

/* test functions */
#ifdef GST_CCNX_TEST

guint64
test_pack_unpack (guint64 num) {
  struct ccn_charbuf* buf_1 = ccn_charbuf_create();
  gst_ccnx_depkt_num2seg(num, buf_1);
  for (int i = 0; i < buf_1->length; i++) {
    printf("\\x%02x", buf_1->buf[i]);
  }
  num = gst_ccnx_depkt_seg2num(buf_1);
  printf("\n");
  return num;
}

#endif

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
// TODO: def __check_duration(self, interest):

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
    GstCCNxDepacketizer *obj, struct ccn_charbuf *buf, ContentObject* pco);
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
  gint r;
  /* prepare return values */
  struct ccn_charbuf * resBuffer = ccn_charbuf_create ();
  struct ccn_charbuf * contentBuffer = NULL;
  ContentObject * pcoBuffer = (ContentObject *) malloc (sizeof(ContentObject));
  /* prepare name for stream_info */
  struct ccn_charbuf * nameStreamInfo = ccn_charbuf_create ();
  ccn_charbuf_append_charbuf (nameStreamInfo, obj->mName);
  ccn_name_from_uri (nameStreamInfo, "stream_info");

  r = ccn_get (obj->mCCNx, nameStreamInfo, NULL, GST_CCNX_CCN_GET_TIMEOUT,
               resBuffer, pcoBuffer, NULL, 0);
  
  if (r >= 0) {
    obj->mStartTime = gst_ccnx_utils_get_timestamp (resBuffer, pcoBuffer);
    contentBuffer = gst_ccnx_utils_get_content (resBuffer, pcoBuffer);
    obj->mCaps = gst_caps_from_string (ccn_charbuf_as_string (contentBuffer));
  }

  ccn_charbuf_destroy (&resBuffer);
  ccn_charbuf_destroy (&nameStreamInfo);
  ccn_charbuf_destroy (&contentBuffer);
  free (pcoBuffer);

  if (obj->mStartTime < 0)
    gst_ccnx_depkt_fetch_start_time (obj);
}

static void
gst_ccnx_depkt_fetch_start_time (GstCCNxDepacketizer *obj)
{
  gint r;
  /* prepare return values */
  struct ccn_charbuf * resBuffer = ccn_charbuf_create ();
  ContentObject * pcoBuffer = (ContentObject *) malloc (sizeof(ContentObject));
  /* prepare name for start time */
  struct ccn_charbuf * nameSegmentZero = ccn_charbuf_create ();
  ccn_charbuf_append_charbuf (nameSegmentZero, obj->mName);
  ccn_name_from_uri (nameSegmentZero, "segments");
  ccn_name_from_uri (nameSegmentZero, "%%00");

  r = ccn_get (obj->mCCNx, nameSegmentZero, NULL, GST_CCNX_CCN_GET_TIMEOUT,
               resBuffer, pcoBuffer, NULL, 0);

  if (r >= 0)
    obj->mStartTime = gst_ccnx_utils_get_timestamp (resBuffer, pcoBuffer);
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
  gint64 to_seek = 0;
  GstCCNxCmdsQueueEntry * cmds_entry = NULL;

  if (g_queue_is_empty (obj->mCmdsQueue))
    return;
  
  cmds_entry = (GstCCNxCmdsQueueEntry *) g_queue_pop_tail (obj->mCmdsQueue);

  if (cmds_entry->mState == GST_CMD_SEEK) {
    to_seek = gst_ccnx_depkt_fetch_seek_query (obj, to_seek);
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

  GstCCNxDepacketizer * obj = (GstCCNxDepacketizer *) selfp->data;
  struct ccn_charbuf * name;
  size_t len, last;

  if (kind == CCN_UPCALL_FINAL)
    return CCN_UPCALL_RESULT_OK;
  if (kind == CCN_UPCALL_CONTENT_UNVERIFIED)
    return CCN_UPCALL_RESULT_VERIFY;
  if (kind != CCN_UPCALL_CONTENT && kind != CCN_UPCALL_INTEREST_TIMED_OUT)
    return CCN_UPCALL_RESULT_ERR;

  if (kind == CCN_UPCALL_CONTENT) {
    len = info->pco->offset[CCN_PCO_B_Name] - info->pco->offset[CCN_PCO_B_Name];
    name = ccn_charbuf_create ();
    obj->mDurationLast = ccn_charbuf_create ();
    ccn_uri_append (name, info->content_ccnb, len, 0);
    for (last = name->length - 1; name->buf[last] != '/'; last--);
    ccn_charbuf_append (obj->mDurationLast,
                        &name->buf[last + 1],
                        name->length - last);
    ccn_charbuf_destroy (&name);
  }

  if (obj->mDurationLast != NULL)
    obj->mDurationNs = gst_ccnx_depkt_seg2num (obj->mDurationLast);
  else
    obj->mDurationNs = 0;

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
gst_ccnx_depkt_process_response (
    GstCCNxDepacketizer *obj, struct ccn_charbuf * buf, ContentObject * pco)
{
  /* when this function returns, buf and pco will be released immediately */
  struct ccn_charbuf * content;
  if (pco == NULL) {
    gst_ccnx_segmenter_pkt_lost (obj->mSegmenter);
    return;
  }
  content = gst_ccnx_utils_get_content (buf, pco);
  /* now we get the content, hopefully, we don't need buf and pco anymore */
  gst_ccnx_segmenter_process_pkt (obj->mSegmenter, content);
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
  size_t i;
  guint64 num = 0;

  if (seg->length % 3 != 0)
    return 0;

  for (i = 0; i < seg->length / 3; i++) {
    if (seg->buf[3*i] != '%')
      return 0;
    num = num * 256 
        + (gst_ccnx_utils_hexit(seg->buf[3*i+1])) * 16
        + (gst_ccnx_utils_hexit(seg->buf[3*i+2]));
  }

  return num;
}

static void
gst_ccnx_depkt_num2seg (guint64 num, struct ccn_charbuf* seg)
{
  size_t i;
  char aByte;
  struct ccn_charbuf * tmp;

  tmp = ccn_charbuf_create ();
  while (num != 0) {
    aByte = num % 256;
    num = num / 256;
    ccn_charbuf_append_value(tmp, (char) aByte, 1);
  }
  ccn_charbuf_append_value(tmp, 0, 1);
  // reverse the buffer
  for (i = 0; i < tmp->length / 2; i++) {
    aByte = tmp->buf[i];
    tmp->buf[i] = tmp->buf[tmp->length - i - 1];
    tmp->buf[tmp->length - i - 1] = aByte;
  }

  ccn_charbuf_reset (seg);
  for (i = 0; i < tmp->length; i++) {
    ccn_charbuf_putf (seg, "%%%02X", tmp->buf[i]);
  }

  ccn_charbuf_destroy (&tmp);
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
  obj->mStartTime = -1;
  obj->mSeekSegment = FALSE;
  obj->mDurationLast = NULL;
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
  ccn_name_from_uri (obj->mName, name);
  ccn_name_from_uri (obj->mNameSegments, name);
  ccn_name_from_uri (obj->mNameSegments, "segments");
  ccn_name_from_uri (obj->mNameFrames, name);
  ccn_name_from_uri (obj->mNameFrames, "index");

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
    ccn_charbuf_destroy (&depkt->mName);
    ccn_charbuf_destroy (&depkt->mNameSegments);
    ccn_charbuf_destroy (&depkt->mNameFrames);
    ccn_charbuf_destroy (&depkt->mDurationLast);
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
  fwrite (buf_1->buf, buf_1->length, 1, stdout);
  num = gst_ccnx_depkt_seg2num(buf_1);
  printf("\t%d\n", num);
  return num;
}

#endif

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

#include "gstCCNxDepacketizer.h"

static void gst_ccnx_depkt_set_window (
    GstCCNxDepacketizer *object, unsigned int window);
static void gst_ccnx_depkt_fetch_stream_info (GstCCNxDepacketizer *object);
static void gst_ccnx_depkt_fetch_start_time (GstCCNxDepacketizer *object);

static void gst_ccnx_depkt_finish_ccnx_loop (GstCCNxDepacketizer *object);
// TODO 
// static void gst_ccnx_depkt_check_duration(interest);

// Bellow methods are called by thread
static void gst_ccnx_depkt_run (GstCCNxDepacketizer *object);
static void gst_ccnx_depkt_process_cmds (GstCCNxDepacketizer *object);
// def fetch_seek_query(self, ns):
// def duration_process_result(self, kind, info):
static enum ccn_upcall_res gst_ccnx_depkt_duration_result (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info);

// def check_duration(self):

static gboolean
gst_ccnx_depkt_express_interest (GstCCNxDepacketizer *object, const char* seg);
static void gst_ccnx_depkt_process_response (GstCCNxDepacketizer *object);
static void gst_ccnx_depkt_push_data (
    GstCCNxDepacketizer *object, const struct ccn_charbuf* buf);

static enum ccn_upcall_res gst_ccnx_depkt_upcall (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info);

// def get_status(self): ?? just for debugging?
static void gst_ccnx_depkt_num2seg (guint64 num, struct ccn_charbuf* seg);
static guint64 gst_ccnx_depkt_seg2num (const struct ccn_charbuf* seg);

static void
gst_ccnx_depkt_set_window (GstCCNxDepacketizer *object, unsigned int window)
{
  // TODO
  //  object->mPipeline = 
}

static void
gst_ccnx_depkt_fetch_stream_info (GstCCNxDepacketizer *object)
{
  // TODO
}

static void
gst_ccnx_depkt_fetch_start_time (GstCCNxDepacketizer *object)
{
  // TODO
}

GstCaps*
gst_ccnx_depkt_get_caps (GstCCNxDepacketizer *object)
{
  // TODO
  return NULL;
}

static void
gst_ccnx_depkt_finish_ccnx_loop (GstCCNxDepacketizer *object)
{
  // TODO
}

void
gst_ccnx_depkt_seek (GstCCNxDepacketizer *object, gint64 seg_start)
{
  // TODO
}

gboolean
gst_ccnx_depkt_check_duration_initial(GstCCNxDepacketizer *object)
{
  // TODO
  if (object == NULL)
    return FALSE;
  return FALSE;
}

/* public methods */

GstCCNxDepacketizer *
gst_ccnx_depkt_create (
    const gchar *name, gint32 window_size, gint32 time_out, gint32 retries)
{
  GstCCNxDepacketizer * object =
      (GstCCNxDepacketizer *) malloc (sizeof(GstCCNxDepacketizer));
  object->mWindowSize = window_size;
  object->mInterestLifetime = time_out;
  object->mInterestRetries = retries;

  object->mDataQueue = new queue<struct ccn_charbuf*>();
  object->mDurationNs = -1;
  object->mRunning = FALSE;
  object->mCaps = NULL;
  object->mStartTime = NULL;
  object->mSeekSegment = FALSE;
  object->mDurationLast = -1;
  object->mCmdQueue = new queue<gint64>();

  object->mCCNx = ccn_create ();
  if (ccn_connect (object->mCCNx, NULL) == -1) {
    // TODO log warning
    gst_ccnx_depkt_destroy (&object);
    return NULL;
  }
  
  object->mName = ccn_charbuf_create ();
  object->mNameSegments = ccn_charbuf_create ();
  object->mNameFrames = ccn_charbuf_create ();
  ccn_name_from_uri (object->mName, name);
  ccn_name_from_uri (object->mNameSegments, name);
  ccn_name_from_uri (object->mNameSegments, "segments");
  ccn_name_from_uri (object->mNameFrames, name);
  ccn_name_from_uri (object->mNameFrames, "index");

  // TODO not GstPipeline
  object->mPipeline = (GstPipeline*) gst_pipeline_new ("");

  return object;
}

void
gst_ccnx_depkt_destroy (GstCCNxDepacketizer ** object)
{
  GstCCNxDepacketizer * depkt = *object;
  if (depkt != NULL) {
    ccn_destroy (&depkt->mCCNx);
    ccn_charbuf_destroy (&depkt->mCaps);
    ccn_charbuf_destroy (&depkt->mStartTime);
    ccn_charbuf_destroy (&depkt->mName);
    ccn_charbuf_destroy (&depkt->mNameSegments);
    ccn_charbuf_destroy (&depkt->mNameFrames);
    // TODO not GstPipeline
    gst_object_unref (depkt->mPipeline);
    gst_object_unref (depkt->mSegmenter);
    *object = NULL;
  }
}

gboolean
gst_ccnx_depkt_start (GstCCNxDepacketizer *object)
{
  if (object == NULL)
    return FALSE;
  // TODO
  return FALSE;
}

gboolean
gst_ccnx_depkt_stop (GstCCNxDepacketizer *object)
{
  if (object == NULL)
    return FALSE;
  // TODO
  return FALSE;
}

static void
gst_ccnx_depkt_run (GstCCNxDepacketizer *object)
{
  // TODO
}

static void
gst_ccnx_depkt_process_cmds (GstCCNxDepacketizer *object)
{
  // TODO
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
gst_ccnx_depkt_express_interest (GstCCNxDepacketizer *object, const char* seg)
{
  // TODO
  // mNameSegments
  return FALSE;
}

static void
gst_ccnx_depkt_process_response (GstCCNxDepacketizer *object)
{
  // TODO
}

static void
gst_ccnx_depkt_push_data (
    GstCCNxDepacketizer *object, const struct ccn_charbuf* buf)
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
  for (int i = seg->length - 1; i >= 0; i--) {
    num = seg->buf[seg->length - i - 1] + num * 256;
  }
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

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

#include <math.h>
#include <string.h>

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
    GstCCNxDepacketizer *obj, guint64 *idxNum);

static void gst_ccnx_depkt_finish_ccnx_loop (GstCCNxDepacketizer *obj);
static gboolean gst_ccnx_depkt_check_duration (GstCCNxDepacketizer *obj);
// TODO: def __check_duration(self, interest):

// Bellow methods are called by thread
static void *gst_ccnx_depkt_run (void *obj);
static void gst_ccnx_depkt_process_cmds (GstCCNxDepacketizer *obj);
// def duration_process_result(self, kind, info):
static enum ccn_upcall_res gst_ccnx_depkt_duration_result (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info);

static gboolean gst_ccnx_depkt_express_interest (
    GstCCNxDepacketizer *obj, gint64 seg);
static void gst_ccnx_depkt_process_response (
    GstCCNxDepacketizer *obj, struct ccn_charbuf *buf);
static void gst_ccnx_depkt_push_data (
    GstCCNxDepacketizer *obj, GstBuffer *buf);

static enum ccn_upcall_res gst_ccnx_depkt_upcall (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info);

static void gst_ccnx_depkt_num2seg (guint64 num, struct ccn_charbuf*seg);
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
  struct ccn_charbuf *resBuffer = ccn_charbuf_create ();
  struct ccn_charbuf *contentBuffer = NULL;
  ContentObject *pcoBuffer = (ContentObject *) malloc (sizeof(ContentObject));
  /* prepare name for stream_info */
  struct ccn_charbuf *nameStreamInfo = ccn_charbuf_create ();
  ccn_charbuf_append_charbuf (nameStreamInfo, obj->mName);
  ccn_name_from_uri (nameStreamInfo, "stream_info");

  r = ccn_get (obj->mCCNx, nameStreamInfo, NULL, GST_CCNX_CCN_GET_TIMEOUT,
               resBuffer, pcoBuffer, NULL, 0);
  
  if (r >= 0) {
    obj->mStartTime = gst_ccnx_utils_get_timestamp (resBuffer->buf, pcoBuffer);
    contentBuffer = gst_ccnx_utils_get_content (resBuffer->buf, pcoBuffer);
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
  struct ccn_charbuf *resBuffer = ccn_charbuf_create ();
  ContentObject *pcoBuffer = (ContentObject *) malloc (sizeof(ContentObject));
  /* prepare name for start time */
  struct ccn_charbuf *nameSegmentZero = ccn_charbuf_create ();
  ccn_charbuf_append_charbuf (nameSegmentZero, obj->mName);
  ccn_name_from_uri (nameSegmentZero, "segments");
  ccn_name_from_uri (nameSegmentZero, "%%00");

  r = ccn_get (obj->mCCNx, nameSegmentZero, NULL, GST_CCNX_CCN_GET_TIMEOUT,
               resBuffer, pcoBuffer, NULL, 0);

  if (r >= 0)
    obj->mStartTime = gst_ccnx_utils_get_timestamp (resBuffer->buf, pcoBuffer);
}

static gint64
gst_ccnx_depkt_fetch_seek_query (GstCCNxDepacketizer *obj, guint64 *idxNum)
{
  gint r;
  guint64 segNum;
  struct ccn_charbuf *idxName = NULL;
  struct ccn_charbuf *comp = NULL;  /* used to represent an entry to exclude */
  struct ccn_charbuf *templ = NULL;
  struct ccn_charbuf *retName = NULL;
  struct ccn_charbuf *retBuffer = NULL;
  struct ccn_charbuf *contentBuffer = NULL;
  ContentObject *pcoBuffer = NULL;

  templ = ccn_charbuf_create ();
  /* <Interest>           */
  ccn_charbuf_append_tt (templ, CCN_DTAG_Interest, CCN_DTAG);

  /* <Name>               */
  ccn_charbuf_append_tt (templ, CCN_DTAG_Name, CCN_DTAG);
  ccn_charbuf_append_closer (templ);
  /* </Name>              */
  
  /* <Exclude>            */
  ccn_charbuf_append_tt (templ, CCN_DTAG_Exclude, CCN_DTAG);
  /* prepare the name to be excluded, seek until the given timestamp */
  idxName = ccn_charbuf_create ();
  comp = ccn_charbuf_create ();
  gst_ccnx_depkt_num2seg (*idxNum + 1, idxName);
  ccn_name_init (comp);
  ccn_name_from_uri (comp, ccn_charbuf_as_string (idxName));
  ccn_charbuf_append(templ, comp->buf + 1, comp->length - 2);
  gst_ccnx_utils_append_exclude_any (templ);
  ccn_charbuf_destroy (&comp);
  ccn_charbuf_destroy (&idxName);
  ccn_charbuf_append_closer (templ);
  /* </Exclude>           */

  /* <ChildSelector>      */
  ccnb_tagged_putf(templ, CCN_DTAG_ChildSelector, "1");
  /* </ChildSelector>     */

  /* <AnswerOriginKind>   */
  ccn_charbuf_append_tt(templ, CCN_DTAG_AnswerOriginKind, CCN_DTAG);
  ccnb_append_number(templ, 0);
  ccn_charbuf_append_closer(templ);
  /* </AnswerOriginKind>  */

  ccn_charbuf_append_closer (templ);
  /* </Interest>          */

  /* prepare return bufferes */
  retBuffer = ccn_charbuf_create ();
  pcoBuffer = (ContentObject *) malloc (sizeof (ContentObject));

  while (TRUE) {
    /* FIXME: daddy, i want the candy, or i'll never give up asking */
    r = ccn_get (obj->mCCNx, obj->mNameFrames, templ, GST_CCNX_CCN_GET_TIMEOUT,
                 retBuffer, pcoBuffer, NULL, 0);
    if (r >= 0)
      break;
  }

  /* get the real index returned with content */
  retName = gst_ccnx_utils_get_content_name (retBuffer->buf, pcoBuffer);
  idxName = gst_ccnx_utils_get_last_comp_from_name (retName);

  /* get the content of this packet as segNum */
  contentBuffer = gst_ccnx_utils_get_content (retBuffer->buf, pcoBuffer);
  if (contentBuffer != NULL && contentBuffer->length > 0) {
    segNum = strtoul (ccn_charbuf_as_string (contentBuffer), NULL, 10);
  }

  ccn_charbuf_destroy (&templ);
  ccn_charbuf_destroy (&retName);
  ccn_charbuf_destroy (&idxName);
  ccn_charbuf_destroy (&retBuffer);
  ccn_charbuf_destroy (&contentBuffer);
  if (pcoBuffer != NULL)
    free (pcoBuffer);
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
gst_ccnx_depkt_run (void *arg)
{
  GstCCNxDepacketizer *obj = (GstCCNxDepacketizer *) arg;
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
  guint64 idxNum = 0;
  guint64 segNum = 0;
  GstCCNxCmdsQueueEntry *cmds_entry = NULL;

  if (g_queue_is_empty (obj->mCmdsQueue))
    return;
  
  cmds_entry = (GstCCNxCmdsQueueEntry *) g_queue_pop_tail (obj->mCmdsQueue);

  if (cmds_entry->mState == GST_CMD_SEEK) {
    idxNum = cmds_entry->mTimestamp;
    segNum = gst_ccnx_depkt_fetch_seek_query (obj, &idxNum);
    obj->mSeekSegment = TRUE;
    gst_ccnx_segmenter_pkt_lost (obj->mSegmenter);
    gst_ccnx_fb_reset (obj->mFetchBuffer, segNum);
    // FIXME _cmd_q.task_done ()
  }
}

static enum ccn_upcall_res
gst_ccnx_depkt_duration_result (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info)
{

  GstCCNxDepacketizer *obj = (GstCCNxDepacketizer *) selfp->data;
  struct ccn_charbuf *name;
  size_t last;

  if (kind == CCN_UPCALL_FINAL)
    return CCN_UPCALL_RESULT_OK;
  if (kind == CCN_UPCALL_CONTENT_UNVERIFIED)
    return CCN_UPCALL_RESULT_VERIFY;
  if (kind != CCN_UPCALL_CONTENT && kind != CCN_UPCALL_INTEREST_TIMED_OUT)
    return CCN_UPCALL_RESULT_ERR;

  if (kind == CCN_UPCALL_CONTENT) {
    /* get the name of returned content and store its last component */
    name = gst_ccnx_utils_get_content_name (info->content_ccnb, info->pco);
    obj->mDurationLast = gst_ccnx_utils_get_last_comp_from_name (name);
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
  struct ccn_charbuf *name = NULL;
  struct ccn_charbuf *segName = NULL;
  struct ccn_charbuf *templ = NULL;
  struct ccn_closure *callback = NULL;

  char *key = NULL;
  GstCCNxRetryEntry * entry =
      (GstCCNxRetryEntry *) malloc (sizeof(GstCCNxRetryEntry));

  name = ccn_charbuf_create ();
  ccn_charbuf_append_charbuf (name, obj->mNameSegments);
  segName = ccn_charbuf_create ();
  gst_ccnx_depkt_num2seg (seg, segName);
  ccn_name_from_uri (name, ccn_charbuf_as_string (segName));

  entry->mRetryCnt = obj->mRetryCnt;
  gst_ccnx_utils_get_current_time (&entry->mTimeVal);
  key = strdup (ccn_charbuf_as_string (segName));
  g_hash_table_insert (obj->mRetryTable, key, entry);

  templ = ccn_charbuf_create ();
  /* <Interest>           */
  ccn_charbuf_append_tt (templ, CCN_DTAG_Interest, CCN_DTAG);

  /* <Name>               */
  ccn_charbuf_append_tt (templ, CCN_DTAG_Name, CCN_DTAG);
  ccn_charbuf_append_closer (templ);
  /* </Name>              */

  /* <Lifetime>           */
  unsigned char buf[3] = { 0 };
  guint32 lifetime_l12 = obj->mInterestLifetime;
  int i;
  for (i = sizeof(buf) - 1; i >= 0; i--, lifetime_l12 >>= 8)
    buf[i] = lifetime_l12 & 0xff;
  ccnb_append_tagged_blob (templ, CCN_DTAG_InterestLifetime, buf, sizeof(buf));
  /* </Lifetime>          */

  ccn_charbuf_append_closer (templ);
  /* </Interest>          */

  callback = (struct ccn_closure*) calloc (1, sizeof(struct ccn_closure));
  callback->data = obj;
  callback->p = &gst_ccnx_depkt_upcall;
  ccn_express_interest (obj->mCCNx, name, callback, templ);

  ccn_charbuf_destroy (&name);
  ccn_charbuf_destroy (&segName);
  ccn_charbuf_destroy (&templ);

  return TRUE;
}

static void
gst_ccnx_depkt_process_response (
    GstCCNxDepacketizer *obj, struct ccn_charbuf *content)
{
  /* when this function returns, content will be released immediately */
  if (content == NULL) {
    gst_ccnx_segmenter_pkt_lost (obj->mSegmenter);
    return;
  }
  // now we get the content, we don't need buf and pco anymore,
  // content as a ccn_charbuf will be freed in segmenter's queue management.
  // FIXME content might be released...
  gst_ccnx_segmenter_process_pkt (obj->mSegmenter, content);
}

static void
gst_ccnx_depkt_push_data (GstCCNxDepacketizer *obj, GstBuffer *buf)
{
  GstCCNxCmd status = GST_CMD_INVALID;
  gint32 queueSize;
  GstCCNxDataQueueEntry *entry;

  if (obj->mSeekSegment == TRUE) {
    status = GST_CMD_SEEK;
    obj->mSeekSegment = FALSE;
  }

  while (TRUE) {
    /* FIXME this shall be run in a seperate thread ? */
    queueSize = g_queue_get_length (obj->mDataQueue);
    if (queueSize < obj->mWindowSize * 2) {
      entry = (GstCCNxDataQueueEntry *) malloc (sizeof(GstCCNxDataQueueEntry));
      entry->mState = status;
      entry->mData = buf;
      g_queue_push_head (obj->mDataQueue, entry);
      break;
    }
    else {
      /* the queue is full */
      if (obj->mRunning == FALSE)
        break;
    }
  }
}

static enum ccn_upcall_res
gst_ccnx_depkt_upcall (struct ccn_closure *selfp,
                       enum ccn_upcall_kind kind,
                       struct ccn_upcall_info *info)
{
  GstCCNxDepacketizer *depkt = (GstCCNxDepacketizer *) selfp->data;

  if (depkt->mRunning) {
    return CCN_UPCALL_RESULT_OK;
  }
  else if (kind == CCN_UPCALL_FINAL) {
    return CCN_UPCALL_RESULT_OK;
  }
  else if (kind == CCN_UPCALL_CONTENT) {
    struct ccn_charbuf *intName = NULL;    /* interest name */
    struct ccn_charbuf *conName = NULL;    /* content name */
    /* segName and segStr are used to look up as key in mRetryTable */
    struct ccn_charbuf *segName = NULL;
    struct ccn_charbuf *content = NULL;

    char *segStr = NULL;
    gdouble now, rtt, diff, absdiff;
    GstCCNxRetryEntry *entry = NULL;

    intName = gst_ccnx_utils_get_interest_name (info->interest_ccnb, info->pi);
    segName = gst_ccnx_utils_get_last_comp_from_name (intName);
    segStr = ccn_charbuf_as_string (segName);
    ccn_charbuf_destroy (&intName);
    ccn_charbuf_destroy (&segName);

    /* lookup the sending time */
    entry = (GstCCNxRetryEntry*) g_hash_table_lookup (
        depkt->mRetryTable, segStr);

    if (entry == NULL)
      return CCN_UPCALL_RESULT_ERR;  /* this should never happen */

    gst_ccnx_utils_get_current_time (&now);
    rtt = now - entry->mTimeVal;
    diff = rtt - depkt->mSRtt;
    absdiff = (diff > 0) ? diff : -diff;
    depkt->mSRtt += 1 / 128.0 * diff;
    depkt->mRttVar += 1 / 64.0 * (absdiff - depkt->mRttVar);
    depkt->mInterestLifetime = depkt->mSRtt + 3 * sqrt (depkt->mRttVar);

    g_hash_table_remove (depkt->mRetryTable, segStr);
    free (segStr);

    /* now we buffer the content */
    conName = gst_ccnx_utils_get_interest_name (info->interest_ccnb, info->pi);
    segName = gst_ccnx_utils_get_last_comp_from_name (conName);

    content = gst_ccnx_utils_get_content (info->content_ccnb, info->pco);
    gst_ccnx_fb_put (
        depkt->mFetchBuffer, gst_ccnx_depkt_seg2num (segName), content);

    // TODO
    return CCN_UPCALL_RESULT_OK;
  }
  else if (kind == CCN_UPCALL_INTEREST_TIMED_OUT) {
    // TOOD
    return CCN_UPCALL_RESULT_OK;
  }
  else if (kind == CCN_UPCALL_CONTENT_UNVERIFIED) {
    /* DEBUG: unverified content */
    return CCN_UPCALL_RESULT_VERIFY;
  }
  
  /* DEBUG: Got unknown kind */
  return CCN_UPCALL_RESULT_ERR;
}

static guint64
gst_ccnx_depkt_seg2num (const struct ccn_charbuf *seg)
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
gst_ccnx_depkt_num2seg (guint64 num, struct ccn_charbuf *seg)
{
  size_t i;
  char aByte;
  struct ccn_charbuf *tmp;

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
    const gchar *name, gint32 window_size, guint32 time_out, gint32 retries)
{
  GstCCNxDepacketizer *obj =
      (GstCCNxDepacketizer *) malloc (sizeof(GstCCNxDepacketizer));
  obj->mWindowSize = window_size;
  obj->mInterestLifetime = time_out;
  obj->mRetryCnt = retries;

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

  obj->mRetryTable = 
      g_hash_table_new_full (g_str_hash, g_str_equal, free, free);

  obj->mSRtt = 0.05;
  obj->mRttVar = 0.01;

  return obj;
}

void
gst_ccnx_depkt_destroy (GstCCNxDepacketizer **obj)
{
  GstCCNxDepacketizer *depkt = *obj;
  GstCCNxDataQueueEntry *data_entry;
  GstCCNxCmdsQueueEntry *cmds_entry;

  if (depkt != NULL) {
    /* destroy data queue and cmds queue */
    while ((data_entry = (GstCCNxDataQueueEntry *) g_queue_pop_tail (
               depkt->mDataQueue)) != NULL) {
      gst_buffer_unref (data_entry->mData);
      free (data_entry);
    }
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
    g_hash_table_destroy (depkt->mRetryTable);

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
  GstCCNxCmdsQueueEntry *cmds_entry =
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
  struct ccn_charbuf *buf_1 = ccn_charbuf_create ();
  gst_ccnx_depkt_num2seg(num, buf_1);
  fwrite (buf_1->buf, buf_1->length, 1, stdout);
  num = gst_ccnx_depkt_seg2num (buf_1);
  printf("\t%lu\n", num);
  return num;
}

#endif

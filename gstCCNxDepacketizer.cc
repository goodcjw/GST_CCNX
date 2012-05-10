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

#include "gstCCNxDepacketizer.h"

static void gst_ccnx_depkt_set_window (
    GstCCNxDepacketizer *object, unsigned int window);
static void gst_ccnx_depkt_fetch_stream_info (GstCCNxDepacketizer *object);
static void gst_ccnx_depkt_fetch_start_time (GstCCNxDepacketizer *object);

static void gst_ccnx_depkt_finish_ccnx_loop (GstCCNxDepacketizer *object);
static void gst_ccnx_depkt_seek (GstCCNxDepacketizer *object);
// TODO 
// static void gst_ccnx_depkt_check_duration(interest);
// static void gst_ccnx_depkt_check_duration_initial(GstCCNxDepacketizer *object);

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
    GstCCNxDepacketizer *object, const char* buf);

static enum ccn_upcall_res gst_ccnx_depkt_upcall (
    struct ccn_closure *selfp,
    enum ccn_upcall_kind kind,
    struct ccn_upcall_info *info);

// def get_status(self): ?? just for debugging?
static void gst_ccnx_depkt_num2seg (guint64 num, struct ccn_charbuf* seg);

static void
gst_ccnx_depkt_set_window (GstCCNxDepacketizer *object, unsigned int window)
{
}

static void
gst_ccnx_depkt_fetch_stream_info (GstCCNxDepacketizer *object)
{
}

static void
gst_ccnx_depkt_fetch_start_time (GstCCNxDepacketizer *object)
{
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
}

static void
gst_ccnx_depkt_seek (GstCCNxDepacketizer *object)
{
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
}

static void
gst_ccnx_depkt_process_cmds (GstCCNxDepacketizer *object)
{
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
}

static void
gst_ccnx_depkt_push_data (GstCCNxDepacketizer *object, const char* buf)
{
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
  for (int i = 0; i < seg->length / 2; i++) {
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

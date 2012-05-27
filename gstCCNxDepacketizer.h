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

#ifndef __GST_CCNX_DEPACKETIZER_H__
#define __GST_CCNX_DEPACKETIZER_H__

#include <pthread.h>

extern "C" {
#include <ccn/ccn.h>
}
#include <gst/gst.h>

#include "gstCCNxFetchBuffer.h"
#include "gstCCNxSegmenter.h"

const gint32 GST_CCNX_DATA_WINDOW_SIZE = 18;
const gint32 GST_CCNX_CMD_WINDOW_SIZE = 2;
const gint32 GST_CCNX_INTEREST_LIFETIME = 4096;
const gint32 GST_CCNX_INTEREST_RETRIES = 1; 
const gint32 GST_CCNX_CHUNK_SIZE = 3900;
const gint32 GST_CCNX_FRESHNESS = 1800;           /* 30 * 60 seconds */
const gint32 GST_CCNX_DEPKT_QUEUE_TIMEOUT = 1;    /* in second */
const gint32 GST_CCNX_CCN_GET_TIMEOUT = 1000;     /* 1000 ms */

typedef enum {
  GST_CMD_INVALID,
  GST_CMD_SEEK
} GstCCNxCmd;

typedef struct _GstCCNxDepacketizer GstCCNxDepacketizer;
typedef struct _GstCCNxDataQueueEntry GstCCNxDataQueueEntry;
typedef struct _GstCCNxCmdsQueueEntry GstCCNxCmdsQueueEntry;
typedef struct _GstCCNxRetryEntry GstCCNxRetryEntry;

struct _GstCCNxDataQueueEntry {
  GstCCNxCmd                     mState;
  GstBuffer                     *mData;
};

struct _GstCCNxCmdsQueueEntry {
  GstCCNxCmd                     mState;
  guint64                        mTimestamp;
};

struct _GstCCNxRetryEntry {
  gint32                         mRetryCnt;
  GTimeVal                       mTimeVal;
};

struct _GstCCNxDepacketizer {
  /* size of input window */
  gint32                         mWindowSize;
  /* in units of 1/4096 seconds */
  guint32                        mInterestLifetime;
  /* how many times to retry request */
  gint32                         mRetryCnt;
  /* input data queue */
  GQueue                        *mDataQueue;
  /* duration of the stream (in nanoseconds) */
  gint64                         mDurationNs;
  gboolean                       mRunning;
  /* ? raw content of content data ? */
  GstCaps                       *mCaps;
  /* using the signing time as video starttime, normal UNIX timestamp */
  gint32                         mStartTime;  
  gboolean                       mSeekSegment;
  /* ??? */
  struct ccn_charbuf            *mDurationLast;
  /* command queue seek in nanosecond */
  GQueue                        *mCmdsQueue;

  struct ccn                    *mCCNx;
  struct ccn_charbuf            *mName;
  struct ccn_charbuf            *mNameSegments;
  struct ccn_charbuf            *mNameFrames;

  GstCCNxFetchBuffer            *mFetchBuffer;
  GstCCNxSegmenter              *mSegmenter;

  /* self._stats = { 'srtt': 0.05, 'rttvar': 0.01 } */
  gint32                         mStatsRetries;
  gint32                         mStatsDrops;

  pthread_t                      mReceiverThread;
  GHashTable                    *mRetryTable;
  /* 
     self._tmp_retry_requests = {}
     DurationChecker = type('DurationChecker', (pyccn.Closure,),
     dict(upcall = self.duration_process_result))
     self._duration_callback = DurationChecker()
  */
};

GstCCNxDepacketizer * gst_ccnx_depkt_create (
    const gchar *name, gint32 window_size, guint32 time_out, gint32 retries);
void gst_ccnx_depkt_destroy (GstCCNxDepacketizer ** object);
gboolean gst_ccnx_depkt_start (GstCCNxDepacketizer *object);
gboolean gst_ccnx_depkt_stop (GstCCNxDepacketizer *object);
GstCaps* gst_ccnx_depkt_get_caps (GstCCNxDepacketizer *);

gboolean gst_ccnx_depkt_init_duration (GstCCNxDepacketizer *object);

void gst_ccnx_depkt_seek (GstCCNxDepacketizer *object, gint64 seg_start);

/* test functions */
guint64 test_pack_unpack (guint64 num);

#endif // __GST_CCNX_DEPACKETIZER_H__

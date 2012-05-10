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

extern "C" {
#include <ccn/ccn.h>
}

#include <gst/gst.h>

#include "gstCCNxSegmenter.h"

typedef struct _GstCCNxDepacketizer GstCCNxDepacketizer;

struct _GstCCNxDepacketizer {
  /* size of input window */
  gint32                         mWindowSize;
  /* in units of 1/4096 seconds */
  gint32                         mInterestLifetime;
  /* how many times to retry request */
  gint32                         mInterestRetries; 
  // TODO shall we have a queue inside gstCCNxSrc? for synchronization?
  /* duration of the stream (in nanoseconds) */
  gint64                         mDurationNs;
  gboolean                       mRunning;
  /* ? raw content of content data ? */
  const char                    *mCaps;
  /* using the signing time as video starttime */
  const struct ccn_charbuf      *mStartTime;  
  gboolean                       mSeekSegment;
  /* ??? */
  gint64                         mDurationLast;
  // TODO shall we have a queue for command?

  struct ccn                    *mCCNx;
  const char                    *mName;
  const char                    *mNameSegments;
  const char                    *mNameFrames;

  GstPipeline                   *mPipeline;
  GstCCNxSegmenter              *mSegmenter;

  /* self._stats = { 'srtt': 0.05, 'rttvar': 0.01 } */
  gint32                         mStatsRetries;
  gint32                         mStatsDrops;
  
  /* 
     self._tmp_retry_requests = {}
     DurationChecker = type('DurationChecker', (pyccn.Closure,),
     dict(upcall = self.duration_process_result))
     self._duration_callback = DurationChecker()
  */
};

gboolean gst_ccnx_depkt_start (GstCCNxDepacketizer *object);
gboolean gst_ccnx_depkt_stop (GstCCNxDepacketizer *object);
GstCaps* gst_ccnx_depkt_get_caps (GstCCNxDepacketizer *);

gboolean gst_ccnx_depkt_check_duration_initial (GstCCNxDepacketizer *object);
void gst_ccnx_depkt_seek (GstCCNxDepacketizer *object, gint64 seg_start);

/* test functions */
guint64 test_pack_unpack (guint64 num);

#endif // __GST_CCNX_DEPACKETIZER_H__

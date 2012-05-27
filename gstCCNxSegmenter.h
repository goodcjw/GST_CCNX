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

#ifndef __GST_CCNX_SEGMENTER_H__
#define __GST_CCNX_SEGMENTER_H__

#include <gst/gst.h>
extern "C" {
#include <ccn/ccn.h>
}

const guint32 GST_CCNX_PACKET_HDR_LEN = 4;
const guint32 GST_CCNX_SEGMENT_HDR_LEN = 20;

struct _GstCCNxDepacketizer;

typedef struct _GstCCNxSegmenter GstCCNxSegmenter;
typedef struct _GstCCNxDepacketizer GstCCNxDepacketizer; 
typedef struct _GstCCNxPacketHeader GstCCNxPacketHeader;
typedef struct _GstCCNxSegmentHeader GstCCNxSegmentHeader;

typedef void (*gst_ccnx_seg_callback) (
    GstCCNxDepacketizer *obj, GstBuffer * buf);

struct _GstCCNxSegmenter {
  GstCCNxDepacketizer          *mDepkt;
  gst_ccnx_seg_callback         mCallback;
  guint32                       mMaxSize;
  struct ccn_charbuf           *mPktContent;
  guint32                       mPktElements;
  guint32                       mPktElementOff;
  gboolean                      mPktLost;
};

struct _GstCCNxPacketHeader {
  guint8                        mLeft;
  guint16                       mOffset;
  guint8                        mCount;
};

struct _GstCCNxSegmentHeader {
  guint32                       mSize;
  guint64                       mTimestamp;
  guint64                       mDuration;
};

GstCCNxSegmenter * gst_ccnx_segmenter_create (
    GstCCNxDepacketizer *       depkt,
    gst_ccnx_seg_callback       func,
    guint32                     max_size);

void gst_ccnx_segmenter_destroy (GstCCNxSegmenter ** obj);

void gst_ccnx_segmenter_pkt_lost (GstCCNxSegmenter* obj);

void gst_ccnx_segmenter_process_pkt (
    GstCCNxSegmenter*           obj, 
    const struct ccn_charbuf*   pkt_buffer);

#endif // __GST_CCNX_SEGMENTER_H__

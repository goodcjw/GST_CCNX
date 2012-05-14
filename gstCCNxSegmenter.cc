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

#include "gstCCNxSegmenter.h"


/* convertor bettwen GstBuffer and CCNx segment */
GstCCNxPacketHeader* gst_ccnx_packet_header_unpack (const struct ccn_charbuf*);
struct ccn_charbuf* gst_ccnx_segmenter_from_buffer (const GstBuffer*);
GstBuffer* gst_ccnx_segmenter_to_buffer(
    const struct ccn_charbuf * segment, gint32 offset);

GstCCNxPacketHeader * 
gst_ccnx_packet_header_unpack (const struct ccn_charbuf * header_buf)
{
  GstCCNxPacketHeader * header =
      (GstCCNxPacketHeader *) malloc (sizeof(GstCCNxPacketHeader));
  header->mLeft = (guint8) header_buf->buf[0];
  header->mOffset = (guint16) header_buf->buf[1] * 256
      + (guint16) header_buf->buf[2];
  header->mCount = (guint8) header_buf->buf[2];
  return header;
}

GstCCNxSegmentHeader *
gst_ccnx_segment_header_unpack (const struct ccn_charbuf * header_buf)
{
  GstCCNxSegmentHeader * header =
      (GstCCNxSegmentHeader *) malloc (sizeof(GstCCNxSegmentHeader));
  
  return header;
}

void gst_ccnx_segmenter_process_buffer (
    GstCCNxSegmenter* object,
    const GstBuffer* buffer, 
    gboolean start_fresh,
    gboolean flush);

void gst_ccnx_segmenter_process_buffer_split (
    GstCCNxSegmenter* object,
    const GstBuffer*);

void gst_ccnx_segmenter_pkt_lost (GstCCNxSegmenter* object);
void gst_ccnx_segmenter_process_pkt (
    GstCCNxSegmenter* object, const struct ccn_charbuf* pkt_buffer);
void gst_ccnx_segmenter_send_callback (
    GstCCNxSegmenter* object, gint32 left, gint32 size);

void gst_ccnx_segmenter_pkt_lost (GstCCNxSegmenter* object)
{
  object->mPktLost = TRUE;
  
  if (object->mPktContent == NULL)
    object->mPktContent = ccn_charbuf_create();
  else
    ccn_charbuf_reset(object->mPktContent);
  
  object->mPktElements = 0;
}

struct ccn_charbuf*
gst_ccnx_segmenter_from_buffer (const GstBuffer*)
{
  // TODO
  return NULL;
}

GstBuffer*
gst_ccnx_segmenter_to_buffer(const struct ccn_charbuf* segment, gint32& offset)
{
  if (segment->length - offset < GST_CCNX_SEGMENT_HDR_LEN)
    return NULL;

  struct ccn_charbuf * segment_header = ccn_charbuf_create ();
  ccn_charbuf_append (
      segment_header, segment->buf + offset, GST_CCNX_SEGMENT_HDR_LEN);

  // TODO
  return NULL;
}

void gst_ccnx_segmenter_process_buffer (
    GstCCNxSegmenter* object,
    const GstBuffer* buffer, 
    gboolean start_fresh,
    gboolean flush)
{
  if (start_fresh && object->mPktContent->length > 0)
    gst_ccnx_segmenter_send_callback (object, 0, -1);
  // TODO
}

void gst_ccnx_segmenter_process_buffer_split (
    GstCCNxSegmenter* object,
    const GstBuffer*)
{
  // TODO
}

void gst_ccnx_segmenter_process_pkt (
    GstCCNxSegmenter* object, const struct ccn_charbuf* pkt_buffer)
{
  struct ccn_charbuf * hdr_buffer = ccn_charbuf_create ();
  ccn_charbuf_append (hdr_buffer, pkt_buffer->buf, GST_CCNX_PACKET_HDR_LEN);
  GstCCNxPacketHeader * header = gst_ccnx_packet_header_unpack (hdr_buffer);
  
  if ( !object->mPktLost || object->mPktContent->length > 0)
    header->mOffset = 0;

  header->mOffset += GST_CCNX_PACKET_HDR_LEN;
  ccn_charbuf_append (object->mPktContent, 
                      pkt_buffer->buf + header->mOffset,
                      pkt_buffer->length - header->mOffset);
  object->mPktElements += header->mCount;

  // TODO

  /* clean up memory */
  ccn_charbuf_destroy (&hdr_buffer);
  free (header);
}

void gst_ccnx_segmenter_send_callback (
    GstCCNxSegmenter* object, gint32 left, gint32 size)
{
  if (size < 0) {
    size = object->mPktContent->length;
  }
  // TODO
}

/* public methods */
GstCCNxSegmenter *
gst_ccnx_segmenter_create (
    GstCCNxDepacketizer * depkt, gst_ccnx_seg_callback func, guint32 max_size)
{
  GstCCNxSegmenter * object =
      (GstCCNxSegmenter*) malloc (sizeof(GstCCNxSegmenter));

  object->mDepkt = depkt;
  object->mCallback = func;
  object->mMaxSize = max_size - GST_CCNX_PACKET_HDR_LEN;
  object->mPktContent = ccn_charbuf_create();
  object->mPktElements = 0;
  object->mPktElementOff = 0;
  object->mPktLost = FALSE;

  return object;
}

void
gst_ccnx_segmenter_destroy (GstCCNxSegmenter ** object)
{
  GstCCNxSegmenter * seg = *object;
  if (seg != NULL) {
    /* mDepkt is just a back reference, we are not going to free it */
    seg->mDepkt = NULL;
    /* destroy dynamic allocated structs */
    ccn_charbuf_destroy (&seg->mPktContent);
    /* destroy the object itself */
    free (seg);
    *object = NULL;
  }
}

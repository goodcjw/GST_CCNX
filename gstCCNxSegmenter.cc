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

#include "gstCCNxUtils.h"
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

  gst_ccnx_unpack_be_guint (&header->mLeft, &header_buf->buf[0], 1);
  gst_ccnx_unpack_be_guint (&header->mOffset, &header_buf->buf[1], 2);
  gst_ccnx_unpack_be_guint (&header->mCount, &header_buf->buf[3], 1);

  return header;
}

GstCCNxSegmentHeader *
gst_ccnx_segment_header_unpack (const struct ccn_charbuf * header_buf)
{
  GstCCNxSegmentHeader * header =
      (GstCCNxSegmentHeader *) malloc (sizeof(GstCCNxSegmentHeader));

  gst_ccnx_unpack_be_guint (&header->mSize, &header_buf->buf[0], 4);
  gst_ccnx_unpack_be_guint (&header->mTimestamp, &header_buf->buf[4], 8);
  gst_ccnx_unpack_be_guint (&header->mDuration, &header_buf->buf[12], 8);

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
  struct ccn_charbuf * segment_hdr_buffer = NULL;
  GstCCNxSegmentHeader * segment_hdr = NULL;
  GstBuffer * ret = NULL;

  if (segment->length - offset < GST_CCNX_SEGMENT_HDR_LEN)
    return NULL;

  segment_hdr_buffer = ccn_charbuf_create ();
  ccn_charbuf_append (
      segment_hdr_buffer, segment->buf + offset, GST_CCNX_SEGMENT_HDR_LEN);
  segment_hdr = gst_ccnx_segment_header_unpack (segment_hdr_buffer);

  guint32 start = offset + GST_CCNX_SEGMENT_HDR_LEN;
  guint32 end = start + segment_hdr->mSize;

  if (end > segment->length)
    goto ret_state;
  
  ret = gst_buffer_new ();
  gst_buffer_set_data (ret, segment->buf + start, segment_hdr->mSize);
  ret->timestamp = segment_hdr->mTimestamp;
  ret->duration = segment_hdr->mDuration;

  offset = end;

ret_state:
  /* clean up memory */
  ccn_charbuf_destroy (&segment_hdr_buffer);
  if (segment_hdr)
    free (segment_hdr);
  return ret;
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
  struct ccn_charbuf * hdr_buffer
      = ccn_charbuf_create ();
  ccn_charbuf_append (hdr_buffer, pkt_buffer->buf, GST_CCNX_PACKET_HDR_LEN);
  GstCCNxPacketHeader * pkt_header
      = gst_ccnx_packet_header_unpack (hdr_buffer);

  if ( !object->mPktLost || object->mPktContent->length > 0)
    pkt_header->mOffset = 0;

  pkt_header->mOffset += GST_CCNX_PACKET_HDR_LEN;
  ccn_charbuf_append (object->mPktContent, 
                      pkt_buffer->buf + pkt_header->mOffset,
                      pkt_buffer->length - pkt_header->mOffset);
  object->mPktElements += pkt_header->mCount;

  // TODO

  /* clean up memory */
  ccn_charbuf_destroy (&hdr_buffer);
  free (pkt_header);
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

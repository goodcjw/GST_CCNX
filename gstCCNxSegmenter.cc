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
struct ccn_charbuf * gst_ccnx_segmenter_from_buffer (const GstBuffer*);
GstBuffer* gst_ccnx_segmenter_to_buffer(const struct ccn_charbuf*);

void gst_ccnx_segmenter_process_buffer (
    GstCCNxSegmenter* object,
    const GstBuffer*, 
    gboolean start_fresh,
    gboolean flush);

// Not currently in use?
// void gst_ccnx_process_buffer_split (GstCCNxSegmenter* object, const GstBuffer*);

void gst_ccnx_segmenter_pkt_lost (GstCCNxSegmenter* object);
void gst_ccnx_segmenter_process_pkt (const struct ccn_charbuf*);
void gst_ccnx_segmenter_send_callback (
    GstCCNxSegmenter* object, guint32 left, guint size);

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
gst_ccnx_segmenter_to_buffer(const struct ccn_charbuf*)
{
  // TODO
  return NULL;
}

void gst_ccnx_segmenter_process_buffer (
    GstCCNxSegmenter* object,
    const GstBuffer*, 
    gboolean start_fresh,
    gboolean flush)
{
  // TODO
}

void gst_ccnx_segmenter_process_pkt (const struct ccn_charbuf*)
{
  // TODO
}

void gst_ccnx_segmenter_send_callback (
    GstCCNxSegmenter* object, guint32 left, guint size)
{
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
  object->mMaxSize = max_size - PACKET_HDR_LEN;
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

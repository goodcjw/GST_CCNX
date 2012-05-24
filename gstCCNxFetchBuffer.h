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

#ifndef __GST_CCNX_FETCH_BUFFER_H__
#define __GST_CCNX_FETCH_BUFFER_H__

#include <gst/gst.h>

#include "gstCCNxUtils.h"

const gint64 GST_CCNX_COUNTER_STEP = 10;

struct _GstCCNxDepacketizer;

typedef struct _GstCCNxDepacketizer GstCCNxDepacketizer; 
typedef struct _GstCCNxFetchBuffer GstCCNxFetchBuffer;
typedef gboolean (*gst_ccnx_fb_request_cb) (
    GstCCNxDepacketizer *object, gint64 seg);
typedef void (*gst_ccnx_fb_response_cb) (
    GstCCNxDepacketizer *object, ContentObject* data);

struct _GstCCNxFetchBuffer {
  /* just a back reference, not created here */
  GstCCNxDepacketizer                   *mDepkt;
  /* size of input window */
  gint32                                 mWindowSize;
  gst_ccnx_fb_request_cb                 mRequester;
  gst_ccnx_fb_response_cb                mResponser;

  /* GHashTable <gint64, ContentObject*> */
  GHashTable                            *mBuffer;
  gint64                                 mPosition;
  gint64                                 mRequested;
  gint64                                 mCounter;
};

GstCCNxFetchBuffer * gst_ccnx_fb_create (
    GstCCNxDepacketizer * depkt, gint32 window,
    gst_ccnx_fb_request_cb req, gst_ccnx_fb_response_cb rep);

void gst_ccnx_fb_destroy (GstCCNxFetchBuffer ** object);

#endif // __GST_CCNX_FETCH_BUFFER_H__

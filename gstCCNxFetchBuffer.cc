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

#include "gstCCNxFetchBuffer.h"

static void gst_ccnx_fb_reset (GstCCNxFetchBuffer* object, gint64 position);
static void gst_ccnx_fb_put (
    GstCCNxFetchBuffer* object, gint64 num, ContentObject* pco);
static void gst_ccnx_fb_timeout (GstCCNxFetchBuffer* object, gint64 num);
static gint64 gst_ccnx_fb_get_position (GstCCNxFetchBuffer* object);
static gint64 gst_ccnx_fb_get_size (GstCCNxFetchBuffer* object);
static void gst_ccnx_fb_request_data (GstCCNxFetchBuffer* object);
static void gst_ccnx_fb_push_data (GstCCNxFetchBuffer* object);

static void
gst_ccnx_fb_reset (GstCCNxFetchBuffer* object, gint64 position)
{
  object->mBuffer->clear();
  object->mPosition = position;
  object->mRequested = position - 1;
  object->mCounter = 0;
}

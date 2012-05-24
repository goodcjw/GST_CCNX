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

extern "C" {
#include <ccn/charbuf.h>
}
#include "gstCCNxFetchBuffer.h"

static void gst_ccnx_fb_reset (GstCCNxFetchBuffer* object, gint64 position);
static void gst_ccnx_fb_put (GstCCNxFetchBuffer* object, gint64 num,
                             struct ccn_charbuf *buf, ContentObject * pco);
static void gst_ccnx_fb_timeout (GstCCNxFetchBuffer* object, gint64 num);
static gint64 gst_ccnx_fb_get_size (GstCCNxFetchBuffer* object);
static void gst_ccnx_fb_request_data (GstCCNxFetchBuffer* object);
static void gst_ccnx_fb_push_data (GstCCNxFetchBuffer* object);
static void gst_ccnx_fb_entry_destroy (void * object);

static void
gst_ccnx_fb_reset (GstCCNxFetchBuffer* object, gint64 position)
{
  g_hash_table_remove_all (object->mBuffer);
  object->mPosition = position;
  object->mRequested = position - 1;
  object->mCounter = 0;

  gst_ccnx_fb_request_data (object);
}

/*
 * once buf and pco been put into the buffer, we are subject to be destroyed
 * when its corresponding entry is removed from the buffer
 */
static void
gst_ccnx_fb_put (GstCCNxFetchBuffer* object, gint64 num,
                 struct ccn_charbuf *buf, ContentObject * pco)
{
  if (num >= object->mPosition) {
    gint64 * p_num = (gint64 *) malloc (sizeof(gint64));
    GstCCNxFBEntry * p_entry = 
        (GstCCNxFBEntry *) malloc (sizeof(GstCCNxFBEntry));
    *p_num = num;
    p_entry->buf = buf;
    p_entry->pco = pco;
    g_hash_table_insert (object->mBuffer, p_num, p_entry);
  }
  gst_ccnx_fb_push_data (object);
}

static void
gst_ccnx_fb_timeout (GstCCNxFetchBuffer* object, gint64 num)
{
  gst_ccnx_fb_put (object, num, NULL, NULL);
}

static gint64
gst_ccnx_fb_get_size (GstCCNxFetchBuffer* object)
{
  if (object->mPosition < 0)
    return 0;

  return object->mRequested - object->mPosition + 1;
}

static void
gst_ccnx_fb_request_data (GstCCNxFetchBuffer* object)
{
  gint32 interest_left;

  if (object->mCounter == 0)
    interest_left = 2;
  else
    interest_left = 1;

  object->mCounter = (object->mCounter + 1) % GST_CCNX_COUNTER_STEP;
  gint64 stop = object->mPosition + object->mWindowSize - 1;

  while (object->mRequested < stop && interest_left > 0) {
    object->mRequested += 1;
    interest_left -= 1;

    if (! object->mRequester (object->mDepkt, object->mRequested))
      return;
  }
}

static void
gst_ccnx_fb_push_data (GstCCNxFetchBuffer* object)
{
  GstCCNxFBEntry * entry;
  while (g_hash_table_size (object->mBuffer) != 0) {
    entry = (GstCCNxFBEntry *) g_hash_table_lookup (
        object->mBuffer, &object->mPosition);
    /* call the callback function in GstCCNxDepacketizer */
    object->mResponser (object->mDepkt, entry->buf, entry->pco);
    /* buf and pco will be release at this point */
    g_hash_table_remove (object->mBuffer, &object->mPosition);
    object->mPosition += 1;
  }

  gst_ccnx_fb_request_data (object);
}

/* public methods */
GstCCNxFetchBuffer * gst_ccnx_fb_create (
    GstCCNxDepacketizer * depkt, gint32 window,
    gst_ccnx_fb_request_cb req, gst_ccnx_fb_response_cb rep)
{
  GstCCNxFetchBuffer * object =
      (GstCCNxFetchBuffer *) malloc (sizeof(GstCCNxFetchBuffer));

  object->mDepkt = depkt;
  object->mPosition = -1;
  object->mRequested = -1;
  object->mCounter = -1;

  object->mWindowSize = window;
  object->mRequester = req;
  object->mResponser = rep;
  object->mBuffer = 
      g_hash_table_new_full (g_int_hash, g_int_equal, 
                             free, gst_ccnx_fb_entry_destroy);

  return object;
}

void
gst_ccnx_fb_destroy (GstCCNxFetchBuffer ** object)
{
  GstCCNxFetchBuffer * fb = *object;
  if (fb != NULL) {
    /* mDepkt is just a back reference, we are not going to free it */
    fb->mDepkt = NULL;
    /* destroy dynamic allocated structs */
    g_hash_table_destroy (fb->mBuffer);
    /* destroy the object itself */
    free (fb);
    *object = NULL;
  }
}

static void
gst_ccnx_fb_entry_destroy (void *obj)
{
  GstCCNxFBEntry * entry = * (GstCCNxFBEntry**) obj;

  if (entry != NULL) {
    /* FIXME shall we destroy these values here ? */
    ccn_charbuf_destroy (&entry->buf);
    if (entry->pco != NULL)
      free (entry->pco);
  }
}

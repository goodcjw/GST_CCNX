/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
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

#ifndef __GST_CCNX_SRC_H__
#define __GST_CCNX_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>

#include "gstCCNxDepacketizer.h"

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_CCNX_SRC \
  (gst_ccnx_src_get_type())
#define GST_CCNX_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CCNX_SRC,GstCCNxSrc))
#define GST_CCNX_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CCNX_SRC,GstCCNxSrcClass))
#define GST_IS_CCNX_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CCNX_SRC))
#define GST_IS_CCNX_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CCNX_SRC))

typedef struct _GstCCNxSrc      GstCCNxSrc;
typedef struct _GstCCNxSrcClass GstCCNxSrcClass;

struct _GstCCNxSrc {
  GstBaseSrc                 *mBase;

  gchar                      *mName;         /* name of the CCNx interest */
  GstCCNxDepacketizer        *mDepkt;        /* the depacketizer component */
  gboolean                    mNoLocking;
  gint64                      mSeeking;
};

struct _GstCCNxSrcClass {
  GstBaseSrcClass parent_class;
};

GType gst_ccnx_src_get_type (void);

G_END_DECLS

#endif /* __GST_CCNX_SRC_H__ */

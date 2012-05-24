#ifdef GST_CCNX_TEST
#include <assert.h>
#endif
#include <stdio.h>

#include <gst/gst.h>

#include "gstCCNxSrc.h"

int
main (int   argc,
      char *argv[])
{
  const gchar *nano_str;
  guint major, minor, micro, nano;

  gst_init (&argc, &argv);

  gst_version (&major, &minor, &micro, &nano);

  if (nano == 1)
    nano_str = "(CVS)";
  else if (nano == 2)
    nano_str = "(Prerelease)";
  else
    nano_str = "";

  printf ("This program is linked against GStreamer"" %d.%d.%d %s\n",
          major, minor, micro, nano_str);

#ifdef GST_CCNX_TEST
  for (size_t i = 0; i < 100; i++) {
    test_pack_unpack (i);
  }
  assert(10 == test_pack_unpack(10));
  assert(100 == test_pack_unpack(100));
  assert(1000 == test_pack_unpack(1000));
  assert(12345 == test_pack_unpack(12345));
  assert(987654 == test_pack_unpack(987654));
  assert(9999999 == test_pack_unpack(9999999));
  printf ("%00\n");
#endif
  return 0;
}

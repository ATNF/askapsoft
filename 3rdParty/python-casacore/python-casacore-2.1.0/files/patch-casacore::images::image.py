--- casacore/images/image.py.orig	2019-09-13 09:55:53.000000000 +0800
+++ casacore/images/image.py	2019-09-13 09:56:15.000000000 +0800
@@ -316,7 +316,7 @@
         set to False.
 
         """
-        return -self._getmask (self._adjustBlc(blc),
+        return ~self._getmask (self._adjustBlc(blc),
                                self._adjustTrc(trc),
                                self._adjustInc(inc));
 

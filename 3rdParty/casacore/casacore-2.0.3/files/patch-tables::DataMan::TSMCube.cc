--- ./tables/DataMan/TSMCube.cc-orig	2018-11-19 17:24:41.000000000 +1100
+++ ./tables/DataMan/TSMCube.cc	2018-11-19 17:26:30.000000000 +1100
@@ -749,7 +749,7 @@
                                  uInt bucketSize)
 {
     // An overdraft of 10% is allowed.
-    if (maxSize > 0  &&  cacheSize * bucketSize > maxSize) {
+    if (maxSize > 0  &&  cacheSize > maxSize / bucketSize) {
         uInt size = maxSize / bucketSize;
         if (10 * cacheSize  >  11 * size) {
             return size;

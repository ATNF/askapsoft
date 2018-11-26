--- ./tables/DataMan/TSMCube.cc-orig	2015-07-24 17:01:01.000000000 +1000
+++ ./tables/DataMan/TSMCube.cc	2018-11-23 14:27:44.000000000 +1100
@@ -186,7 +161,7 @@
         os << ">>> TSMCube cache statistics:" << endl;
         os << "cubeShape: " << cubeShape_p << endl;
         os << "tileShape: " << tileShape_p << endl;
-        os << "maxCacheSz:" << stmanPtr_p->maximumCacheSize() << endl;
+        os << "maxCacheSz:" << stmanPtr_p->maximumCacheSize() << " MiB" << endl;
         cache_p->showStatistics (os);
         os << "<<<" << endl;
     }
@@ -745,14 +720,14 @@
                             bucketSize_p);
 }
 
-uInt TSMCube::validateCacheSize (uInt cacheSize, uInt maxSize,
+uInt TSMCube::validateCacheSize (uInt cacheSize, uInt maxSizeMiB,
                                  uInt bucketSize)
 {
     // An overdraft of 10% is allowed.
-    if (maxSize > 0  &&  cacheSize * bucketSize > maxSize) {
-        uInt size = maxSize / bucketSize;
-        if (10 * cacheSize  >  11 * size) {
-            return size;
+    uInt maxnb = std::max(1u, uInt(1024. * 1024. * maxSizeMiB / bucketSize));
+    if (maxSizeMiB > 0  &&  cacheSize > maxnb) {
+        if (10 * cacheSize  >  11 * maxnb) {
+            return maxnb;
         }
     }
     return cacheSize;
@@ -782,10 +757,10 @@
 {
     uInt cacheSize = calcCacheSize (sliceShape, windowStart,
 				    windowLength, axisPath);
-    // If not userset and if the entire cube needs to be cached,
-    // do not cache if more than 20% of the memory is needed.
-    if (!userSet  &&  cacheSize >= nrTiles_p) {
-      uInt maxSize = uInt(HostInfo::memoryTotal() * 1024.*0.2 / bucketSize_p);
+    // If not userset, do not cache if more than 25% of the memory is needed.
+    if (!userSet) {
+      uInt maxSize = uInt(HostInfo::memoryTotal(True) * 1024.*0.25 /
+                          bucketSize_p);
       if (cacheSize > maxSize) {
 	cacheSize = 1;
       }

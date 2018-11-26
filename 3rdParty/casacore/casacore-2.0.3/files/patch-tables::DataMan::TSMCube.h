--- ./tables/DataMan/TSMCube.h-orig	2015-07-24 17:01:01.000000000 +1000
+++ ./tables/DataMan/TSMCube.h	2018-11-23 14:27:41.000000000 +1100
@@ -244,7 +245,7 @@
                                const IPosition& windowStart,
                                const IPosition& windowLength,
                                const IPosition& axisPath,
-                               uInt maxCacheSize, uInt bucketSize);
+                               uInt maxCacheSizeMiB, uInt bucketSize);
     // </group>
 
     // Set the cache size for the given slice and access path.
@@ -264,11 +265,12 @@
     virtual void setCacheSize (uInt cacheSize, Bool forceSmaller, Bool userSet);
 
     // Validate the cache size (in buckets).
-    // This means it will return the given cache size if smaller
-    // than the maximum cache size. Otherwise the maximum is returned.
+    // This means it will return the given cache size (in buckets) if
+    // smaller than the maximum cache size (given in MiB).
+    // Otherwise the maximum is returned.
     // <group>
     uInt validateCacheSize (uInt cacheSize) const;
-    static uInt validateCacheSize (uInt cacheSize, uInt maxSize,
+    static uInt validateCacheSize (uInt cacheSize, uInt maxSizeMiB,
                                    uInt bucketSize);
     // </group>
 

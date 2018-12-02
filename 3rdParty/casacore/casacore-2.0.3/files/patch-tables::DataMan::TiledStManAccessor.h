--- ./tables/DataMan/TiledStManAccessor.h-orig	2015-07-24 17:01:01.000000000 +1000
+++ ./tables/DataMan/TiledStManAccessor.h	2018-11-23 14:28:25.000000000 +1100
@@ -143,7 +143,7 @@
 //  // Open a table.
 //  Table table("someName.data");
 //  // Set the maximum cache size of its tiled hypercube storage
-//  // manager TSMExample to 0.5 Mb.
+//  // manager TSMExample to 0.5 MiB.
 //  ROTiledStManAccessor accessor(table, "TSMExample");
 //  accessor.setMaximumCacheSize (512*1024);
 // </srcblock>
@@ -176,16 +176,16 @@
     // Assignment (reference semantics).
     ROTiledStManAccessor& operator= (const ROTiledStManAccessor& that);
 
-    // Set the maximum cache size (in bytes) to be used by a hypercube
+    // Set the maximum cache size (in MibiByte) to be used by a hypercube
     // in the storage manager. Note that each hypercube has its own cache.
     // 0 means unlimited.
     // The initial maximum cache size is unlimited.
     // The maximum cache size given in this way is not persistent.
     // Only the maximum cache size given to the constructors of the tiled
     // storage managers, is persistent.
-    void setMaximumCacheSize (uInt nbytes);
+    void setMaximumCacheSize (uInt nMiB);
 
-    // Get the maximum cache size (in bytes).
+    // Get the maximum cache size (in MiB).
     uInt maximumCacheSize() const;
 
     // Get the current cache size (in buckets) for the hypercube in

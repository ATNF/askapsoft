--- ./tables/DataMan/TiledStMan.h-orig	2015-07-24 17:01:01.000000000 +1000
+++ ./tables/DataMan/TiledStMan.h	2018-11-23 14:28:14.000000000 +1100
@@ -111,13 +111,13 @@
     TiledStMan();
 
     // Create a TiledStMan storage manager.
-    // The given maximum cache size is persistent,
+    // The given maximum cache size (in MibiByte) is persistent,
     // thus will be reused when the table is read back. Note that the class
     // <linkto class=ROTiledStManAccessor>ROTiledStManAccessor</linkto>
     // allows one to overwrite the maximum cache size temporarily.
     // Its description contains a discussion about the effects of
     // setting a maximum cache.
-    TiledStMan (const String& hypercolumnName, uInt maximumCacheSize);
+    TiledStMan (const String& hypercolumnName, uInt maximumCacheSizeMiB);
 
     virtual ~TiledStMan();
 
@@ -131,12 +131,12 @@
     virtual Record dataManagerSpec() const;
 
     // Get data manager properties that can be modified.
-    // It is only ActualCacheSize (the actual cache size in buckets).
+    // It is only MaxCacheSize (the maximum cache size in MibiByte).
     // It is a subset of the data manager specification.
     virtual Record getProperties() const;
 
     // Modify data manager properties.
-    // Only ActualCacheSize can be used. It is similar to function setCacheSize
+    // Only MaxCacheSize can be used. It is similar to function setCacheSize
     // with <src>canExceedNrBuckets=False</src>.
     virtual void setProperties (const Record& spec);
 
@@ -167,10 +167,10 @@
 				    uInt maxNrPixelsPerTile = 32768);
     // </group>
 
-    // Set the maximum cache size (in bytes) in a non-persistent way.
-    virtual void setMaximumCacheSize (uInt nbytes);
+    // Set the maximum cache size (in MiB) in a non-persistent way.
+    virtual void setMaximumCacheSize (uInt nMiB);
 
-    // Get the current maximum cache size (in bytes).
+    // Get the current maximum cache size (in MiB (MibiByte)).
     uInt maximumCacheSize() const;
 
     // Get the current cache size (in buckets) for the hypercube in
@@ -376,8 +380,8 @@
       { return dataCols_p[colnr]; }
 
 protected:
-    // Set the persistent maximum cache size.
-    void setPersMaxCacheSize (uInt nbytes);
+    // Set the persistent maximum cache size (in MiB).
+    void setPersMaxCacheSize (uInt nMiB);
 
     // Get the bindings of the columns with the given names.
     // If bound, the pointer to the TSMColumn object is stored in the block.
@@ -504,9 +508,9 @@
     PtrBlock<TSMFile*> fileSet_p;
     // The assembly of all TSMCube objects.
     PtrBlock<TSMCube*> cubeSet_p;
-    // The persistent maximum cache size for a hypercube.
+    // The persistent maximum cache size (in MiB) for a hypercube.
     uInt      persMaxCacheSize_p;
-    // The actual maximum cache size for a hypercube.
+    // The actual maximum cache size for a hypercube (in MiB).
     uInt      maxCacheSize_p;
     // The dimensionality of the hypercolumn.
     uInt      nrdim_p;
@@ -547,10 +551,10 @@
 inline const TSMCube* TiledStMan::getHypercube (uInt rownr) const
     { return const_cast<TiledStMan*>(this)->getHypercube (rownr); }
 
-inline void TiledStMan::setPersMaxCacheSize (uInt nbytes)
+inline void TiledStMan::setPersMaxCacheSize (uInt nMiB)
 {
-    persMaxCacheSize_p = nbytes;
-    maxCacheSize_p = nbytes;
+    persMaxCacheSize_p = nMiB;
+    maxCacheSize_p = nMiB;
 }
 
 

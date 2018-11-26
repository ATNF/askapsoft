--- ./tables/DataMan/TSMOption.h-orig	2015-07-24 17:01:01.000000000 +1000
+++ ./tables/DataMan/TSMOption.h	2018-11-23 14:27:54.000000000 +1100
@@ -88,7 +88,7 @@
 // </ul>
 // The aipsrc variables are:
 // <ul>
-//  <li> <src>tables.tsm.option</src> gives the option as the case-insensitive
+//  <li> <src>table.tsm.option</src> gives the option as the case-insensitive
 //       string value:
 //   <ul>
 //    <li> <src>cache</src> means TSMCache.
@@ -101,12 +101,12 @@
 //       It defaults to value <src>default</src>.
 //       Note that <src>mmapold</src> is almost the same as <src>default</src>.
 //       Only on 32-bit systems it is different.
-//  <li> <src>tables.tsm.maxcachesizemb</src> gives the maximum cache size in MB
-//       for option <src>TSMOption::Cache</src>. A value -1 means that
-//       the system determines the maximum. A value 0 means unlimited.
+//  <li> <src>table.tsm.maxcachesizemb</src> gives the maximum cache size in
+//       MibiByte for option <src>TSMOption::Cache</src>. A value -1 means
+//       that the system determines the maximum. A value 0 means unlimited.
 //       It defaults to -1.
 //       Note it can always be overridden using class ROTiledStManAccessor.
-//  <li> <src>tables.tsm.buffersize</src> gives the buffer size for option
+//  <li> <src>table.tsm.buffersize</src> gives the buffer size for option
 //       <src>TSMOption::Buffer</src>. A value <=0 means use the default 4096.
 //       It defaults to 0.
 // </ul>
@@ -133,6 +133,8 @@
     // Create an option object.
     // The parameter values are described in the synopsis.
     // A size value -2 means reading that size from the aipsrc file.
+    // The buffer size has to be given in bytes.
+    // The maximum cache size has to be given in MibiBytes (1024*1024 bytes).
     TSMOption (Option option=Aipsrc, Int bufferSize=-2,
                Int maxCacheSizeMB=-2);
 
@@ -148,7 +150,7 @@
     Int bufferSize() const
       { return itsBufferSize; }
 
-    // Get the maximum cache size. -1 means undefined.
+    // Get the maximum cache size (in MibiByte). -1 means undefined.
     Int maxCacheSizeMB() const
       { return itsMaxCacheSize; }
 

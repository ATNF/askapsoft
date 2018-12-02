--- include/Blob/BlobArray.tcc.orig	2016-03-08 16:26:47.000000000 +1100
+++ include/Blob/BlobArray.tcc	2016-03-08 16:27:30.000000000 +1100
@@ -29,7 +29,7 @@
 #include <Common/LofarLogger.h>
 
 #if defined(HAVE_AIPSPP) 
-# include <casa/Arrays/Array.h>
+# include <casacore/casa/Arrays/Array.h>
 #endif
 
 

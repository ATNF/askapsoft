--- ./tables/DataMan/ISMBase.h-orig	2015-07-24 17:01:01.000000000 +1000
+++ ./tables/DataMan/ISMBase.h	2018-11-23 14:27:30.000000000 +1100
@@ -128,7 +128,7 @@
     virtual Record getProperties() const;
 
     // Modify data manager properties.
-    // Only ActualCacheSize can be used. It is similar to function setCacheSize
+    // Only MaxCacheSize can be used. It is similar to function setCacheSize
     // with <src>canExceedNrBuckets=False</src>.
     virtual void setProperties (const Record& spec);
 

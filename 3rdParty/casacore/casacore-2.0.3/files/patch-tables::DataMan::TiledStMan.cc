--- ./tables/DataMan/TiledStMan.cc-orig	2015-07-24 17:01:01.000000000 +1000
+++ ./tables/DataMan/TiledStMan.cc	2018-11-23 14:28:09.000000000 +1100
@@ -310,14 +310,14 @@
 Record TiledStMan::getProperties() const
 {
     Record rec;
-    rec.define ("ActualMaxCacheSize", Int(maxCacheSize_p));
+    rec.define ("MaxCacheSize", Int(maxCacheSize_p));
     return rec;
 }
 
 void TiledStMan::setProperties (const Record& rec)
 {
-    if (rec.isDefined("ActualMaxCacheSize")) {
-        setMaximumCacheSize (rec.asInt("ActualCacheSize"));
+    if (rec.isDefined("MaxCacheSize")) {
+        setMaximumCacheSize (rec.asInt("MaxCacheSize"));
     }
 }
 
@@ -353,8 +353,8 @@
     DOos::remove (fileName(), False, False);
 }
 
-void TiledStMan::setMaximumCacheSize (uInt nbytes)
-    { maxCacheSize_p = nbytes; }
+void TiledStMan::setMaximumCacheSize (uInt nMiB)
+    { maxCacheSize_p = nMiB; }
 
 
 Bool TiledStMan::canChangeShape() const

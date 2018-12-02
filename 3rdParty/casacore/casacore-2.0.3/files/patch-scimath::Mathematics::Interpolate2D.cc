--- ./scimath/Mathematics/Interpolate2D.cc.orig	2017-04-26 16:31:36.000000000 +1000
+++ ./scimath/Mathematics/Interpolate2D.cc	2017-04-26 16:31:26.000000000 +1000
@@ -278,7 +278,12 @@
   if (tmp==String("N")) {
     method2 = Interpolate2D::NEAREST;
   } else if (tmp==String("L")) {
-    method2 = Interpolate2D::LINEAR;
+    String tmp2 = String(typeU.at(1, 1));
+    if (tmp2==String("A")) {
+      method2 = Interpolate2D::LANCZOS;
+    } else {
+      method2 = Interpolate2D::LINEAR;
+    }
   } else if (tmp==String("C")) {
     method2 = Interpolate2D::CUBIC;
   } else if (tmp==String("Z")) {

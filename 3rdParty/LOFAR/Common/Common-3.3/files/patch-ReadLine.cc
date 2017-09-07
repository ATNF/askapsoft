--- src/ReadLine.cc.orig	2017-06-29 14:29:42.570581542 +1000
+++ src/ReadLine.cc	2017-06-29 14:27:02.837335819 +1000
@@ -46,7 +46,7 @@
   {
     if (!prompt.empty()) cerr << prompt;
     getline (cin, line);
-    return cin;
+    return true;
   }
 #endif
 

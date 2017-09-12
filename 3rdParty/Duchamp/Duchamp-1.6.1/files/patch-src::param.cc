--- ./src/param.cc.orig	2017-09-12 08:29:36.000000000 +0800
+++ ./src/param.cc	2017-09-12 08:29:23.000000000 +0800
@@ -911,7 +911,7 @@
       }
 
       if(doWarn){
-	DUCHAMPWARN("Reading parameters",errmsg);
+	DUCHAMPWARN("Reading parameters",errmsg.str());
 	DUCHAMPWARN("Reading parameters","The growth function is being turned off.");
 
       }

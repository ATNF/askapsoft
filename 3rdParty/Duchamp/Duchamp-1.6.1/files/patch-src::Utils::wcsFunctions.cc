--- src/Utils/wcsFunctions.cc.orig	2017-09-12 11:16:11.000000000 +1000
+++ src/Utils/wcsFunctions.cc	2017-09-12 11:16:43.000000000 +1000
@@ -358,7 +358,7 @@
 			      << wcs->cunit[specIndex]
 			      << " to " << outputUnits.c_str();
       //      errmsg << "\nUsing coordinate value instead.\n";
-      DUCHAMPERROR("coordToVel", errmsg);
+      DUCHAMPERROR("coordToVel", errmsg.str());
       errflag = 1;
     }
     return coord;

--- src/FitsIO/wcsIO.cc.orig	2017-09-12 08:31:55.000000000 +0800
+++ src/FitsIO/wcsIO.cc	2017-09-12 08:34:21.000000000 +0800
@@ -194,7 +194,7 @@
 	if (stat[i] > 0) 
 	  errmsg << i+1 << ": WCSFIX error code=" << stat[i] << ": "
 		 << wcsfix_errmsg[stat[i]] << std::endl;
-      DUCHAMPWARN("Cube Reader", errmsg);
+      DUCHAMPWARN("Cube Reader", errmsg.str());
       return FAILURE;
     }
     // Set up the wcsprm struct. Report if something goes wrong.
@@ -215,7 +215,7 @@
 	  if (stat[i] > 0) 
 	    errmsg << i+1 << ": WCSFIX error code=" << stat[i] << ": "
 		   << wcsfix_errmsg[stat[i]] << std::endl;
-	DUCHAMPWARN("Cube Reader", errmsg );
+	DUCHAMPWARN("Cube Reader", errmsg.str());
       }
 
 

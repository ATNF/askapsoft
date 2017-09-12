--- src/Utils/Section.cc.orig	2017-09-12 11:14:39.000000000 +1000
+++ src/Utils/Section.cc	2017-09-12 11:15:13.000000000 +1000
@@ -228,7 +228,7 @@
 
     if(removeStep){
       errmsg << this->subsection;
-      DUCHAMPWARN("Section parsing", errmsg);
+      DUCHAMPWARN("Section parsing", errmsg.str());
     }
 
     this->flagParsed = true;

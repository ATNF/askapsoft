--- ./casa/BasicMath/test/tMathNaN.cc.orig	2017-06-29 14:46:13.681491832 +1000
+++ ./casa/BasicMath/test/tMathNaN.cc	2017-06-29 14:46:19.733387480 +1000
@@ -43,7 +43,7 @@
 			    ((*(Int *)(x) & 0x007fffff) != 0x00000000))
 
 inline Bool isNaN_isnan(Float val) {
-  return (isnan(Double(val)));
+  return (std::isnan(Double(val)));
 }
 
 inline Bool isNaN_isnanf(const Float& val) {
--- ./casa/BasicMath/Math.cc.orig	2017-06-29 14:44:57.906798414 +1000
+++ ./casa/BasicMath/Math.cc	2017-06-29 14:45:28.138277132 +1000
@@ -178,7 +178,7 @@
   // infinite. I can only have access to Solaris, Linux and SGI machines to
   // determine this.
 #if defined(AIPS_LINUX)
-  return (isinf(Double(val)));
+  return (std::isinf(Double(val)));
 #elif defined(AIPS_DARWIN)
   return (std::isinf(Double(val)));
 #elif defined(AIPS_SOLARIS) || defined(AIPS_IRIX)
@@ -212,7 +212,7 @@
   // infinite. I can only have access to Solaris, Linux and SGI machines to
   // determine this.
 #if defined(AIPS_LINUX)
-  return (isinf(Double(val)));
+  return (std::isinf(Double(val)));
 #elif defined(AIPS_DARWIN)
   return (std::isinf(Double(val)));
 #elif defined(AIPS_SOLARIS) || defined(AIPS_IRIX)

--- ./casa/Utilities/CountedPtr.h.orig	2016-05-13 09:51:27.000000000 +1000
+++ ./casa/Utilities/CountedPtr.h	2016-05-13 09:52:11.000000000 +1000
@@ -30,7 +30,7 @@
 
 #include <casacore/casa/aips.h>
 
-#if (defined(AIPS_CXX11) || (defined(__APPLE_CC__) && __APPLE_CC__ > 5621))
+#if (defined(AIPS_CXX11) || defined(__APPLE_CC__))
 #include <memory>
 #define SHARED_PTR std::shared_ptr
 #define DYNAMIC_POINTER_CAST std::dynamic_pointer_cast

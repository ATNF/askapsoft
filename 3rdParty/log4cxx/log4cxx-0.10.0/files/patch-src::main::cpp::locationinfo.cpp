--- src/main/cpp/locationinfo.cpp.orig	2017-06-29 13:55:42.865752139 +1000
+++ src/main/cpp/locationinfo.cpp	2017-06-29 13:56:02.061421148 +1000
@@ -148,7 +148,7 @@
     if (lineNumber == -1 && fileName == NA && methodName == NA_METHOD) {
          os.writeNull(p);
     } else {
-        char prolog[] = {
+        unsigned char prolog[] = {
          0x72, 0x00, 0x21, 0x6F, 0x72, 0x67, 0x2E, 
          0x61, 0x70, 0x61, 0x63, 0x68, 0x65, 0x2E, 0x6C, 
          0x6F, 0x67, 0x34, 0x6A, 0x2E, 0x73, 0x70, 0x69, 
@@ -161,7 +161,7 @@
                 0x61, 0x76, 0x61, 0x2F, 0x6C, 0x61, 0x6E, 0x67, 
                 0x2F, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3B,
          0x78, 0x70 };
-      os.writeProlog("org.apache.log4j.spi.LocationInfo", 2, prolog, sizeof(prolog), p);
+      os.writeProlog("org.apache.log4j.spi.LocationInfo", 2, (char *) prolog, sizeof(prolog), p);
         char* line = p.itoa(lineNumber);
         //
         //   construct Java-like fullInfo (replace "::" with ".")
--- src/main/cpp/loggingevent.cpp.orig	2017-06-29 13:55:42.865752139 +1000
+++ src/main/cpp/loggingevent.cpp	2017-06-29 13:56:21.585084502 +1000
@@ -236,7 +236,7 @@
 
 
 void LoggingEvent::writeProlog(ObjectOutputStream& os, Pool& p)  {
-     char classDesc[] = {
+     unsigned char classDesc[] = {
         0x72, 0x00, 0x21, 
         0x6F, 0x72, 0x67, 0x2E, 0x61, 0x70, 0x61, 0x63, 
         0x68, 0x65, 0x2E, 0x6C, 0x6F, 0x67, 0x34, 0x6A, 
@@ -292,7 +292,7 @@
         0x3B, 0x78, 0x70 }; 
 
      os.writeProlog("org.apache.log4j.spi.LoggingEvent", 
-        8, classDesc, sizeof(classDesc), p);
+        8, (char *) classDesc, sizeof(classDesc), p);
 }
 
 void LoggingEvent::write(helpers::ObjectOutputStream& os, Pool& p) const {
--- src/main/cpp/objectoutputstream.cpp.orig	2017-06-29 13:55:42.865752139 +1000
+++ src/main/cpp/objectoutputstream.cpp	2017-06-29 13:57:22.984025801 +1000
@@ -36,8 +36,8 @@
        objectHandle(0x7E0000),
        classDescriptions(new ClassDescriptionMap())
 {
-   char start[] = { 0xAC, 0xED, 0x00, 0x05 };
-   ByteBuffer buf(start, sizeof(start));
+   unsigned char start[] = { 0xAC, 0xED, 0x00, 0x05 };
+   ByteBuffer buf((char *) start, sizeof(start));
    os->write(buf, p);
 }
 
@@ -81,7 +81,7 @@
     //
     //  TC_OBJECT and the classDesc for java.util.Hashtable
     //
-    char prolog[] = {
+    unsigned char prolog[] = {
         0x72, 0x00, 0x13, 0x6A, 0x61, 0x76, 0x61, 
         0x2E, 0x75, 0x74, 0x69, 0x6C, 0x2E, 0x48, 0x61, 
         0x73, 0x68, 0x74, 0x61, 0x62, 0x6C, 0x65, 0x13, 
@@ -90,7 +90,7 @@
         0x64, 0x46, 0x61, 0x63, 0x74, 0x6F, 0x72, 0x49, 
         0x00, 0x09, 0x74, 0x68, 0x72, 0x65, 0x73, 0x68, 
         0x6F, 0x6C, 0x64, 0x78, 0x70  };
-    writeProlog("java.util.Hashtable", 1, prolog, sizeof(prolog), p);
+    writeProlog("java.util.Hashtable", 1, (char *) prolog, sizeof(prolog), p);
     //
     //   loadFactor = 0.75, threshold = 5, blockdata start, buckets.size = 7
     char data[] = { 0x3F, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 
--- src/test/cpp/xml/domtestcase.cpp.orig	2017-06-29 13:55:42.861752208 +1000
+++ src/test/cpp/xml/domtestcase.cpp	2017-06-29 14:05:15.399879942 +1000
@@ -190,9 +190,9 @@
                 DOMConfigurator::configure(LOG4CXX_TEST_STR("input/xml/DOMTestCase3.xml"));
                 LOG4CXX_INFO(logger, "File name is expected to end with a superscript 3");
 #if LOG4CXX_LOGCHAR_IS_UTF8
-                const logchar fname[] = { 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2F, 0x64, 0x6F, 0x6D, 0xC2, 0xB3, 0 };
+                const logchar fname[] = { 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2F, 0x64, 0x6F, 0x6D, static_cast<logchar>(0xC2), static_cast<logchar>(0xB3), 0 };
 #else
-                const logchar fname[] = { 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2F, 0x64, 0x6F, 0x6D, 0xB3, 0 };
+                const logchar fname[] = { 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2F, 0x64, 0x6F, 0x6D, static_cast<logchar>(0xB3), 0 };
 #endif
                 File file;
                 file.setPath(fname);
@@ -209,9 +209,9 @@
                 DOMConfigurator::configure(LOG4CXX_TEST_STR("input/xml/DOMTestCase4.xml"));
                 LOG4CXX_INFO(logger, "File name is expected to end with an ideographic 4");
 #if LOG4CXX_LOGCHAR_IS_UTF8
-                const logchar fname[] = { 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2F, 0x64, 0x6F, 0x6D, 0xE3, 0x86, 0x95, 0 };
+                const logchar fname[] = { 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2F, 0x64, 0x6F, 0x6D, static_cast<logchar>(0xE3), static_cast<logchar>(0x86), static_cast<logchar>(0x95), 0 };
 #else
-                const logchar fname[] = { 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2F, 0x64, 0x6F, 0x6D, 0x3195, 0 };
+                const logchar fname[] = { 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2F, 0x64, 0x6F, 0x6D, static_cast<logchar>(0x3195), 0 };
 #endif
                 File file;
                 file.setPath(fname);

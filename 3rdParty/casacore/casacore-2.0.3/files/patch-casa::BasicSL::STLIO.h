--- casa/BasicSL/STLIO.h.orig	2016-03-14 17:13:17.000000000 +1100
+++ casa/BasicSL/STLIO.h	2016-03-14 17:14:45.000000000 +1100
@@ -98,7 +98,35 @@
                                                 const char* prefix="[",
                                                 const char* postfix="]")
     { showDataIter (os, c.begin(), c.end(), separator, prefix, postfix); }
+  // Print the contents of a container on LogIO.
+  // <group>
+  template<typename T>
+  inline LogIO& operator<<(LogIO &os, const std::vector<T> &a)
+    { os.output() << a; return os; }
+  template<typename T>
+  inline LogIO& operator<<(LogIO &os, const std::set<T> &a)
+    { os.output() << a; return os; }
+  template<typename T>
+  inline LogIO& operator<<(LogIO &os, const std::list<T> &a)
+    { os.output() << a; return os; }
+  template<typename T, typename U>
+  inline LogIO& operator<<(LogIO& os, const std::map<T,U>& a)
+    { os.output() << a; return os; }
+  // </group>
 
+  // Read or write the contents of an STL vector from/to AipsIO.
+  // The container is written in the same way as Block,
+  // thus can be read back that way and vice-versa.
+  // <group>
+  template<typename T>
+  AipsIO& operator>> (AipsIO& ios, std::vector<T>&);
+  template<typename T>
+  AipsIO& operator<< (AipsIO& ios, const std::vector<T>&);
+  // </group>
+
+} //# NAMESPACE CASACORE - END
+// These should be in the standard namespace because their arguments are
+namespace std {
   // Write a std::pair.
   template <typename T, typename U>
   inline ostream& operator<< (ostream& os, const std::pair<T,U>& p)
@@ -112,7 +140,7 @@
   template<typename T>
   inline ostream& operator<<(ostream& os, const std::vector<T>& v)
   {
-    showContainer (os, v, ",", "[", "]");
+    casa::showContainer (os, v, ",", "[", "]");
     return os;
   }
 
@@ -121,7 +149,7 @@
   template<typename T>
   inline ostream& operator<<(ostream& os, const std::set<T>& v)
   {
-    showContainer (os, v, ",", "[", "]");
+    casa::showContainer (os, v, ",", "[", "]");
     return os;
   }
 
@@ -130,7 +158,7 @@
   template<typename T>
   inline ostream& operator<<(ostream& os, const std::list<T>& v)
   {
-    showContainer (os, v, ",", "[", "]");
+    casa::showContainer (os, v, ",", "[", "]");
     return os;
   }
 
@@ -139,38 +167,11 @@
   template<typename T, typename U>
   inline ostream& operator<<(ostream& os, const std::map<T,U>& m)
   {
-    showContainer (os, m, ", ", "{", "}");
+    casa::showContainer (os, m, ", ", "{", "}");
     return os;
   }
 
-  // Print the contents of a container on LogIO.
-  // <group>
-  template<typename T>
-  inline LogIO& operator<<(LogIO &os, const std::vector<T> &a)
-    { os.output() << a; return os; }
-  template<typename T>
-  inline LogIO& operator<<(LogIO &os, const std::set<T> &a)
-    { os.output() << a; return os; }
-  template<typename T>
-  inline LogIO& operator<<(LogIO &os, const std::list<T> &a)
-    { os.output() << a; return os; }
-  template<typename T, typename U>
-  inline LogIO& operator<<(LogIO& os, const std::map<T,U>& a)
-    { os.output() << a; return os; }
-  // </group>
-
-  // Read or write the contents of an STL vector from/to AipsIO.
-  // The container is written in the same way as Block,
-  // thus can be read back that way and vice-versa.
-  // <group>
-  template<typename T>
-  AipsIO& operator>> (AipsIO& ios, std::vector<T>&);
-  template<typename T>
-  AipsIO& operator<< (AipsIO& ios, const std::vector<T>&);
-  // </group>
-
-} //# NAMESPACE CASACORE - END
-
+}
 #ifndef CASACORE_NO_AUTO_TEMPLATES
 #include <casacore/casa/BasicSL/STLIO.tcc>
 #endif //# CASACORE_NO_AUTO_TEMPLATES

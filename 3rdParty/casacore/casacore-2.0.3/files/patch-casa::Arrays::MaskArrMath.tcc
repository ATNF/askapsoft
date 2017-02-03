--- ./casa/Arrays/MaskArrMath.tcc.orig	2016-03-10 17:26:59.000000000 +1100
+++ ./casa/Arrays/MaskArrMath.tcc	2016-03-10 17:31:30.000000000 +1100
@@ -1742,13 +1742,24 @@
   MaskedArray<T> arr (array);
   Array<T> result (resShape);
   DebugAssert (result.contiguousStorage(), AipsError);
+  Array<Bool> resultMask(resShape);
   T* res = result.data();
+  Bool* resMask = resultMask.data();
   // Loop through all data and assemble as needed.
   IPosition blc(ndim, 0);
   IPosition trc(hboxsz);
   IPosition pos(ndim, 0);
   while (True) {
-    *res++ = funcObj (arr(blc,trc));
+//    *res++ = funcObj (arr(blc,trc));
+    MaskedArray<T> subarr (arr(blc,trc));
+    if (subarr.nelementsValid() == 0) {
+      *resMask++ = False;
+      *res++ = T();
+    }
+    else {
+      *resMask++ = True;
+      *res++ = funcObj (arr(blc,trc));
+    }
     uInt ax;
     for (ax=0; ax<ndim; ax++) {
       if (++pos[ax] < resShape[ax]) {

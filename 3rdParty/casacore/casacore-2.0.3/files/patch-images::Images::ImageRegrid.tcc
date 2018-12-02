--- ./images/Images/ImageRegrid.tcc.orig	2016-03-11 10:46:44.000000000 +1100
+++ ./images/Images/ImageRegrid.tcc	2016-03-11 10:48:04.000000000 +1100
@@ -1262,63 +1262,137 @@
 // to be masked as the coarse grid is unlikely to finish exactly
 // on the lattice edge
 
+  uInt i, k;
+  Int where;
+  Bool ok = True;
+
+  const uInt nPixelAxes = 2;
+  uInt nConversions;
+  if ( decimate > 1 ) {
+    nConversions = nOutI*nOutJ;
+  } else {
+    nConversions = ni*nj;
+  }
+
   Timer t0;
   uInt ii = 0;
   uInt jj = 0;
-  for (uInt j=0; j<nj; j+=jInc,jj++) {
+
+  // if useMachine, then do each pixel separately. Otherwise do a bulk conversion
+  if (useMachine) { // must be Direction
+    //
+    jj = 0;
+    for (uInt j=0; j<nj; j+=jInc,jj++) {
 	  ii = 0;
 	  for (uInt i=0; i<ni; i+=iInc,ii++) {
-		  outPixel(outXIdx) = i + outPos[xOutAxis];
-		  outPixel(outYIdx) = j + outPos[yOutAxis];
+		outPixel(outXIdx) = i + outPos[xOutAxis];
+        outPixel(outYIdx) = j + outPos[yOutAxis];
 
-		  // Do coordinate conversions (outpixel to world to inpixel)
-		  // for the axes of interest
+        // Do coordinate conversions (outpixel to world to inpixel)
+        // for the axes of interest
 
-		  if (useMachine) {                             // must be Direction
-			  ok1 = outDir.toWorld(outMVD, outPixel);
-			  ok2 = False;
-			  if (ok1) {
-				  inMVD = machine(outMVD).getValue();
-				  ok2 = inDir.toPixel(inPixel, inMVD);
-			  };
-		  } else {
-			  if (isDir) {
-				  ok1 = outDir.toWorld(world, outPixel);
-				  ok2 = False;
-				  if (ok1) ok2 = inDir.toPixel(inPixel, world);
-			  } else {
-				  ok1 = outLin.toWorld(world, outPixel);
-				  ok2 = False;
-				  if (ok1) ok2 = inLin.toPixel(inPixel, world);
-			  }
-		  };
+        ok1 = outDir.toWorld(outMVD, outPixel);
+        ok2 = False;
+        if (ok1) {
+          inMVD = machine(outMVD).getValue();
+          ok2 = inDir.toPixel(inPixel, inMVD);
+        };
 		  //
-		  if (!ok1 || !ok2) {
-			  succeed(i,j) = False;
-			  if (decimate>1) ijInMask2D(ii,jj) = False;
-		  } else {
-
-			  // This gives the 2D input pixel coordinate (relative to
-			  // the start of the full Lattice)
-			  // to find the interpolated result at.  (,,0) pertains to
-			  // inX and (,,1) to inY
-			  in2DPos(i,j,0) = inPixel(inXIdx);
-			  in2DPos(i,j,1) = inPixel(inYIdx);
-			  allFailed = False;
-			  succeed(i,j) = True;
-			  //
-			  if (decimate <= 1) {
-				  minInX = min(minInX,inPixel(inXIdx));
-				  minInY = min(minInY,inPixel(inYIdx));
-				  maxInX = max(maxInX,inPixel(inXIdx));
-				  maxInY = max(maxInY,inPixel(inYIdx));
-			  } else {
-				  iInPos2D(ii,jj) = inPixel(inXIdx);
-				  jInPos2D(ii,jj) = inPixel(inYIdx);
-				  ijInMask2D(ii,jj) = True;
-			  };
-		  };
-	  };
+        if (!ok1 || !ok2) {
+          succeed(i,j) = False;
+          if (decimate>1) ijInMask2D(ii,jj) = False;
+        } else {
+
+          // This gives the 2D input pixel coordinate (relative to
+          // the start of the full Lattice)
+          // to find the interpolated result at.  (,,0) pertains to
+          // inX and (,,1) to inY
+          in2DPos(i,j,0) = inPixel(inXIdx);
+          in2DPos(i,j,1) = inPixel(inYIdx);
+          allFailed = False;
+          succeed(i,j) = True;
+          //
+          if (decimate <= 1) {
+            minInX = min(minInX,inPixel(inXIdx));
+            minInY = min(minInY,inPixel(inYIdx));
+            maxInX = max(maxInX,inPixel(inXIdx));
+            maxInY = max(maxInY,inPixel(inYIdx));
+          } else {
+            iInPos2D(ii,jj) = inPixel(inXIdx);
+            jInPos2D(ii,jj) = inPixel(inYIdx);
+            ijInMask2D(ii,jj) = True;
+          };
+        };
+      };
+    };
+  } else {
+    // generate coordinate conversions in bulk
+    // set storage matrices for the conversions
+    Matrix<Double> inPixelMatrix(nPixelAxes,nConversions);
+    Matrix<Double> outPixelMatrix(nPixelAxes,nConversions);
+    Matrix<Double> worldMatrix(nPixelAxes,nConversions);
+    Vector<Bool> failures1(nConversions);
+    Vector<Bool> failures2(nConversions);
+    // set the output coordinates
+    uInt kk = 0;
+    jj = 0;
+    for (uInt j=0; j<nj; j+=jInc,jj++) {
+      ii = 0;
+      for (uInt i=0; i<ni; i+=iInc,ii++) {
+        outPixelMatrix(outXIdx,kk) = i + outPos[xOutAxis];
+        outPixelMatrix(outYIdx,kk) = j + outPos[yOutAxis];
+        kk++;
+      };
+    };
+    // do the conversions
+    if (isDir) {
+      ok1 = outDir.toWorldMany( worldMatrix, outPixelMatrix, failures1 );
+      ok2 = False;
+      if (ok1) ok2 = inDir.toPixelMany( inPixelMatrix, worldMatrix, failures2 );
+    } else {
+      ok1 = outLin.toWorldMany( worldMatrix, outPixelMatrix, failures1 );
+      ok2 = False;
+      if (ok1) ok2 = inLin.toPixelMany( inPixelMatrix, worldMatrix, failures2 );
+    }
+    // only keep going if some of the conversions succeeded
+    if (!ok2) {
+      allFailed = True;
+      succeed.set(False);
+      ijInMask2D.set(False);
+    } else {
+      allFailed = False;
+      kk = 0;
+      jj = 0;
+      for (uInt j=0; j<nj; j+=jInc,jj++) {
+        ii = 0;
+        for (uInt i=0; i<ni; i+=iInc,ii++) {
+          if (failures1(kk) || failures2(kk)) {
+            succeed(i,j) = False;
+            if (decimate>1) ijInMask2D(ii,jj) = False;
+          } else {
+            // This gives the 2D input pixel coordinate (relative to
+            // the start of the full Lattice)
+            // to find the interpolated result at.  (,,0) pertains to
+            // inX and (,,1) to inY
+            in2DPos(i,j,0) = inPixelMatrix(inXIdx,kk);
+            in2DPos(i,j,1) = inPixelMatrix(inYIdx,kk);
+            succeed(i,j) = True;
+            //
+            if (decimate <= 1) {
+              minInX = min(minInX,inPixelMatrix(inXIdx,kk));
+              minInY = min(minInY,inPixelMatrix(inYIdx,kk));
+              maxInX = max(maxInX,inPixelMatrix(inXIdx,kk));
+              maxInY = max(maxInY,inPixelMatrix(inYIdx,kk));
+            } else {
+              iInPos2D(ii,jj) = inPixelMatrix(inXIdx,kk);
+              jInPos2D(ii,jj) = inPixelMatrix(inYIdx,kk);
+              ijInMask2D(ii,jj) = True;
+            };
+          };
+          kk++;
+        };
+      };
+    };
   };
   if (itsShowLevel > 0) {
     cerr << "nII, nJJ= " << ii << ", " << jj << endl;

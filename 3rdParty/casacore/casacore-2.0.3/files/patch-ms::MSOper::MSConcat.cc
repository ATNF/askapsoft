--- ms/MSOper/MSConcat.cc.orig	2017-02-03 10:57:32.000000000 +1100
+++ ms/MSOper/MSConcat.cc	2017-02-03 10:55:08.000000000 +1100
@@ -1149,6 +1149,11 @@
   const ROScalarColumn<Int>& otherStateId = otherMainCols.stateId();
   const ROScalarColumn<Int>& otherObsId=otherMainCols.observationId();
 
+  const ROArrayColumn<Float>& otherSigmaSp = otherMainCols.sigmaSpectrum();
+  ArrayColumn<Float>& thisSigmaSp = sigmaSpectrum();
+  Bool copySigSp = !(thisSigmaSp.isNull() || otherSigmaSp.isNull());
+  copySigSp = copySigSp && thisSigmaSp.isDefined(0) && otherSigmaSp.isDefined(0);
+
   ScalarColumn<Int> thisScan;
   ScalarColumn<Int> thisStateId;
   ScalarColumn<Int> thisObsId;
@@ -1445,7 +1450,7 @@
     thisFlag.put(curRow, otherFlag, r);
     if (copyFlagCat) thisFlagCat.put(curRow, otherFlagCat, r);
     thisFlagRow.put(curRow, otherFlagRow, r);
-
+    if (copySigSp) thisSigmaSp.put(curRow, otherSigmaSp, r);
   } // end for
 
   if(doModelData){ //update the MODEL_DATA keywords

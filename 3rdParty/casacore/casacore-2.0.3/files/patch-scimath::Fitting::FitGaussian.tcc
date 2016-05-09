--- ./scimath/Fitting/FitGaussian.tcc.orig	2016-03-11 08:16:07.000000000 +1100
+++ ./scimath/Fitting/FitGaussian.tcc	2016-03-11 08:24:37.000000000 +1100
@@ -234,9 +234,10 @@
 
 
   NonLinearFitLM<T> fitter(0);
-  Vector<T> solution;
+  Vector<T> solution,errors;
   Matrix<T> startparameters(itsNGaussians, ngpars);
-  Matrix<T> solutionparameters(itsNGaussians, ngpars);
+  itsSolutionParameters = Matrix<T>(itsNGaussians, ngpars);
+  itsSolutionErrors = Matrix<T>(itsNGaussians, ngpars);
 
  Block<Gaussian1D<AutoDiff<T> > > gausscomp1d((itsDimension==1)*itsNGaussians);
  Block<Gaussian2D<AutoDiff<T> > > gausscomp2d((itsDimension==2)*itsNGaussians);
@@ -376,6 +377,7 @@
     fitter.setCriteria(convcriteria);
 
     solution.resize(0);
+    errors.resize(0);
     fitfailure = 0;
     attempt++;
 
@@ -385,6 +387,7 @@
     
     try {
        solution = fitter.fit(pos, f, sigma);
+       errors = fitter.errors();
     } catch (AipsError fittererror) {
       string errormessage;
       errormessage = fittererror.getMesg();
@@ -440,7 +443,9 @@
             //best fit so far - write parameters to solution matrix
             for (uInt g = 0; g < itsNGaussians; g++) {  
               for (uInt p = 0; p < ngpars; p++) {
-                solutionparameters(g,p) = solution(g*ngpars+p);
+                // solutionparameters(g,p) = solution(g*ngpars+p);
+                itsSolutionParameters(g,p) = solution(g*ngpars+p);
+                itsSolutionErrors(g,p) = errors(g*ngpars+p);
               }
             }
             bestRMS = itsRMS;
@@ -468,8 +473,8 @@
       cout << "no fit satisfies RMS criterion; using best available fit";
       cout << endl;
     }
-    correctParameters(solutionparameters);
-    return solutionparameters;
+    correctParameters(itsSolutionParameters);
+    return itsSolutionParameters;
   }
 
 // Otherwise, return all zeros 
@@ -479,11 +484,13 @@
 
   for (uInt g = 0; g < itsNGaussians; g++)  {   
     for (uInt p = 0; p < ngpars; p++) {
-      solutionparameters(g,p) = T(0.0);
+      //solutionparameters(g,p) = T(0.0);
+      itsSolutionParameters(g,p) = T(0.0);
+      itsSolutionErrors(g,p) = T(0.0);
     }
   }
 //
-  return solutionparameters;
+  return itsSolutionParameters;
    
 }
 

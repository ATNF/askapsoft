--- scimath/Fitting/FitGaussian.h.orig	2016-03-11 12:01:34.000000000 +1100
+++ ./scimath/Fitting/FitGaussian.h	2016-03-11 12:00:00.000000000 +1100
@@ -177,6 +177,9 @@
                 T maximumRMS = 1.0, uInt maxiter = 1024, 
                 T convcriteria = 0.0001);
 
+  Matrix<T> solution(){return itsSolutionParameters;};
+  Matrix<T> errors(){return itsSolutionErrors;}; 
+
   // Internal function for ensuring that parameters stay within their stated
   // domains (see <src>Gaussian2D</src> and <src>Gaussian3D</src>.)
   void correctParameters(Matrix<T>& parameters);
@@ -215,6 +218,12 @@
 
   //Find the number of unmasked parameters to be fit
   uInt countFreeParameters();
+
+  // The solutions to the fit
+  Matrix<T> itsSolutionParameters;
+  // The errors on the solution parameters
+  Matrix<T> itsSolutionErrors;
+
 };
 
 

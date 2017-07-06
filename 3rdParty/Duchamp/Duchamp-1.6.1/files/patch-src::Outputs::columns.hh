--- src/Outputs/columns.hh.orig	2017-07-06 12:44:11.000000000 +1000
+++ src/Outputs/columns.hh	2017-07-06 12:43:40.000000000 +1000
@@ -105,14 +105,14 @@
 	//-----------
 	// managing the width,precision,etc based on a value
 	void checkPrec(double d);
-	void checkWidth(int w);
-	void check(int i)          {int negVal=(i<0)?1:0; checkWidth(int(log10(fabs(double(i)))+1)+negVal);};
-	void check(long i)         {int negVal=(i<0)?1:0; checkWidth(int(log10(fabs(double(i)))+1)+negVal);};
-	void check(unsigned int i) {checkWidth(int(log10(double(i))+1));};
-	void check(unsigned long i){checkWidth(int(log10(double(i))+1));};
-	void check(std::string s){checkWidth(int(s.size()));};
-	void check(float f, bool doPrec=true) {if(doPrec) checkPrec(double(f)); int negVal=(f<0)?1:0; checkWidth(int(log10(fabs(f))+1)+1+itsPrecision+negVal); };
-	void check(double d, bool doPrec=true){if(doPrec) checkPrec(d);         int negVal=(d<0)?1:0; checkWidth(int(log10(fabs(d))+1)+1+itsPrecision+negVal); };
+	void checkWidth(int w, bool checkHeader=true);
+	void check(int i, bool checkHeader=true)          {int negVal=(i<0)?1:0; checkWidth(int(log10(fabs(double(i)))+1)+negVal, checkHeader);};
+	void check(long i, bool checkHeader=true)         {int negVal=(i<0)?1:0; checkWidth(int(log10(fabs(double(i)))+1)+negVal, checkHeader);};
+	void check(unsigned int i, bool checkHeader=true) {checkWidth(int(log10(double(i))+1), checkHeader);};
+	void check(unsigned long i, bool checkHeader=true){checkWidth(int(log10(double(i))+1), checkHeader);};
+	void check(std::string s, bool checkHeader=true){checkWidth(int(s.size()), checkHeader);};
+	void check(float f, bool checkHeader=true, bool doPrec=true) {if(doPrec) checkPrec(double(f)); int negVal=(f<0)?1:0; checkWidth(int(log10(fabs(f))+1)+1+itsPrecision+negVal, checkHeader); };
+	void check(double d, bool checkHeader=true, bool doPrec=true){if(doPrec) checkPrec(d);         int negVal=(d<0)?1:0; checkWidth(int(log10(fabs(d))+1)+1+itsPrecision+negVal, checkHeader); };
 
       //--------------
       // Outputting functions -- all in columns.cc

--- ms/MSOper/MSSummary.cc.orig	2018-01-24 10:24:40.000000000 +1100
+++ ms/MSOper/MSSummary.cc	2018-01-24 10:42:43.000000000 +1100
@@ -311,7 +311,7 @@
 	Int widthbtime = 22;
 	Int widthetime = 10;
 	Int widthFieldId = 5;
-	Int widthField = 20;
+	Int widthName = 32;
 	Int widthnrow = 10;
 	Int widthNUnflaggedRow = 13;
 	//Int widthInttim = 7;
@@ -498,8 +498,8 @@
 						os.output().setf(ios::right, ios::adjustfield);
 						os.output().width(widthFieldId); os << lastfldids(0) << " ";
 						os.output().setf(ios::left, ios::adjustfield);
-						if (name.length()>20) name.replace(19,1,'*');
-						os.output().width(widthField); os << name.at(0,20);
+						if (name.length()>widthName) name.replace((widthName-1),1,'*');
+						os.output().width(widthName); os << name.at(0,widthName);
 						//os.output().width(widthnrow); os << thisnrow;
 						os.output().width(widthnrow);
 						os.output().setf(ios::right, ios::adjustfield);
@@ -651,8 +651,8 @@
 			os.output().setf(ios::right, ios::adjustfield);
 			os.output().width(widthFieldId); os << lastfldids(0) << " ";
 			os.output().setf(ios::left, ios::adjustfield);
-			if (name.length()>20) name.replace(19,1,'*');
-			os.output().width(widthField); os << name.at(0,20);
+			if (name.length()>widthName) name.replace((widthName-1),1,'*');
+			os.output().width(widthName); os << name.at(0,widthName);
 			os.output().width(widthnrow);
 			os.output().setf(ios::right, ios::adjustfield);
 			os << _msmd->nRows(MSMetaData::BOTH, arrid, obsid, lastscan, lastfldids(0));
@@ -1264,7 +1264,7 @@
 		Int widthLead  =  2;
 		Int widthField =  5;
 		Int widthCode  =  5;
-		Int widthName  = 20;
+		Int widthName  = 32;
 		Int widthRA    = 16;
 		Int widthDec   = 16;
 		Int widthType  =  8;
@@ -1303,12 +1303,12 @@
 				MVAngle mvRa = mRaDec.getAngle().getValue()(0);
 				MVAngle mvDec= mRaDec.getAngle().getValue()(1);
 				String name=msFC.name()(fld);
-				if (name.length()>20) name.replace(19,1,"*");
+				if (name.length()>widthName) name.replace((widthName-1),1,"*");
 				os.output().setf(ios::left, ios::adjustfield);
 				os.output().width(widthLead);	os << "  ";
 				os.output().width(widthField);	os << (fld);
 				os.output().width(widthCode);   os << msFC.code()(fld);
-				os.output().width(widthName);	os << name.at(0,20);
+				os.output().width(widthName);	os << name.at(0,widthName);
 				os.output().width(widthRA);	os << mvRa(0.0).string(MVAngle::TIME,12);
 				os.output().width(widthDec);	os << mvDec.string(MVAngle::DIG2,11);
 				os.output().width(widthType);
@@ -1491,7 +1491,7 @@
 			Int widthLead =  2;
 			Int widthSrc  =  5;
 			//      Int widthTime = 15;
-			Int widthName = 20;
+			Int widthName = 32;
 			//      Int widthRA   = 14;
 			//      Int widthDec  = 15;
 			Int widthSpw  =  6;
@@ -1521,14 +1521,14 @@
 				MVAngle mvRa=mRaDec.getAngle().getValue()(0);
 				MVAngle mvDec=mRaDec.getAngle().getValue()(1);
 				String name=msSC.name()(row);
-				if (name.length()>20) name.replace(19,1,"*");
+				if (name.length()>widthName) name.replace((widthName-1),1,"*");
 
 				os.output().setf(ios::left, ios::adjustfield);
 				os.output().width(widthLead);	os<< "  ";
 				//	os.output().width(widthTime);
 				//				os<< MVTime(msSC.time()(row)/86400.0).string();
 				os.output().width(widthSrc);	os<< msSC.sourceId()(row);
-				os.output().width(widthName);	os<< name.at(0,20);
+				os.output().width(widthName);	os<< name.at(0,widthName);
 				//	os.output().width(widthRA);	os<< mvRa(0.0).string(MVAngle::TIME,10);
 				//	os.output().width(widthDec);	os<< mvDec.string(MVAngle::DIG2,10);
 				os.output().width(widthSpw);
@@ -2077,4 +2077,3 @@
 
 
 } //# NAMESPACE CASACORE - END
-

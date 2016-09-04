--- src/Detection/detection.cc.orig	2014-05-02 10:02:03.000000000 +1000
+++ src/Detection/detection.cc	2016-08-31 14:30:13.000000000 +1000
@@ -91,6 +91,10 @@
     this->velWidth = 0.;
     this->velMin = 0.;
     this->velMax = 0.;
+    this->z50min = 0.;
+    this->z50max = 0.;
+    this->z20min = 0.;
+    this->z20max = 0.;
     this->w20 = 0.;
     this->v20min = 0.;
     this->v20max = 0.;
@@ -168,6 +172,10 @@
     this->velWidth     = d.velWidth;
     this->velMin       = d.velMin;
     this->velMax       = d.velMax;
+    this->z50min       = d.z50min;
+    this->z50max       = d.z50max;
+    this->z20min       = d.z20min;
+    this->z20max       = d.z20max;
     this->w20          = d.w20;
     this->v20min       = d.v20min;
     this->v20max       = d.v20max;
@@ -917,11 +925,11 @@
     double zpt,xpt=double(this->getXcentre()),ypt=double(this->getYcentre());
     bool goLeft;
     
-    if(this->negSource){
-      // if we've inverted the source, need to make the feature
-      // positive for the interpolation/extrapolation to work
-      for(size_t i=0;i<zdim;i++) intSpec[i] *= -1.;
-    }
+    // if(this->negSource){
+    //   // if we've inverted the source, need to make the feature
+    //   // positive for the interpolation/extrapolation to work
+    //   for(size_t i=0;i<zdim;i++) intSpec[i] *= -1.;
+    // }
 
     float peak=0.;
     size_t peakLoc=0;
@@ -931,55 +939,88 @@
 	peakLoc = z;
       }
     }
+    zpt=double(peakLoc);
+    float level20=peak*0.2;
+    float level50=peak*0.5;
     
     size_t z=this->getZmin();
-    goLeft = intSpec[z]>peak*0.5;
-    if(goLeft) while(z>0 && intSpec[z]>peak*0.5) z--;
-    else       while(z<peakLoc && intSpec[z]<peak*0.5) z++;
-    if(z==0) this->v50min = this->velMin;
+    goLeft = intSpec[z]>level50;
+    if(goLeft) while(z>0 && intSpec[z]>level50) z--;
+    else       while(z<peakLoc && intSpec[z]<level50) z++;
+    if(z==0) {
+        this->z50min = this->getZmin();
+        this->v50min = this->velMin;
+    }
     else{
-      if(goLeft) zpt = z + (peak*0.5-intSpec[z])/(intSpec[z+1]-intSpec[z]);
-      else       zpt = z - (peak*0.5-intSpec[z])/(intSpec[z-1]-intSpec[z]);
-      this->v50min = head.pixToVel(xpt,ypt,zpt);
+      if(goLeft) this->z50min = z + (level50-intSpec[z])/(intSpec[z+1]-intSpec[z]);
+      else       this->z50min = z - (level50-intSpec[z])/(intSpec[z-1]-intSpec[z]);
+      this->v50min = head.pixToVel(xpt,ypt,this->z50min);
     }
+    
     z=this->getZmax();
-    goLeft = intSpec[z]<peak*0.5;
-    if(goLeft) while(z>peakLoc && intSpec[z]<peak*0.5) z--;
-    else       while(z<zdim    && intSpec[z]>peak*0.5) z++;
-    if(z==zdim) this->v50max = this->velMax;
+    goLeft = intSpec[z]<level50;
+    if(goLeft) while(z>peakLoc && intSpec[z]<level50) z--;
+    else       while(z<zdim    && intSpec[z]>level50) z++;
+    if(z==zdim){
+        this->z50max = this->getZmax();
+        this->v50max = this->velMax;
+    }
     else{
-      if(goLeft) zpt = z + (peak*0.5-intSpec[z])/(intSpec[z+1]-intSpec[z]);
-      else       zpt = z - (peak*0.5-intSpec[z])/(intSpec[z-1]-intSpec[z]);
-      this->v50max = head.pixToVel(xpt,ypt,zpt);
+      if(goLeft) this->z50max = z + (level50-intSpec[z])/(intSpec[z+1]-intSpec[z]);
+      else       this->z50max = z - (level50-intSpec[z])/(intSpec[z-1]-intSpec[z]);
+      this->v50max = head.pixToVel(xpt,ypt,this->z50max);
+    }
+   
+    if (z50min > z50max){
+        std::swap(z50min,z50max);
+    }
+    if (v50min > v50max){
+        std::swap(v50min,v50max);
     }
+    
     z=this->getZmin();
-    goLeft = intSpec[z]>peak*0.2;
-    if(goLeft) while(z>0 && intSpec[z]>peak*0.2) z--;
-    else       while(z<peakLoc && intSpec[z]<peak*0.2) z++;
-    if(z==0) this->v20min = this->velMin;
+    goLeft = intSpec[z]>level20;
+    if(goLeft) while(z>0 && intSpec[z]>level20) z--;
+    else       while(z<peakLoc && intSpec[z]<level20) z++;
+    if(z==0){
+        this->z20min = this->getZmin();
+        this->v20min = this->velMin;
+    }
     else{
-      if(goLeft) zpt = z + (peak*0.2-intSpec[z])/(intSpec[z+1]-intSpec[z]);
-      else       zpt = z - (peak*0.2-intSpec[z])/(intSpec[z-1]-intSpec[z]);
-      this->v20min = head.pixToVel(xpt,ypt,zpt);
+      if(goLeft) this->z20min = z + (level20-intSpec[z])/(intSpec[z+1]-intSpec[z]);
+      else       this->z20min = z - (level20-intSpec[z])/(intSpec[z-1]-intSpec[z]);
+      this->v20min = head.pixToVel(xpt,ypt,this->z20min);
     }
+    
     z=this->getZmax();
-    goLeft = intSpec[z]<peak*0.2;
-    if(goLeft) while(z>peakLoc && intSpec[z]<peak*0.2) z--;
-    else       while(z<zdim    && intSpec[z]>peak*0.2) z++;
-    if(z==zdim) this->v20max = this->velMax;
+    goLeft = intSpec[z]<level20;
+    if(goLeft) while(z>peakLoc && intSpec[z]<level20) z--;
+    else       while(z<zdim    && intSpec[z]>level20) z++;
+    if(z==zdim){
+        this->z50max = this->getZmax();
+        this->v20max = this->velMax;
+    }
     else{
-      if(goLeft) zpt = z + (peak*0.2-intSpec[z])/(intSpec[z+1]-intSpec[z]);
-      else       zpt = z - (peak*0.2-intSpec[z])/(intSpec[z-1]-intSpec[z]);
-      this->v20max = head.pixToVel(xpt,ypt,zpt);
+      if(goLeft) this->z20max = z + (level20-intSpec[z])/(intSpec[z+1]-intSpec[z]);
+      else       this->z20max = z - (level20-intSpec[z])/(intSpec[z-1]-intSpec[z]);
+      this->v20max = head.pixToVel(xpt,ypt,this->z20max);
+    }
+
+    if (z20min > z20max){
+        std::swap(z20min,z20max);
+    }
+    if (v20min > v20max){
+        std::swap(v20min,v20max);
     }
 
+
     this->w20 = fabs(this->v20min - this->v20max);
     this->w50 = fabs(this->v50min - this->v50max);
     
-    if(this->negSource){
-      // un-do the inversion, in case intSpec is needed elsewhere
-      for(size_t i=0;i<zdim;i++) intSpec[i] *= -1.;
-    }
+    // if(this->negSource){
+    //   // un-do the inversion, in case intSpec is needed elsewhere
+    //   for(size_t i=0;i<zdim;i++) intSpec[i] *= -1.;
+    // }
 
 
   }
@@ -1022,13 +1063,52 @@
   }
   //--------------------------------------------------------------------
 
+    std::string Detection::boundingSection(std::vector<size_t> dim, FitsHeader *header, unsigned int padsize)
+    {
+	/// @details This function returns a subsection string that shows the bounding box for the object. This will be in a suitable format for use with the subsection string in the input parameter set. It uses the FitsHeader object to know which axis belongs where.
+
+	std::vector<std::string> sectionlist(dim.size(),"1:1");
+	std::stringstream ss;
+	// ra - x-dim range
+	int axis=header->getWCS()->lng;
+	if(axis>=0){
+	    ss.str("");
+	    ss << std::max(1L,this->xmin-padsize+1)<<":"<<std::min(long(dim[axis]),this->xmax+padsize+1);
+	    sectionlist[axis]=ss.str();
+	}
+	// dec - y-dim range
+	axis=header->getWCS()->lat;
+	if(axis>=0){
+	    ss.str("");
+	    ss << std::max(1L,this->ymin-padsize+1)<<":"<<std::min(long(dim[axis]),this->ymax+padsize+1);
+	    sectionlist[axis]=ss.str();
+	}
+	// ra - x-dim range
+	axis=header->getWCS()->spec;
+	if(axis>=0){
+	    ss.str("");
+	    ss << std::max(1L,this->zmin-padsize+1)<<":"<<std::min(long(dim[axis]),this->zmax+padsize+1);
+	    sectionlist[axis]=ss.str();
+	}
+	ss.str("");
+	ss << "[ " << sectionlist[0];
+	for(size_t i=1;i<dim.size();i++) ss << "," << sectionlist[i];
+	ss << "]";
+	return ss.str();
+    }
+
+  //--------------------------------------------------------------------
+
   void Detection::setOffsets(Param &par)
   {
-    ///  @details
-    /// This function stores the values of the offsets for each cube axis.
-    /// The offsets are the starting values of the cube axes that may differ from
-    ///  the default value of 0 (for instance, if a subsection is being used).
-    /// The values will be used when the detection is outputted.
+    ///  @details This function stores the values of the offsets for
+    /// each cube axis.  The offsets are the starting values of the
+    /// cube axes that may differ from the default value of 0 (for
+    /// instance, if a subsection is being used).  The values will be
+    /// used when the detection is outputted.  NB - this function
+    /// merely sets the values of the offset parameters, it *does not*
+    /// apply them to the pixels & parameters (that is the
+    /// applyOffsets() function).
 
     this->xSubOffset = par.getXOffset();
     this->ySubOffset = par.getYOffset();

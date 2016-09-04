--- src/Detection/detection.hh.orig	2016-08-31 14:16:07.000000000 +1000
+++ src/Detection/detection.hh	2016-08-31 14:16:34.000000000 +1000
@@ -106,20 +106,22 @@
     /// @brief Calculate the spatial (moment-0) shape
     void findShape(const float *momentMap, const size_t *dim, FitsHeader &head);
 
+      /// @brief Return a string indicating the bounding subsection
+      std::string boundingSection(std::vector<size_t> dim, FitsHeader *header, unsigned int padsize=0);
+
+
     /// @brief Set the values of the axis offsets from the cube. 
     void   setOffsets(Param &par); 
 
-      using Object3D::addOffsets;  //tell the compiler we want both the addOffsets from Object3D *and* Detection
-
-    /// @brief Add the offset values to the pixel locations 
-   void   addOffsets(size_t xoff, size_t yoff, size_t zoff){
-       Object3D::addOffsets(xoff,yoff,zoff);
-       xpeak+=xoff; ypeak+=yoff; zpeak+=zoff;
-       xCentroid+=xoff; yCentroid+=yoff; zCentroid+=zoff;
-    };
+      /// @brief Add the offset values to the pixel locations 
+      void   addOffsets(long xoff, long yoff, long zoff){
+	  Object3D::addOffsets(xoff,yoff,zoff);
+	  xpeak+=xoff; ypeak+=yoff; zpeak+=zoff;
+	  xCentroid+=xoff; yCentroid+=yoff; zCentroid+=zoff;
+      };
 
       void   addOffsets(){ addOffsets(xSubOffset, ySubOffset, zSubOffset);};
-      void   removeOffsets(size_t xoff, size_t yoff, size_t zoff){ addOffsets(-xoff, -yoff, -zoff);};
+      void   removeOffsets(long xoff, long yoff, long zoff){ addOffsets(-xoff, -yoff, -zoff);};
       void   removeOffsets(){ addOffsets(-xSubOffset, -ySubOffset, -zSubOffset);};
       void   addOffsets(Param &par){setOffsets(par); addOffsets();};
 
@@ -238,6 +240,10 @@
     double      getVelWidth(){return velWidth;};
     double      getVelMin(){return velMin;};
     double      getVelMax(){return velMax;};
+      double    getZ50min(){return z50min;};
+      double    getZ50max(){return z50max;};
+      double    getZ20min(){return z20min;};
+      double    getZ20max(){return z20max;};
     double      getW20(){return w20;};
     double      getV20Min(){return v20min;};
     double      getV20Max(){return v20max;};
@@ -306,6 +312,10 @@
     double         velWidth;       ///< Full velocity width
     double         velMin;         ///< Minimum velocity
     double         velMax;         ///< Maximum velocity
+      double       z50min;         ///< Minimum z point at 50% of peak flux
+      double       z50max;         ///< Maximum z point at 50% of peak flux
+      double       z20min;         ///< Minimum z point at 20% of peak flux
+      double       z20max;         ///< Maximum z point at 20% of peak flux
     double         v20min;         ///< Minimum velocity at 20% of peak flux
     double         v20max;         ///< Maximum velocity at 20% of peak flux
     double         w20;            ///< Velocity width at 20% of peak flux  

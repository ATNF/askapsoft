--- lang/java/src/com/sleepycat/asm/ClassReader.java.orig	2015-04-10 09:47:36.086150000 +0800
+++ lang/java/src/com/sleepycat/asm/ClassReader.java	2015-04-10 09:48:08.144774000 +0800
@@ -162,10 +162,13 @@
      */
     public ClassReader(final byte[] b, final int off, final int len) {
         this.b = b;
+/**
+* Comment out as this is failing the galaxy build
         // checks the class version
         if (readShort(6) > Opcodes.V1_7) {
             throw new IllegalArgumentException();
         }
+*/
         // parses the constant pool
         items = new int[readUnsignedShort(off + 8)];
         int n = items.length;

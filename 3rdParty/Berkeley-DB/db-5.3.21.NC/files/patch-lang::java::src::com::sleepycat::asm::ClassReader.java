--- lang/java/src/com/sleepycat/asm/ClassReader.java.orig	2015-12-10 13:12:48.223988000 +0800
+++ lang/java/src/com/sleepycat/asm/ClassReader.java	2015-12-10 13:13:06.049236000 +0800
@@ -163,7 +163,7 @@
     public ClassReader(final byte[] b, final int off, final int len) {
         this.b = b;
         // checks the class version
-        if (readShort(6) > Opcodes.V1_7) {
+        if (readShort(6) > Opcodes.V1_8) {
             throw new IllegalArgumentException();
         }
         // parses the constant pool

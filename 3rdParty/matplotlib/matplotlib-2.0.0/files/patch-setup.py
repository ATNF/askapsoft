--- setup.py.orig	2018-04-20 14:58:58.877053464 +1000
+++ setup.py	2018-04-20 14:59:23.629664967 +1000
@@ -68,7 +68,7 @@
     'Required dependencies and extensions',
     setupext.Numpy(),
     setupext.Six(),
-    setupext.Dateutil(),
+    setupext.Dateutil('==2.6.1'),
     setupext.FuncTools32(),
     setupext.Subprocess32(),
     setupext.Pytz(),

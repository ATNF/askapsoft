--- setup.py.orig	2019-09-09 13:18:10.000000000 +1000
+++ setup.py	2019-09-09 13:18:25.000000000 +1000
@@ -98,7 +98,7 @@
       description=DESCRIPTION,
       scripts=scripts,
       requires=['astropy', 'numpy', 'matplotlib'],
-      install_requires=['astropy'],
+#      install_requires=['astropy'],
       provides=[PACKAGENAME],
       author=AUTHOR,
       author_email=AUTHOR_EMAIL,

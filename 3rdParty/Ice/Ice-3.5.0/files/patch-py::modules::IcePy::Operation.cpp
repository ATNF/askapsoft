--- py/modules/IcePy/Operation.cpp	2013-03-12 02:19:47.000000000 +1100
+++ py/modules/IcePy/Operation.cpp.new	2017-11-07 15:10:29.000000000 +1100
@@ -1654,10 +1654,7 @@
             }
             else
             {
-                if(PyTuple_SET_ITEM(results.get(), info->pos, Unset) < 0)
-                {
-                    return 0;
-                }
+                PyTuple_SET_ITEM(results.get(), info->pos, Unset);
                 Py_INCREF(Unset); // PyTuple_SET_ITEM steals a reference.
             }
         }
@@ -2504,10 +2501,7 @@
             throwPythonException();
         }
 
-        if(PyTuple_SET_ITEM(result.get(), 0, ok ? getTrue() : getFalse()) < 0)
-        {
-            throwPythonException();
-        }
+        PyTuple_SET_ITEM(result.get(), 0, ok ? getTrue() : getFalse());
 
 #if PY_VERSION_HEX >= 0x03000000
         PyObjectHandle op;
@@ -2544,10 +2538,7 @@
         }
 #endif
         
-        if(PyTuple_SET_ITEM(result.get(), 1, op.get()) < 0)
-        {
-            throwPythonException();
-        }
+        PyTuple_SET_ITEM(result.get(), 1, op.get());
         op.release(); // PyTuple_SET_ITEM steals a reference.
 
         return result.release();
@@ -2785,10 +2776,7 @@
             return 0;
         }
 
-        if(PyTuple_SET_ITEM(args.get(), 0, ok ? getTrue() : getFalse()) < 0)
-        {
-            return 0;
-        }
+        PyTuple_SET_ITEM(args.get(), 0, ok ? getTrue() : getFalse());
 
 #if PY_VERSION_HEX >= 0x03000000
         Py_ssize_t sz = results.second - results.first;
@@ -2825,10 +2813,7 @@
         memcpy(buf, results.first, sz);
 #endif
 
-        if(PyTuple_SET_ITEM(args.get(), 1, op.get()) < 0)
-        {
-            return 0;
-        }
+        PyTuple_SET_ITEM(args.get(), 1, op.get());
         op.release(); // PyTuple_SET_ITEM steals a reference.
 
         return args.release();
@@ -2868,12 +2853,7 @@
             return;
         }
 
-        if(PyTuple_SET_ITEM(args.get(), 0, ok ? getTrue() : getFalse()) < 0)
-        {
-            assert(PyErr_Occurred());
-            PyErr_Print();
-            return;
-        }
+        PyTuple_SET_ITEM(args.get(), 0, ok ? getTrue() : getFalse());
 
 #if PY_VERSION_HEX >= 0x03000000
         Py_ssize_t sz = results.second - results.first;
@@ -2916,12 +2896,7 @@
         memcpy(buf, results.first, sz);
 #endif
 
-        if(PyTuple_SET_ITEM(args.get(), 1, op.get()) < 0)
-        {
-            assert(PyErr_Occurred());
-            PyErr_Print();
-            return;
-        }
+        PyTuple_SET_ITEM(args.get(), 1, op.get());
         op.release(); // PyTuple_SET_ITEM steals a reference.
 
         PyObjectHandle tmp = PyObject_Call(_response, args.get(), 0);
@@ -3084,12 +3059,7 @@
             return;
         }
 
-        if(PyTuple_SET_ITEM(args.get(), 0, ok ? getTrue() : getFalse()) < 0)
-        {
-            assert(PyErr_Occurred());
-            PyErr_Print();
-            return;
-        }
+        PyTuple_SET_ITEM(args.get(), 0, ok ? getTrue() : getFalse());
 
 #if PY_VERSION_HEX >= 0x03000000
         Py_ssize_t sz = results.second - results.first;
@@ -3132,12 +3102,7 @@
         memcpy(buf, results.first, sz);
 #endif
 
-        if(PyTuple_SET_ITEM(args.get(), 1, op.get()) < 0)
-        {
-            assert(PyErr_Occurred());
-            PyErr_Print();
-            return;
-        }
+        PyTuple_SET_ITEM(args.get(), 1, op.get());
         op.release(); // PyTuple_SET_ITEM steals a reference.
 
         const string methodName = "ice_response";
@@ -3260,10 +3225,7 @@
                 }
                 else
                 {
-                    if(PyTuple_SET_ITEM(args.get(), info->pos + offset, Unset) < 0)
-                    {
-                        throwPythonException();
-                    }
+                    PyTuple_SET_ITEM(args.get(), info->pos + offset, Unset);
                     Py_INCREF(Unset); // PyTuple_SET_ITEM steals a reference.
                 }
             }
@@ -3287,10 +3249,7 @@
     // Create an object to represent Ice::Current. We need to append this to the argument tuple.
     //
     PyObjectHandle curr = createCurrent(current);
-    if(PyTuple_SET_ITEM(args.get(), PyTuple_GET_SIZE(args.get()) - 1, curr.get()) < 0)
-    {
-        throwPythonException();
-    }
+    PyTuple_SET_ITEM(args.get(), PyTuple_GET_SIZE(args.get()) - 1, curr.get());
     curr.release(); // PyTuple_SET_ITEM steals a reference.
 
     if(_op->amd)
@@ -3305,11 +3264,7 @@
         }
         obj->upcall = new UpcallPtr(this);
         obj->encoding = current.encoding;
-        if(PyTuple_SET_ITEM(args.get(), 0, (PyObject*)obj) < 0) // PyTuple_SET_ITEM steals a reference.
-        {
-            Py_DECREF(obj);
-            throwPythonException();
-        }
+        PyTuple_SET_ITEM(args.get(), 0, (PyObject*)obj); // PyTuple_SET_ITEM steals a reference.
     }
 
     //
@@ -3656,10 +3611,7 @@
     }
 #endif
 
-    if(PyTuple_SET_ITEM(args.get(), start, ip.get()) < 0)
-    {
-        throwPythonException();
-    }
+    PyTuple_SET_ITEM(args.get(), start, ip.get());
     ++start;
     ip.release(); // PyTuple_SET_ITEM steals a reference.
 
@@ -3668,10 +3620,7 @@
     // this to the argument tuple.
     //
     PyObjectHandle curr = createCurrent(current);
-    if(PyTuple_SET_ITEM(args.get(), start, curr.get()) < 0)
-    {
-        throwPythonException();
-    }
+    PyTuple_SET_ITEM(args.get(), start, curr.get());
     curr.release(); // PyTuple_SET_ITEM steals a reference.
 
     string dispatchName = "ice_invoke";
@@ -3688,11 +3637,7 @@
         }
         obj->upcall = new UpcallPtr(this);
         obj->encoding = current.encoding;
-        if(PyTuple_SET_ITEM(args.get(), 0, (PyObject*)obj) < 0) // PyTuple_SET_ITEM steals a reference.
-        {
-            Py_DECREF(obj);
-            throwPythonException();
-        }
+        PyTuple_SET_ITEM(args.get(), 0, (PyObject*)obj); // PyTuple_SET_ITEM steals a reference.
     }
 
     //

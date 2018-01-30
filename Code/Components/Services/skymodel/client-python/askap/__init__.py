# pkg_resources namespace package.
# We don't need the fallback to pkgutil namespace package for 2 reasons:
# 1 - we have setuptools as a required package for installation
# 2 - pkg_resources and pkgutil namespace packages are not cross-compatible,
# so falling back to one from the other does not make sense.
__import__('pkg_resources').declare_namespace(__name__)

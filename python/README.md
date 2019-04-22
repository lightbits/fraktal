# Python bindings for the fraktal core library

## Installation

* Run the appropriate build script to generate a dynamic library for your platform. On Windows/MSVC this is done using build_dynamic_lib. On Linux/MacOS this is done using the Makefile.

* Copy the pyfraktal directory into your project or Python installation.

* Make the dynamic library accessible to Python. You can do this by defining an environment variable "PYFRAKTAL_DLL" to be the complete path to the fraktal.dll (or .so) on your system. You may also copy the library into your project or the pyfraktal package directory.

## Test your installation

You can test your installation by running the following in Python:

```
import fraktal
a = fraktal.create_array(None, 32, 16, 4, fraktal.FLOAT, fraktal.READ_ONLY)
w,h = fraktal.array_size(a)
print(w, h)
```

If fraktal is successfully installed, the script should print "(32, 16)"..


"""
Python bindings for fraktal.
"""

__author__ = 'Simen Haugo'
__license__ = 'MIT'

import sys
import os
import ctypes

_to_char_p = lambda s: s.encode('utf-8')

libname = 'fraktal.so'
if sys.platform == 'win32':
    libname = 'fraktal.dll'

if os.environ.get('PYFRAKTAL_DLL', ''):
    try:
        _fraktal = ctypes.CDLL(os.environ['PYFRAKTAL_DLL'])
    except OSError:
        _fraktal = None
else:
    try:
        _fraktal = ctypes.CDLL(libname)
    except OSError:
        # try the package directory
        try:
            _fraktal = ctypes.CDLL(os.path.join(os.path.abspath(os.path.dirname(__file__)), libname))
        except OSError:
            _fraktal = None

if _fraktal is None:
    raise ImportError(
    "Failed to load fraktal dynamic/shared library. "
    "Ensure that fraktal.dll (or .so) is accessible "
    "from the working directory. You may alternatively "
    "set the environment variable PYFRAKTAL_DLL to be "
    "the full path to fraktal.dll/so on your computer.")

############################################################
# §1 Enums
############################################################

READ_ONLY     = 0
READ_WRITE    = 1
FLOAT         = 2
UINT8         = 3
CLAMP_TO_EDGE = 4
REPEAT        = 5
LINEAR        = 6
NEAREST       = 7

class FraktalError(Exception):
    def __init__(self, message):
        super(FraktalError, self).__init__(message)

############################################################
# §2 Arrays
############################################################

_fraktal.fraktal_create_array.restype = ctypes.c_void_p
_fraktal.fraktal_create_array.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
def create_array(data, width, height, channels, format, access):
    if data is None:
        return _fraktal.fraktal_create_array(None, width, height, channels, format, access)
    else:
        if format == FLOAT:
            array_type = ctypes.c_float * (channels * width * height)
        elif format == UINT8:
            array_type = ctypes.c_uchar * (channels * width * height)
        else:
            raise
        array = array_type()
        for y in range(height):
            for x in range(width):
                for i in range(channels):
                    array[channels*(x + y*width) + i] = data[channels*(x + y*width) + i]
        return _fraktal.fraktal_create_array(array, width, height, channels, format, access)

_fraktal.fraktal_destroy_array.restype = None
_fraktal.fraktal_destroy_array.argtypes = [ctypes.c_void_p]
def destroy_array(array):
    _fraktal.fraktal_destroy_array(array)

_fraktal.fraktal_zero_array.restype = None
_fraktal.fraktal_zero_array.argtypes = [ctypes.c_void_p]
def zero_array(array):
    _fraktal.fraktal_zero_array(array)

_fraktal.fraktal_to_cpu.restype = None
_fraktal.fraktal_to_cpu.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
def to_cpu(array):
    width,height = array_size(array)
    channels = array_channels(array)
    format = array_format(array)
    if format == FLOAT:
        array_type = ctypes.c_float * (channels * width * height)
        dcpu = array_type()
        _fraktal.fraktal_to_cpu(dcpu, array)
        return [float(i) for i in dcpu]
    elif format == UINT8:
        array_type = ctypes.c_uchar * (channels * width * height)
        dcpu = array_type()
        _fraktal.fraktal_to_cpu(dcpu, array)
        return [int(i) for i in dcpu]
    else:
        raise

_fraktal.fraktal_array_size.restype = None
_fraktal.fraktal_array_size.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
def array_size(array):
    width = ctypes.c_int(0)
    pwidth = ctypes.pointer(width)
    height = ctypes.c_int(0)
    pheight = ctypes.pointer(height)
    _fraktal.fraktal_array_size(array, pwidth, pheight)
    return width.value, height.value

_fraktal.fraktal_array_channels.restype = ctypes.c_int
_fraktal.fraktal_array_channels.argtypes = [ctypes.c_void_p]
def array_format(array):
    return _fraktal.fraktal_array_format(array)

_fraktal.fraktal_array_channels.restype = ctypes.c_int
_fraktal.fraktal_array_channels.argtypes = [ctypes.c_void_p]
def array_channels(array):
    return _fraktal.fraktal_array_channels(array)

############################################################
# §3 Kernels
############################################################

_fraktal.fraktal_create_link.restype = ctypes.c_void_p
_fraktal.fraktal_create_link.argtypes = []
def create_link():
    return _fraktal.fraktal_create_link()

_fraktal.fraktal_destroy_link.restype = None
_fraktal.fraktal_destroy_link.argtypes = [ctypes.c_void_p]
def destroy_link(link):
    return _fraktal.fraktal_destroy_link(link)

_fraktal.fraktal_add_link_data.restype = None
_fraktal.fraktal_add_link_data.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_char_p]
def add_link_data(link, data, size, name):
    return _fraktal.fraktal_add_link_data(link, data, size, _to_char_p(name))

_fraktal.fraktal_add_link_file.restype = None
_fraktal.fraktal_add_link_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
def add_link_file(link, path):
    return _fraktal.fraktal_add_link_file(link, _to_char_p(path))

_fraktal.fraktal_destroy_kernel.restype = None
_fraktal.fraktal_destroy_kernel.argtypes = [ctypes.c_void_p]
def destroy_kernel(kernel):
    return _fraktal.fraktal_destroy_kernel(kernel)

_fraktal.fraktal_load_kernel.restype = ctypes.c_void_p
_fraktal.fraktal_load_kernel.argtypes = [ctypes.c_char_p]
def load_kernel(filename):
    return _fraktal.fraktal_load_kernel(_to_char_p(filename))

_fraktal.fraktal_use_kernel.restype = None
_fraktal.fraktal_use_kernel.argtypes = [ctypes.c_void_p]
def use_kernel(kernel):
    _fraktal.fraktal_use_kernel(kernel)

_fraktal.fraktal_run_kernel.restype = None
_fraktal.fraktal_run_kernel.argtypes = [ctypes.c_void_p]
def run_kernel(array):
    _fraktal.fraktal_run_kernel(array)

############################################################
# §4 Parameters
############################################################

_fraktal.fraktal_get_param_offset.restype = ctypes.c_int
_fraktal.fraktal_get_param_offset.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
def get_param_offset(kernel, name):
    return _fraktal.fraktal_get_param_offset(kernel, _to_char_p(name))

_fraktal.fraktal_param_1f.restype = None
_fraktal.fraktal_param_1f.argtypes = [ctypes.c_int, ctypes.c_float]
def fraktal_param_1f(offset, x):
    _fraktal.fraktal_param_1f(offset, x)

_fraktal.fraktal_param_2f.restype = None
_fraktal.fraktal_param_2f.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float]
def fraktal_param_2f(offset, x, y):
    _fraktal.fraktal_param_2f(offset, x, y)

_fraktal.fraktal_param_3f.restype = None
_fraktal.fraktal_param_3f.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float]
def fraktal_param_3f(offset, x, y, z):
    _fraktal.fraktal_param_3f(offset, x, y, z)

_fraktal.fraktal_param_4f.restype = None
_fraktal.fraktal_param_4f.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
def fraktal_param_4f(offset, x, y, z, w):
    _fraktal.fraktal_param_4f(offset, x, y, z, w)

_fraktal.fraktal_param_1i.restype = None
_fraktal.fraktal_param_1i.argtypes = [ctypes.c_int, ctypes.c_int]
def fraktal_param_1i(offset, x):
    _fraktal.fraktal_param_1i(offset, x)

_fraktal.fraktal_param_2i.restype = None
_fraktal.fraktal_param_2i.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
def fraktal_param_2i(offset, x, y):
    _fraktal.fraktal_param_2i(offset, x, y)

_fraktal.fraktal_param_3i.restype = None
_fraktal.fraktal_param_3i.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
def fraktal_param_3i(offset, x, y, z):
    _fraktal.fraktal_param_3i(offset, x, y, z)

_fraktal.fraktal_param_4i.restype = None
_fraktal.fraktal_param_4i.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
def fraktal_param_4i(offset, x, y, z, w):
    _fraktal.fraktal_param_4i(offset, x, y, z, w)

_fraktal.fraktal_param_matrix4f.restype = None
_fraktal.fraktal_param_matrix4f.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_float)]
def param_matrix4f(offset, data, transpose=False):
    data_type = ctypes.c_float * (4 * 4)
    pdata = data_type()
    if transpose:
        for col in range(4):
            for row in range(4):
                pdata[row + 4*col] = data[row][col]
    else:
        for col in range(4):
            for row in range(4):
                pdata[col + 4*row] = data[row][col]
    _fraktal.fraktal_param_matrix4f(offset, pdata)

_fraktal.fraktal_param_array.restype = None
_fraktal.fraktal_param_array.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_void_p]
def param_array(offset, tex_unit, array):
    return _fraktal.fraktal_param_array(offset, tex_unit, array)


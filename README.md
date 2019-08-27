<img width="128" height="128" src="res/fraktal-logo.svg">

Fraktal is a software library for procedural function-based representations (FRep) aimed at facilitating computer vision and machine learning research. It is designed to aid researchers interested in using the FRep in such applications as:

* Scene reconstruction
* Inverse rendering / procedural modeling / program synthesis
* Learning generative models
* Generating ground-truth synthetic datasets

The core library is a GPU-accelerated function evaluator bundled with a rich standard library utilities, such as:

* Primitives
* Transformations and deformations
* Boolean operators
* Accelerated ray-surface intersection
* Surface normal evaluation
* Physically-based path tracer
* (Depth + Normals + Thickness) map rendering

Fraktal also comes with a graphical application, which lets you visualize and live-edit FReps or adjust camera and scene parameters. The bundled renderer uses a novel de-noising algorithm that lets you create publication-quality figures quickly.

## Requirements

The core library provides GPU-acceleration on the major hardware platforms: NVIDIA, AMD and Intel integrated graphics. At minimum, you need a GPU and a driver with OpenGL 3.1 support (most platforms, including MacBook laptops, meet these requirements).

## Screenshots

The interactive FRep viewer:

<a href="res/mocks/gui.png"><img src="res/mocks/gui.png" style="display:inline-block;width:45%;"></a>
<a href="res/mocks/gui2.png"><img src="res/mocks/gui2.png" style="display:inline-block;width:45%;"></a>
<div style="padding:0;margin:0;">
    <div style="display:inline-block;width:45%;text-align:center;">Creating a figure with custom settings</div>
    <div style="display:inline-block;width:45%;text-align:center;">Creating an alpha-masked depth image</div>
</div>

Example of multi-channel image output:

<img src="res/example_color.png" style="display:inline-block;width:23%;">
<img src="res/example_normals.png" style="display:inline-block;width:23%;">
<img src="res/example_thickness.png" style="display:inline-block;width:23%;">
<img src="res/example_depth.png" style="display:inline-block;width:23%;">
<div style="padding:0;margin:0;">
    <div style="display:inline-block;width:23%;text-align:center;">Color</div>
    <div style="display:inline-block;width:23%;text-align:center;">Normals</div>
    <div style="display:inline-block;width:23%;text-align:center;">Thickness</div>
    <div style="display:inline-block;width:23%;text-align:center;">Depth</div>
</div>

Note that the core library is not limited to evaluating the function densely over an image, but can also be run on arbitrary input sets, such as point clouds or sparse pixels.

## Installation

### C/C++ library
The core library can be compiled from source into a static or dynamic library using the appropriate build script for your platform:

* Windows: build_static_lib.bat or build_dynamic_lib.bat
* Linux/MacOS: make

### GUI
The GUI application can be compiled from source using the appropriate build script for your platform:

* Windows: build_gui.bat
* Linux/MacOS: make


### GUI command line options

The GUI accepts the following command line options:

* `-width`: X resolution of rendered image
* `-height`: Y resolution of rendered image
* `-model`: Path to file containing model kernel definition (default: `libf/vase.f`)
* `-color`: Path to file containing color renderer (default: `libf/publication.f`)
* `-geometry`: Path to file containing geometry renderer (default: `libf/geometry.f`)

### Python bindings
Python bindings can be found in the [python](python) directory. See that directory's readme for installation instructions.

## About Function Representations

A Function Representation (FRep) is a function that converts a 3D coordinate to a scalar, whose sign determines whether the given point is inside or outside the surface. In C, it would have the following declaration:

```
float f(float x, float y, float z)
```

In general f can be an arbitrary computer program, using conditionals, loops, variable declarations and mathematical expression. However, the FRep framework supports a number of high-level modeling operations (which are again defined procedurally), such as

* Geometric primitives
* Boolean combination
* Rigid-body transformation
* Repetition and mirroring
* Bending, twisting and other deformations

These can be combined hierarchically to construct complex shapes or define a library of higher-level primitives that can be reused in other models.

To learn more about FReps, and procedural modeling in general, I recommend the following resources:

* [Wikipedia](https://en.wikipedia.org/wiki/Function_representation)
* [HyperFun](http://www.hyperfun.org/F-rep.html)
* A. Pasko, V. Adzhiev, A. Sourin and V. Savchenko. Function Representation in Geometric Modeling: Concepts, Implementation and Applications. 1995.
* [J. Snyder and J. Kajiya. Generative Modeling: A Symbolic System for Geometric Modeling. 1992. ](https://ohiostate.pressbooks.pub/app/uploads/sites/45/2017/09/generative-snyder-kajiya.pdf)

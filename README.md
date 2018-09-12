# Texturize: A framework for example-based Texture Synthesis

*Texturize* is a framework, that has been created alongside my [master's thesis](https://github.com/Aschratt/Texturize-Thesis). Most of the researchers in the field of texture synthesis do either not provide any implementation samples or have abandoned their sources, so that they are no longer accessible. My motivation was to create a framework, that can be used as a starting point to implement different algorithms and make them comparable, without requiring to re-implement common principles. It provides a unified infrastructure, that allows to customize and mix various aspects of different synthesis algorithms. It follows a modular design that allows to build new modules or customize existing ones. Typical tasks of example-based texture synthesis have been carefully studied and generalized in order to construct a common workflow. That way, I was able to create a prototype synthesizer, that is capable of synthesizing materials for Physically Based Rendering (PBR) from low-resolution exemplars.

The framework must not be confused with the [GIMP PlugIn of the same name](https://github.com/lmanul/gimp-texturize).

## What's it all about?

In general, texture synthesis is a problem field, that researches algorithms, that can create images which are used to add detail to geometric surfaces in computer graphics. Those images are called *textures* and if they are not created by humans, they are called *synthetic textures*. There are two major areas in texture synthesis:

- Procedural synthesizers are algorithms that form new textures from mathematical models.
- Non-Parametric synthesizers take existing texture *exemplars* and aim to create new, visually similar textures with higher resolution.

*Texturize* is a framework for non-parametric synthesizers. Those algorithms have applications in many different computer graphics problems. They are used in image editing (e.g. to fill "holes" or transfer style), video editing and others. When creating this framework, I focused on re-mixing existing textures, i.e. synthesizing new images, that look visually similar to the exemplar, but do not feature heavy repetitions. This can, for example, be used to cover geometric surfaces without visible tiling artifacts.

If you want a more detailed introduction into texture synthesis, consider taking a look int my [thesis](https://github.com/Aschratt/Texturize-Thesis).

## Getting started

Building a synthesizer is a process that involves multiple steps. For convenience, they are described in the [getting started guide](https://aschratt.github.io/Texturize/getting-started.html). A reference implementation, called *Sandbox*, is also provided. You can learn more about it in the [sandbox guide]
(https://aschratt.github.io/Texturize/getting-started-sandbox.html).

## Custom framework builds

In case you want to include the framework into other applications, or require different versions of its dependencies, a custom build might be required. Please refer to [the building guide](https://aschratt.github.io/Texturize/getting-started-build.html) for more information.

## MIT License

> Copyright (c) 2018 Carsten Rudolph
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

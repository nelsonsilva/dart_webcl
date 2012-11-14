# WebCL for Dart

This is heavily based on [node-webcl](https://github.com/fifield/node-webcl/)

## Building

This is Linux only for the moment.

You need to build some OpenCL wrappers so first use the build_wrapper.sh script to checkout and build it.

After this the build.dart should handle building the extension in the Dart editor.

It currently only builds the C files when any of these are changed so if you don't have a libwebcl.so in your lib directory just save one the C files to fire the build.


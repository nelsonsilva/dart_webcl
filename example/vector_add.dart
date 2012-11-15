library lesson1;

import 'dart:math' as Math;
import 'dart:scalarlist';

// TODO - Dart doesn't like package: with native extensions
import 'packages/webcl/webcl.dart' as WebCL;

var clProgramVectorAdd =
'__kernel void ckVectorAdd(__global unsigned int* vectorIn1,\n'
'__global unsigned int* vectorIn2,\n'
'__global unsigned int* vectorOut,\n'
'unsigned int uiVectorWidth) {'
'unsigned int x = get_global_id(0);'
'if (x >= (uiVectorWidth)) return;'
'vectorOut[x] = vectorIn1[x] + vectorIn2[x];'
'return; }';

main() {
  var output = new StringBuffer();
  try {
    
  // Generate input vectors
  var vectorLength = 30;
  var UIvector1 = new Uint32List(vectorLength);    
  var UIvector2 = new Uint32List(vectorLength);
  var outBuffer = new Uint32List(vectorLength);
  var rnd = new Math.Random();
  for ( var i=0; i<vectorLength;  i=i+1) {
      UIvector1[i] = rnd.nextInt(100); //Random number 0..99
      UIvector2[i] = rnd.nextInt(100); //Random number 0..99
  }

  output.add("\nVector length = $vectorLength");

  // Setup WebCL context using the default device of the first platform 
  var platforms = WebCL.platforms;
  var ctx = WebCL.createContextFromType([WebCL.CONTEXT_PLATFORM,  platforms[0]], WebCL.DEVICE_TYPE_GPU);
        
  // Reserve buffers
  var bufSize = vectorLength * 4; // size in bytes
  output.add("\nBuffer size: $bufSize bytes");
  var bufIn1 = ctx.createBuffer(WebCL.MEM_READ_ONLY, UIvector1.lengthInBytes());
  var bufIn2 = ctx.createBuffer(WebCL.MEM_READ_ONLY, UIvector2.lengthInBytes());
  var bufOut = ctx.createBuffer(WebCL.MEM_WRITE_ONLY, outBuffer.lengthInBytes());

  // Create and build program for the first device
  var kernelSrc = clProgramVectorAdd;
  var program = ctx.createProgram(kernelSrc);
  var devices = ctx.getInfo(WebCL.CONTEXT_DEVICES);

  try {
      program.build ([devices[0]], "");
  } catch(e) {
      print ("Failed to build WebCL program. Error "
             "${program.getBuildInfo (devices[0], WebCL.PROGRAM_BUILD_STATUS)}"
              ":  " 
              "${program.getBuildInfo (devices[0], WebCL.PROGRAM_BUILD_LOG)}");
      throw e;
  }

  // Create kernel and set arguments
  var kernel = program.createKernel("ckVectorAdd");
  kernel.setArg (0, bufIn1, WebCL.types.MEM);
  kernel.setArg (1, bufIn2, WebCL.types.MEM);    
  kernel.setArg (2, bufOut, WebCL.types.MEM);
  kernel.setArg (3, vectorLength, WebCL.types.UINT);

  // Create command queue using the first available device
  var cmdQueue = ctx.createCommandQueue (devices[0], 0);

  cmdQueue.enqueueWriteBuffer (bufIn1, false, 0, UIvector1.lengthInBytes(), UIvector1, []);
  cmdQueue.enqueueWriteBuffer (bufIn2, false, 0, UIvector2.lengthInBytes(), UIvector2, []);

  // Init ND-range
  var localWS = [8];
  var globalWS = [(vectorLength / localWS[0]).ceil() * localWS[0]];

  output.add("\nGlobal work item size: $globalWS");
  output.add("\nLocal work item size: $localWS");

  // Execute (enqueue) kernel
  cmdQueue.enqueueNDRangeKernel(kernel, globalWS.length, [], 
                                      globalWS, localWS, []);

  // Read the result buffer from OpenCL device
  
  cmdQueue.enqueueReadBuffer (bufOut, false, 0, outBuffer.lengthInBytes(), outBuffer, []);
  
  cmdQueue.finish(); //Finish all the operations

  //Print input vectors and result vector
  output.add("\nVector1 = "); 
  for (var i = 0; i < vectorLength; i = i + 1) {
      output.add("${UIvector1[i]}, ");
  }
  output.add("\nVector2 = ");
  for (var i = 0; i < vectorLength; i = i + 1) {
    output.add("${UIvector2[i]}, ");
  }
  output.add("\nResult = ");
  for (var i = 0; i < vectorLength; i = i + 1) {
    output.add("${outBuffer[i]}, ");
  }
    
  } catch (e) {
    print("[$e]\nUnfortunately platform or device inquiry failed.");
  }
  
  print(output);
}

library webcl;

import "dart-ext:webcl";
import 'dart:nativewrappers';
import 'dart:scalarlist';

part 'typed_arrays.dart';

class WebCLPlatform extends NativeFieldWrapperClass1 {
  getDevices(type) native "WebCLPlatform_getDevices";
  get allDevices => getDevices(WebCL.DEVICE_TYPE_ALL);
  
  getInfo(param) native "WebCLPlatform_getInfo";
  get name => getInfo(WebCL.PLATFORM_NAME);
  get vendor => getInfo(WebCL.PLATFORM_VENDOR);
  get version => getInfo(WebCL.PLATFORM_VERSION);
  get profile => getInfo(WebCL.PLATFORM_PROFILE);
  get extensions => getInfo(WebCL.PLATFORM_EXTENSIONS);
}

class WebCLDevice extends NativeFieldWrapperClass1 {
  getInfo(param) native "WebCLDevice_getInfo";
  get name => getInfo(WebCL.DEVICE_NAME);
}

class WebCLContext extends NativeFieldWrapperClass1 {
  createBuffer(flags, size) native "WebCLContext_createBuffer";
  createProgram(code) native "WebCLContext_createProgram";
  createCommandQueue(WebCLDevice device, options) native "WebCLContext_createCommandQueue";
  getInfo(param) native "WebCLContext_getInfo";
}

class WebCLMemoryObject extends NativeFieldWrapperClass1 {
}

class WebCLProgram extends NativeFieldWrapperClass1 {
  getBuildInfo(param, WebCLDevice device) native "WebCLProgram_getBuildInfo";
  build(List<WebCLDevice> devices, String options) native "WebCLProgram_build";
  createKernel(String name) native "WebCLProgram_createKernel";
}

class WebCLKernel extends NativeFieldWrapperClass1 {
  setArg(index, value, type) native "WebCLKernel_setArg";
}

class WebCLCommandQueue extends NativeFieldWrapperClass1 {
  enqueueWriteBuffer(buffer, bool blockingWrite, num offset, num size, data, [eventWaitList]) native "WebCLCommandQueue_enqueueWriteBuffer";
  enqueueReadBuffer(buffer, bool blockingRead, num offset, num size, data, [eventWaitList]) native "WebCLCommandQueue_enqueueReadBuffer";
  enqueueNDRangeKernel(WebCLKernel kernel, num workDim, List<num> globalWorkOffset, List<num> globalWorkSize, List<num> localWorkSize, [eventWaitList]) native "WebCLCommandQueue_enqueueNDRangeKernel";
  finish() native "WebCLCommandQueue_finish";
}

class WebCLEvent extends NativeFieldWrapperClass1 {
  
}

class WebCLException implements Exception {
  final String message;
  WebCLException._internal(String this.message);
  toString() => "WebCLExceptionException: $message";
}

class WebCL {
  static createContextFromType(properties, type) native "CreateContextFromType";
  static get platforms native "GetPlatforms";


  /* cl_device_info */
  static const DEVICE_NAME                              =0x102B;
  
  /* cl_device_type - bitfield */
  static const DEVICE_TYPE_DEFAULT                     = (1 << 0);
  static const DEVICE_TYPE_CPU                         = (1 << 1);
  static const DEVICE_TYPE_GPU                         = (1 << 2);
  static const DEVICE_TYPE_ACCELERATOR                 = (1 << 3);
  static const DEVICE_TYPE_ALL                         = 0xFFFFFFFF;
  
  /* cl_platform_info */
  static const PLATFORM_PROFILE                         =0x0900;
  static const PLATFORM_VERSION                         =0x0901;
  static const PLATFORM_NAME                            =0x0902;
  static const PLATFORM_VENDOR                          =0x0903;
  static const PLATFORM_EXTENSIONS                      =0x0904;
  
  /* cl_context_info  */
  static const CONTEXT_REFERENCE_COUNT                  =0x1080;
  static const CONTEXT_DEVICES                          =0x1081;
  static const CONTEXT_PROPERTIES                       =0x1082;
  static const CONTEXT_NUM_DEVICES                      =0x1083;
  
  /* cl_context_info + cl_context_properties */
  static const CONTEXT_PLATFORM                         =0x1084;
  
  /* cl_mem_flags - bitfield */
  static const MEM_READ_WRITE                           =(1 << 0);
  static const MEM_WRITE_ONLY                           =(1 << 1);
  static const MEM_READ_ONLY                            =(1 << 2);
  
  /* cl_program_build_info */
  static const PROGRAM_BUILD_STATUS                     =0x1181;
  static const PROGRAM_BUILD_OPTIONS                    =0x1182;
  static const PROGRAM_BUILD_LOG                        =0x1183;
  
  static var types = new _WebCLTypes();
}

class _WebCLTypes {
  const UINT = 7;
  const MEM = 20;
}
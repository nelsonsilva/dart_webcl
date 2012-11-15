library webcl;

import "dart-ext:webcl";
import 'dart:nativewrappers';

createContextFromType(properties, type) native "CreateContextFromType";

get platforms native "GetPlatforms";

class WebCLPlatform extends NativeFieldWrapperClass1 {
  getDevices(type) native "WebCLPlatform_getDevices";
  get allDevices => getDevices(DEVICE_TYPE_ALL);
  
  getInfo(param) native "WebCLPlatform_getInfo";
  get name => getInfo(PLATFORM_NAME);
  get vendor => getInfo(PLATFORM_VENDOR);
  get version => getInfo(PLATFORM_VERSION);
  get profile => getInfo(PLATFORM_PROFILE);
  get extensions => getInfo(PLATFORM_EXTENSIONS);
}

class WebCLDevice extends NativeFieldWrapperClass1 {
  getInfo(param) native "WebCLDevice_getInfo";
  get name => getInfo(DEVICE_NAME);
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

/* cl_device_info */
const DEVICE_NAME                              =0x102B;

/* cl_device_type - bitfield */
const DEVICE_TYPE_DEFAULT                     = (1 << 0);
const DEVICE_TYPE_CPU                         = (1 << 1);
const DEVICE_TYPE_GPU                         = (1 << 2);
const DEVICE_TYPE_ACCELERATOR                 = (1 << 3);
const DEVICE_TYPE_ALL                         = 0xFFFFFFFF;

/* cl_platform_info */
const PLATFORM_PROFILE                         =0x0900;
const PLATFORM_VERSION                         =0x0901;
const PLATFORM_NAME                            =0x0902;
const PLATFORM_VENDOR                          =0x0903;
const PLATFORM_EXTENSIONS                      =0x0904;

/* cl_context_info  */
const CONTEXT_REFERENCE_COUNT                  =0x1080;
const CONTEXT_DEVICES                          =0x1081;
const CONTEXT_PROPERTIES                       =0x1082;
const CONTEXT_NUM_DEVICES                      =0x1083;

/* cl_context_info + cl_context_properties */
const CONTEXT_PLATFORM                         =0x1084;

/* cl_mem_flags - bitfield */
const MEM_READ_WRITE                           =(1 << 0);
const MEM_WRITE_ONLY                           =(1 << 1);
const MEM_READ_ONLY                            =(1 << 2);

/* cl_program_build_info */
const PROGRAM_BUILD_STATUS                     =0x1181;
const PROGRAM_BUILD_OPTIONS                    =0x1182;
const PROGRAM_BUILD_LOG                        =0x1183;

var types = new _Types();

class _Types {
  const UINT = 7;
  const MEM = 20;
}
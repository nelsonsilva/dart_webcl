library webcl;

import "dart-ext:webcl";
import 'dart:nativewrappers';

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

String getHello() native "GetHello";

get platforms native "GetPlatforms";

class Platform extends NativeFieldWrapperClass1 {
  getDevices(type) native "Platform_GetDevices";
  get allDevices => getDevices(DEVICE_TYPE_ALL);
  
  getInfo(param) native "Platform_GetInfo";
  get name => getInfo(PLATFORM_NAME);
  get vendor => getInfo(PLATFORM_VENDOR);
  get version => getInfo(PLATFORM_VERSION);
  get profile => getInfo(PLATFORM_PROFILE);
  get extensions => getInfo(PLATFORM_EXTENSIONS);
}

class Device extends NativeFieldWrapperClass1 {
  getInfo(param) native "Device_GetInfo";
  get name => getInfo(DEVICE_NAME);
}

class WebCLException implements Exception {
  final String message;
  WebCLException._internal(String this.message);
  toString() => "WebCLExceptionException: $message";
}

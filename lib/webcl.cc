#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dart_api.h"
#include "wrapper/include/platformwrapper.h"
#include "wrapper/include/devicewrapper.h"

#define LOG(msg) fprintf(stderr,msg);

static Dart_Handle library;

void Throw(const char* message) {
  Dart_Handle messageHandle = Dart_NewStringFromCString(message);
  Dart_Handle exceptionClass = Dart_GetClass(library, Dart_NewStringFromCString("WebCLException"));
  Dart_ThrowException(Dart_New(exceptionClass, Dart_NewStringFromCString("_internal"), 1, &messageHandle));
}

#define WEBCL_COND_RETURN_THROW(error) if (ret == error) return Throw("#error");

Dart_NativeFunction ResolveName(Dart_Handle name, int argc);

DART_EXPORT Dart_Handle webcl_Init(Dart_Handle parent_library) {
  if (Dart_IsError(parent_library)) { return parent_library; }

  Dart_Handle result_code = Dart_SetNativeResolver(parent_library, ResolveName);
  if (Dart_IsError(result_code)) return result_code;

  library = Dart_NewPersistentHandle(parent_library);
  
  return Dart_Null();
}

Dart_Handle HandleError(Dart_Handle handle) {
  if (Dart_IsError(handle)) Dart_PropagateError(handle);
  return handle;
}

void Platform_destroy(Dart_Handle handle, void* pw) {
  static_cast<PlatformWrapper *>(pw)->release();
}

Dart_Handle Platform_create(PlatformWrapper* pw) {

  Dart_Handle cls = HandleError(Dart_GetClass(library, Dart_NewStringFromCString("Platform")));
  Dart_Handle instance = HandleError(Dart_New(cls, Dart_Null(), 0, 0));  // instance is Dart wrapper instance.

  Dart_SetNativeInstanceField(instance, 0, reinterpret_cast<intptr_t>(pw));

  // Now register a week callback for it.
  Dart_NewWeakPersistentHandle(instance, pw, &Platform_destroy);
  
  return instance;
}

void Device_destroy(Dart_Handle handle, void* dw) {
  static_cast<DeviceWrapper *>(dw)->release();
}

Dart_Handle Device_create(DeviceWrapper* dw) {
  Dart_Handle cls = Dart_GetClass(library, Dart_NewStringFromCString("Device"));
  Dart_Handle instance = Dart_New(cls, Dart_Null(), 0, 0);  // instance is Dart wrapper instance.

  Dart_SetNativeInstanceField(instance, 0, reinterpret_cast<intptr_t>(dw));

  // Now register a week callback for it.
  Dart_NewWeakPersistentHandle(instance, dw, &Device_destroy);

  return instance;
}

void GetPlatforms(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	std::vector<PlatformWrapper*> platforms;
	cl_int ret = PlatformWrapper::getPlatforms(platforms);
	if (ret != CL_SUCCESS) {
		if (ret == CL_INVALID_VALUE) Throw("CL_INVALID_VALUE");
		if (ret == CL_OUT_OF_HOST_MEMORY) Throw("CL_OUT_OF_HOST_MEMORY");
		Throw("UNKNOWN ERROR");
	    return;
	}

	int length = platforms.size();

	Dart_Handle result = Dart_NewList(length);

	for (int i=0; i<length; i++) {
	  Dart_Handle platform = Platform_create(platforms[i]);
	  Dart_ListSetAt(result, i, platform);
	}
      
	Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
}
  
void Platform_GetInfo(Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
	Dart_Handle instance = Dart_GetNativeArgument(arguments, 0);
	int count;
	Dart_GetNativeInstanceFieldCount(instance, &count);

	intptr_t ptr;
	Dart_GetNativeInstanceField(instance, 0, &ptr);
	PlatformWrapper* pw = (PlatformWrapper*) ptr;
	

	Dart_Handle param_name_arg = Dart_GetNativeArgument(arguments, 1);
    int64_t param_name;                                                
    Dart_IntegerToInt64(param_name_arg, &param_name);                                            

    size_t param_value_size_ret = 0;
    char param_value[1024];
    cl_int ret = PlatformWrapper::platformInfoHelper(pw,
						     param_name,
						     sizeof(param_value),
						     param_value,
						     &param_value_size_ret);
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_PLATFORM);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
	
	Dart_Handle result = HandleError(Dart_NewStringFromCString(param_value));
	
	Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
    
}
  
void Device_GetInfo(Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
	Dart_Handle instance = Dart_GetNativeArgument(arguments, 0);
	int count;
	Dart_GetNativeInstanceFieldCount(instance, &count);

	intptr_t ptr;
	Dart_GetNativeInstanceField(instance, 0, &ptr);
	DeviceWrapper* dw = (DeviceWrapper*) ptr;
	

	Dart_Handle param_name_arg = Dart_GetNativeArgument(arguments, 1);
    int64_t param_name;                                                
    Dart_IntegerToInt64(param_name_arg, &param_name);                                            

    size_t param_value_size_ret = 0;
    char param_value[1024];
    cl_int ret = DeviceWrapper::deviceInfoHelper(dw,
						     param_name,
						     sizeof(param_value),
						     param_value,
						     &param_value_size_ret);
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
	
	Dart_Handle result;
	
	switch (param_name) {
	    case CL_DEVICE_NAME:
	    case CL_DEVICE_VENDOR:
	    case CL_DRIVER_VERSION:
	    case CL_DEVICE_PROFILE:
	    case CL_DEVICE_VERSION:
	    case CL_DEVICE_OPENCL_C_VERSION:
	    case CL_DEVICE_EXTENSIONS:
	        result = HandleError(Dart_NewStringFromCString(param_value));
	    	break;
	    default:
	    	result = HandleError(Dart_NewInteger(*(size_t*)param_value));
    }
    
	Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
    
}

void Platform_GetDevices(Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
	Dart_Handle instance = Dart_GetNativeArgument(arguments, 0);
	
	intptr_t ptr;
	Dart_GetNativeInstanceField(instance, 0, &ptr);
	PlatformWrapper* pw = reinterpret_cast<PlatformWrapper*>(ptr);
                                             
    Dart_Handle device_type_arg = Dart_GetNativeArgument(arguments, 1);
    int64_t device_type;                                                
    Dart_IntegerToInt64(device_type_arg, &device_type);
                                                     
    std::vector<DeviceWrapper*> devices;
	
    cl_int ret = pw->getDevices((int) device_type, devices);
	
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_PLATFORM);
		WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE_TYPE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_DEVICE_NOT_FOUND);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }

	int length = devices.size();
	Dart_Handle result = Dart_NewList(length);
	Dart_Handle dartDevices[length];
	for (int i=0; i<length; i++) {
		Dart_ListSetAt(result, i, Device_create(devices[i]));
	}

    Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
}

void GetHello(Dart_NativeArguments arguments) {
  Dart_EnterScope();
  Dart_Handle result = HandleError(Dart_NewStringFromCString("Hello from WebCL!"));
  Dart_SetReturnValue(arguments, result);
  Dart_ExitScope();
}

struct FunctionLookup {
  const char* name;
  Dart_NativeFunction function;
};

FunctionLookup function_list[] = {
    {"GetHello", GetHello},
    {"GetPlatforms", GetPlatforms},
    {"Platform_GetDevices", Platform_GetDevices},
    {"Platform_GetInfo", Platform_GetInfo},  
    {"Device_GetInfo", Device_GetInfo},    
    {NULL, NULL}};

Dart_NativeFunction ResolveName(Dart_Handle name, int argc) {
  if (!Dart_IsString(name)) return NULL;
  Dart_NativeFunction result = NULL;
  Dart_EnterScope();
  const char* cname;
  HandleError(Dart_StringToCString(name, &cname));

  for (int i=0; function_list[i].name != NULL; ++i) {
    if (strcmp(function_list[i].name, cname) == 0) {
      result = function_list[i].function;
      break;
    }
  }
  Dart_ExitScope();
  return result;
}

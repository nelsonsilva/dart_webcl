#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dart_api.h"

#define D_LOG(level, msg) fprintf(stderr,msg);
#include "wrapper/include/clwrappercommon.h"
#include "wrapper/include/platformwrapper.h"
#include "wrapper/include/devicewrapper.h"
#include "wrapper/include/contextwrapper.h"
#include "wrapper/include/memoryobjectwrapper.h"
#include "wrapper/include/programwrapper.h"
#include "wrapper/include/kernelwrapper.h"
#include "wrapper/include/commandqueuewrapper.h"
#include "wrapper/include/eventwrapper.h"
#include "wrapper/include/clwrappertypes.h"

#define LOG(msg) fprintf(stderr,msg);

#define DEFINE_NATIVE_CLASS(CLASS, WRAPPER_CLASS) \
WRAPPER_CLASS* CLASS##_unwrap(Dart_Handle instance) { \
	intptr_t ptr; \
	Dart_GetNativeInstanceField(instance, 0, &ptr); \
	return (WRAPPER_CLASS *) ptr; \
}\
void CLASS##_destroy(Dart_Handle handle, void* w) { \
  static_cast<WRAPPER_CLASS *>(w)->release(); \
}\
\
Dart_Handle CLASS##_create(WRAPPER_CLASS* w) { \
  Dart_Handle cls = HandleError(Dart_GetClass(library, Dart_NewStringFromCString(#CLASS))); \
  Dart_Handle instance = HandleError(Dart_New(cls, Dart_Null(), 0, 0)); \
  Dart_SetNativeInstanceField(instance, 0, reinterpret_cast<intptr_t>(w)); \
  Dart_NewWeakPersistentHandle(instance, w, &CLASS##_destroy); \
  return instance; \
}


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

DEFINE_NATIVE_CLASS(WebCLPlatform, PlatformWrapper)
DEFINE_NATIVE_CLASS(WebCLDevice, DeviceWrapper)
DEFINE_NATIVE_CLASS(WebCLContext, ContextWrapper)
DEFINE_NATIVE_CLASS(WebCLMemoryObject, MemoryObjectWrapper)
DEFINE_NATIVE_CLASS(WebCLProgram, ProgramWrapper)
DEFINE_NATIVE_CLASS(WebCLKernel, KernelWrapper)
DEFINE_NATIVE_CLASS(WebCLCommandQueue, CommandQueueWrapper)
DEFINE_NATIVE_CLASS(WebCLEvent, EventWrapper)

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
	  Dart_Handle platform = WebCLPlatform_create(platforms[i]);
	  Dart_ListSetAt(result, i, platform);
	}
      
	Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
}
  
void CreateContextFromType(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	
	Dart_Handle properties_list = Dart_GetNativeArgument(arguments, 0);
	intptr_t length;
	Dart_ListLength(properties_list, &length);
	
	std::vector<std::pair<cl_context_properties, cl_context_properties> > properties;
	for (int i=0; i<length; i+=2) {
		int64_t context_property;                                                
    	Dart_IntegerToInt64(Dart_ListGetAt(properties_list, i), &context_property);     
		
		PlatformWrapper * pw = WebCLPlatform_unwrap(Dart_ListGetAt(properties_list, i+1));   
		
		std::pair<cl_context_properties, cl_context_properties> p(context_property, (cl_context_properties) pw->getWrapped());
		
		properties.push_back(p);
	}
	
	int64_t device_type;
	Dart_IntegerToInt64(Dart_GetNativeArgument(arguments, 1), &device_type);
	
	ContextWrapper *cw = 0;
	cl_int ret = ContextWrapper::createContextFromType(properties, (cl_device_type) device_type, NULL,NULL, &cw);

	if (ret != CL_SUCCESS) {
	    WEBCL_COND_RETURN_THROW(CL_INVALID_PLATFORM);
	    WEBCL_COND_RETURN_THROW(CL_INVALID_PROPERTY);
	    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
	    WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE_TYPE);
	    WEBCL_COND_RETURN_THROW(CL_DEVICE_NOT_AVAILABLE);
	    WEBCL_COND_RETURN_THROW(CL_DEVICE_NOT_FOUND);
	    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
	    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
	    return Throw("UNKNOWN ERROR");
	}
	
	Dart_Handle result = WebCLContext_create(cw);
	Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
}

void WebCLPlatform_getInfo(Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
	PlatformWrapper * pw = WebCLPlatform_unwrap(Dart_GetNativeArgument(arguments, 0)); 

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
  
void WebCLPlatform_getDevices(Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
    PlatformWrapper * pw = WebCLPlatform_unwrap(Dart_GetNativeArgument(arguments, 0)); 
                                             
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

	for (int i=0; i<length; i++) {
		Dart_ListSetAt(result, i, WebCLDevice_create(devices[i]));
	}

    Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
}

void WebCLDevice_getInfo(Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
	DeviceWrapper * dw = WebCLDevice_unwrap(Dart_GetNativeArgument(arguments, 0)); 

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

void WebCLContext_createBuffer(Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
	ContextWrapper * cw = WebCLContext_unwrap(Dart_GetNativeArgument(arguments, 0));
	
    int64_t flags;                                                
    Dart_IntegerToInt64(Dart_GetNativeArgument(arguments, 1), &flags);
    
    int64_t size;                                                
    Dart_IntegerToInt64(Dart_GetNativeArgument(arguments, 2), &size);
    
    MemoryObjectWrapper *mw = 0;
    cl_int ret = cw->createBuffer(flags, size, 0, &mw);

    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_BUFFER_SIZE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_HOST_PTR);
		WEBCL_COND_RETURN_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
    
    Dart_Handle result = WebCLMemoryObject_create(mw);
    Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
}
	
void WebCLContext_createProgram(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	ContextWrapper * cw = WebCLContext_unwrap(Dart_GetNativeArgument(arguments, 0));
	
	Dart_Handle code_arg = Dart_GetNativeArgument(arguments, 1);
	intptr_t length = 0;
 	Dart_StringLength(code_arg, &length);
	const char *code = new char[length];
	Dart_StringToCString(code_arg, &code);
	std::string cpp_str(code);
	//delete[] code;
	
	ProgramWrapper *pw = 0;
	cl_int ret = cw->createProgramWithSource(cpp_str, &pw);
	if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
    Dart_Handle result = WebCLProgram_create(pw);
    Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
}

void WebCLContext_createCommandQueue(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	ContextWrapper * cw = WebCLContext_unwrap(Dart_GetNativeArgument(arguments, 0));
	DeviceWrapper * dw = WebCLDevice_unwrap(Dart_GetNativeArgument(arguments, 1)); 
	
	int64_t properties;                                                
    Dart_IntegerToInt64(Dart_GetNativeArgument(arguments, 1), &properties);
    
    CommandQueueWrapper *qw = 0;
    
    cl_int ret = cw->createCommandQueue(dw, properties, &qw);

    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_QUEUE_PROPERTIES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }

    Dart_Handle result = WebCLCommandQueue_create(qw);
    Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
}

void WebCLContext_getInfo(Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
	ContextWrapper * cw = WebCLContext_unwrap(Dart_GetNativeArgument(arguments, 0)); 

    int64_t param_name;                                                
    Dart_IntegerToInt64(Dart_GetNativeArgument(arguments, 1), &param_name);                                            

    size_t param_value_size_ret = 0;
    char param_value[1024];
    cl_int ret = ContextWrapper::contextInfoHelper(cw,
						   param_name,
						   sizeof(param_value),
						   param_value,
						   &param_value_size_ret);
						   
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
	
	Dart_Handle result;
	
	switch (param_name) {
	    case CL_CONTEXT_REFERENCE_COUNT:
	    case CL_CONTEXT_NUM_DEVICES:
	    	result = HandleError(Dart_NewInteger(*(cl_uint*)param_value));
	    	break;
	    case CL_CONTEXT_DEVICES: {
			size_t num_devices = param_value_size_ret / sizeof(cl_device_id);
			result = Dart_NewList(num_devices);

			for (uint i=0; i<num_devices; i++) {
				cl_device_id d = ((cl_device_id*)param_value)[i];
			    DeviceWrapper *dw = new DeviceWrapper(d);
				Dart_ListSetAt(result, i,WebCLDevice_create(dw));
			}
			break; }
	    case CL_CONTEXT_PROPERTIES:
			return Throw("CL_CONTEXT_PROPERTIES unimplemented");
	    default:
			return Throw("UNKNOWN param_name");    	
    }
    
	Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();
    
}

void WebCLProgram_getBuildInfo (Dart_NativeArguments arguments) {
    
	Dart_EnterScope();
	ProgramWrapper * pw = WebCLProgram_unwrap(Dart_GetNativeArgument(arguments, 0));
	DeviceWrapper * dw = WebCLDevice_unwrap(Dart_GetNativeArgument(arguments, 1));
	
	int64_t param_name;                                                
    Dart_IntegerToInt64(Dart_GetNativeArgument(arguments, 2), &param_name);  
    
    size_t param_value_size_ret = 0;
    char param_value[4096];
    
    cl_int ret = ProgramWrapper::programBuildInfoHelper(pw,
							dw,
							param_name,
							sizeof(param_value),
							param_value,
							&param_value_size_ret);
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
    
    Dart_Handle result;
	
	switch (param_name) {
		case CL_PROGRAM_BUILD_STATUS:
			result = HandleError(Dart_NewInteger(*(size_t*)param_value));
			break;
    	default:
    		result = Dart_NewStringFromCString(param_value);
	}
	
    Dart_SetReturnValue(arguments, result);
    Dart_ExitScope();    
}

void WebCLProgram_build(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	   	   
	ProgramWrapper * pw = WebCLProgram_unwrap(Dart_GetNativeArgument(arguments, 0));
	
	Dart_Handle device_list = Dart_GetNativeArgument(arguments, 1);
	Dart_Handle options_string = Dart_GetNativeArgument(arguments, 2);
	
	if (!Dart_IsList(device_list))
	    Throw("CL_INVALID_VALUE");
    if (!Dart_IsString(options_string))
	    Throw("CL_INVALID_VALUE");
	
	intptr_t length;
	Dart_ListLength(device_list, &length);
	
	std::vector<DeviceWrapper*> devices;
	for (int i=0; i<length; i++) {                                               
    	DeviceWrapper * dw = WebCLDevice_unwrap(Dart_ListGetAt(device_list, i));     
		devices.push_back(dw);
	}
	    
	intptr_t options_length = 0;
 	Dart_StringLength(options_string, &options_length);
	const char *c_str = new char[options_length];
	Dart_StringToCString(options_string, &c_str);
	std::string cpp_str(c_str);
	//delete[] c_str;
	
	cl_int ret = pw->buildProgram(devices, cpp_str, 0, 0);

    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_BINARY);
		WEBCL_COND_RETURN_THROW(CL_INVALID_BUILD_OPTIONS);
		WEBCL_COND_RETURN_THROW(CL_INVALID_OPERATION);
		WEBCL_COND_RETURN_THROW(CL_COMPILER_NOT_AVAILABLE);
		WEBCL_COND_RETURN_THROW(CL_BUILD_PROGRAM_FAILURE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
    
    Dart_SetReturnValue(arguments, Dart_Null());
    Dart_ExitScope(); 
}

void WebCLProgram_createKernel(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	   	   
	ProgramWrapper * pw = WebCLProgram_unwrap(Dart_GetNativeArgument(arguments, 0));
	
	Dart_Handle name_arg = Dart_GetNativeArgument(arguments, 1);
	
	intptr_t name_length = 0;
 	Dart_StringLength(name_arg, &name_length);
	const char *c_str = new char[name_length];
	Dart_StringToCString(name_arg, &c_str);
	std::string cpp_str(c_str);
	//delete[] c_str;
	
	KernelWrapper *kw = 0;
	cl_int ret = pw->createKernel(cpp_str, &kw);
    
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
		WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM_EXECUTABLE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_KERNEL_NAME);
		WEBCL_COND_RETURN_THROW(CL_INVALID_KERNEL_DEFINITION);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
    
    Dart_SetReturnValue(arguments, WebCLKernel_create(kw));
    Dart_ExitScope(); 
}
	

void WebCLKernel_setArg(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	   	   
	KernelWrapper * kw = WebCLKernel_unwrap(Dart_GetNativeArgument(arguments, 0));
	
	Dart_Handle index_arg = Dart_GetNativeArgument(arguments, 1);
	
	if (!Dart_IsInteger(index_arg))
	    Throw("CL_INVALID_ARG_INDEX");
	
	
    int64_t arg_index;
    Dart_IntegerToInt64(index_arg, &arg_index); 
    
    int64_t type = 0;
    Dart_IntegerToInt64(Dart_GetNativeArgument(arguments, 3), &type); 
    
    size_t arg_size = 0;
    
    void *arg_value = 0;
    cl_int ret;

	Dart_Handle value_arg = Dart_GetNativeArgument(arguments, 2);
	
    switch (type) {
	    case types::MEMORY_OBJECT: {
			cl_mem mem;
			if (Dart_IsInteger(value_arg)) {
			    int64_t ptr;
			    Dart_IntegerToInt64(value_arg, &ptr);
			    if (ptr)
					return Throw("ARG is not of specified type");
			    mem = 0;
			} else {
				
			    MemoryObjectWrapper *mw = WebCLMemoryObject_unwrap(value_arg);
			    mem = mw->getWrapped();
			}
			arg_value = &mem;
			arg_size = sizeof(mem);
			ret = kw->setArg(arg_index, arg_size, arg_value);
			break; 
	    }
	    case types::UINT: {
			if (!Dart_IsInteger(value_arg))
			    return Throw("ARG is not of specified type");
			uint64_t arg64;
			Dart_IntegerToUint64(value_arg, &arg64);
			cl_uint arg = (cl_uint) arg64;
			arg_value = &arg;
			arg_size = sizeof(arg);
			ret = kw->setArg(arg_index, arg_size, arg_value);
			break;
		}
	    // TODO - Add the other case
	    case types::UNKNOWN:
	    default:
			return Throw("UNKNOWN TYPE");
    }

    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_KERNEL);
		WEBCL_COND_RETURN_THROW(CL_INVALID_ARG_INDEX);
		WEBCL_COND_RETURN_THROW(CL_INVALID_ARG_VALUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_SAMPLER);
		WEBCL_COND_RETURN_THROW(CL_INVALID_ARG_SIZE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }

    Dart_SetReturnValue(arguments, Dart_Null());
    Dart_ExitScope();    
}

void WebCLCommandQueue_enqueueNDRangeKernel(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	   	   
	CommandQueueWrapper * qw = WebCLCommandQueue_unwrap(Dart_GetNativeArgument(arguments, 0));
	
	KernelWrapper * kw = WebCLKernel_unwrap(Dart_GetNativeArgument(arguments, 1));
	
	int64_t work_dim;
	Dart_IntegerToInt64(Dart_GetNativeArgument(arguments, 2), &work_dim);

    Dart_Handle globalWorkOffset = Dart_GetNativeArgument(arguments, 3);
    intptr_t globalWorkOffsetLength;
	Dart_ListLength(globalWorkOffset, &globalWorkOffsetLength);

	int64_t s;
	
	std::vector<size_t> global_work_offset;
	 
    for (int i=0; i<globalWorkOffsetLength; i++) {	
		Dart_IntegerToInt64(Dart_ListGetAt(globalWorkOffset, i), &s);
		global_work_offset.push_back(s);
    }

	Dart_Handle globalWorkSize = Dart_GetNativeArgument(arguments, 4);
    intptr_t globalWorkSizeLength;
	Dart_ListLength(globalWorkSize, &globalWorkSizeLength);
	
    std::vector<size_t> global_work_size;

	for (int i=0; i<globalWorkSizeLength; i++) {	
		Dart_IntegerToInt64(Dart_ListGetAt(globalWorkSize, i), &s);
		global_work_size.push_back(s);
    }
    
	Dart_Handle localWorkSize = Dart_GetNativeArgument(arguments, 5);
    intptr_t localWorkSizeLength;
	Dart_ListLength(localWorkSize, &localWorkSizeLength);
	
    std::vector<size_t> local_work_size;

	for (int i=0; i<localWorkSizeLength; i++) {	
		Dart_IntegerToInt64(Dart_ListGetAt(localWorkSize, i), &s);
		local_work_size.push_back(s);
    }
    
	Dart_Handle eventWaitArray = Dart_GetNativeArgument(arguments, 6);
    intptr_t eventWaitArrayLength;
	Dart_ListLength(eventWaitArray, &eventWaitArrayLength);
	
    std::vector<EventWrapper*> event_wait_list;

	for (int i=0; i<eventWaitArrayLength; i++) {	
		event_wait_list.push_back(WebCLEvent_unwrap(Dart_ListGetAt(eventWaitArray, i)));
    }
    

    EventWrapper *event = 0;
    cl_int ret = qw->enqueueNDRangeKernel(kw,
								    work_dim,
								    global_work_offset,
								    global_work_size,
								    local_work_size,
								    event_wait_list,
								    &event);
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM_EXECUTABLE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_COMMAND_QUEUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_KERNEL);
		WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_KERNEL_ARGS);
		WEBCL_COND_RETURN_THROW(CL_INVALID_WORK_DIMENSION);
		WEBCL_COND_RETURN_THROW(CL_INVALID_GLOBAL_OFFSET);
		WEBCL_COND_RETURN_THROW(CL_INVALID_WORK_GROUP_SIZE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_WORK_ITEM_SIZE);
		WEBCL_COND_RETURN_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
		WEBCL_COND_RETURN_THROW(CL_INVALID_IMAGE_SIZE);
		WEBCL_COND_RETURN_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		WEBCL_COND_RETURN_THROW(CL_INVALID_EVENT_WAIT_LIST);
		return Throw("UNKNOWN ERROR");
    }

	Dart_SetReturnValue(arguments, WebCLEvent_create(event));
    Dart_ExitScope();    
}

void getTypedArrayData(Dart_Handle array, void **ptr) {

    Dart_Handle type = Dart_GetClass(library, Dart_NewStringFromCString("TypedArray"));
    bool isTypedArray;
    Dart_ObjectIsType(array, type, &isTypedArray);
                 
    if (isTypedArray) {
    	array = Dart_GetField(array, Dart_NewStringFromCString("_list8"));
    }                    
     
    if ( Dart_IsByteArrayExternal(array)) {
   		Dart_ExternalByteArrayGetData(array, ptr);
   	} else { 
   		LOG("\nIT'S NOT AN EXTERNAL BYTE ARRAY :/");
   	}
}

void WebCLCommandQueue_enqueueWriteBuffer(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	   	   
	CommandQueueWrapper * qw = WebCLCommandQueue_unwrap(Dart_GetNativeArgument(arguments, 0));
    
    MemoryObjectWrapper * mw = WebCLMemoryObject_unwrap(Dart_GetNativeArgument(arguments, 1));
    
    // TODO: arg checking
    bool blocking_write_flag;
    Dart_BooleanValue(Dart_GetNativeArgument(arguments, 2), &blocking_write_flag);
    
    cl_bool blocking_write =  blocking_write_flag ? CL_TRUE : CL_FALSE;
    
    uint64_t offset;
    Dart_IntegerToUint64(Dart_GetNativeArgument(arguments, 3), &offset);
  
  	uint64_t cb;
    Dart_IntegerToUint64(Dart_GetNativeArgument(arguments, 4), &cb);
    
    void *ptr;
    getTypedArrayData(Dart_GetNativeArgument(arguments, 5), &ptr);

	Dart_Handle eventWaitArray = Dart_GetNativeArgument(arguments, 6);
    intptr_t eventWaitArrayLength;
	Dart_ListLength(eventWaitArray, &eventWaitArrayLength);
	
    std::vector<EventWrapper*> event_wait_list;

	for (int i=0; i<eventWaitArrayLength; i++) {	
		event_wait_list.push_back(WebCLEvent_unwrap(Dart_ListGetAt(eventWaitArray, i)));
    }

    EventWrapper *event = 0;
    cl_int ret = qw->enqueueWriteBuffer(mw,
								  blocking_write,
								  offset,
								  cb,
								  ptr,
								  event_wait_list,
								  &event);
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_COMMAND_QUEUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_EVENT_WAIT_LIST);
		WEBCL_COND_RETURN_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
		WEBCL_COND_RETURN_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
		WEBCL_COND_RETURN_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
    
    Dart_SetReturnValue(arguments, WebCLEvent_create(event));
    Dart_ExitScope();  

}

void WebCLCommandQueue_enqueueReadBuffer(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	   	   
	CommandQueueWrapper * qw = WebCLCommandQueue_unwrap(Dart_GetNativeArgument(arguments, 0));
    
    MemoryObjectWrapper * mw = WebCLMemoryObject_unwrap(Dart_GetNativeArgument(arguments, 1));

    
    // TODO: arg checking
    bool blocking_read_flag;
    Dart_BooleanValue(Dart_GetNativeArgument(arguments, 2), &blocking_read_flag);
    
    cl_bool blocking_read =  blocking_read_flag ? CL_TRUE : CL_FALSE;
    
    uint64_t offset;
    Dart_IntegerToUint64(Dart_GetNativeArgument(arguments, 3), &offset);
  
  	uint64_t cb;
    Dart_IntegerToUint64(Dart_GetNativeArgument(arguments, 4), &cb);
    
    void *ptr;
    getTypedArrayData(Dart_GetNativeArgument(arguments, 5), &ptr);

	Dart_Handle eventWaitArray = Dart_GetNativeArgument(arguments, 6);
    intptr_t eventWaitArrayLength;
	Dart_ListLength(eventWaitArray, &eventWaitArrayLength);
	
    std::vector<EventWrapper*> event_wait_list;

	for (int i=0; i<eventWaitArrayLength; i++) {	
		event_wait_list.push_back(WebCLEvent_unwrap(Dart_ListGetAt(eventWaitArray, i)));
    }

    EventWrapper *event = 0;
    cl_int ret = qw->enqueueReadBuffer(mw,
								 blocking_read,
								 offset,
								 cb,
								 ptr,
								 event_wait_list,
								 &event);
    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_COMMAND_QUEUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
		WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
		WEBCL_COND_RETURN_THROW(CL_INVALID_EVENT_WAIT_LIST);
		WEBCL_COND_RETURN_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
		WEBCL_COND_RETURN_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
		WEBCL_COND_RETURN_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
    
    Dart_SetReturnValue(arguments, WebCLEvent_create(event));
    Dart_ExitScope();
}
	
void WebCLCommandQueue_finish(Dart_NativeArguments arguments) {
	Dart_EnterScope();
	   	   
	CommandQueueWrapper * qw = WebCLCommandQueue_unwrap(Dart_GetNativeArgument(arguments, 0));

    cl_int ret = qw->finish();

    if (ret != CL_SUCCESS) {
		WEBCL_COND_RETURN_THROW(CL_INVALID_COMMAND_QUEUE);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
		WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
		return Throw("UNKNOWN ERROR");
    }
    
 	Dart_SetReturnValue(arguments, Dart_Null());
    Dart_ExitScope();
}

void ExternalList_Free(void* buffer) {
  delete[] reinterpret_cast<uint8_t*>(buffer);
}

void ExternalList_Allocate(Dart_NativeArguments arguments) {
  Dart_EnterScope();

  uint64_t size;
  Dart_IntegerToUint64(Dart_GetNativeArgument(arguments, 0), &size);
    
  uint8_t* data = new uint8_t[size];
  Dart_Handle result = Dart_NewExternalByteArray(data,
                                                 size,
                                                 data,
                                                 ExternalList_Free);
  
  if (Dart_IsError(result)) {
    ExternalList_Free(data);
    Dart_PropagateError(result);
  }

  Dart_SetReturnValue(arguments, result);
  Dart_ExitScope();
}

struct FunctionLookup {
  const char* name;
  Dart_NativeFunction function;
};

FunctionLookup function_list[] = {
	{"AllocateExternalList", ExternalList_Allocate},
    {"GetPlatforms", GetPlatforms},
    {"CreateContextFromType", CreateContextFromType}, 
    {"WebCLPlatform_getDevices", WebCLPlatform_getDevices},
    {"WebCLPlatform_getInfo", WebCLPlatform_getInfo},  
    {"WebCLDevice_getInfo", WebCLDevice_getInfo},
    {"WebCLContext_createBuffer", WebCLContext_createBuffer},
    {"WebCLContext_createProgram", WebCLContext_createProgram},
    {"WebCLContext_getInfo", WebCLContext_getInfo},
    {"WebCLContext_createCommandQueue", WebCLContext_createCommandQueue},
    {"WebCLProgram_getBuildInfo", WebCLProgram_getBuildInfo},
    {"WebCLProgram_build", WebCLProgram_build},
    {"WebCLProgram_createKernel", WebCLProgram_createKernel},
    {"WebCLKernel_setArg", WebCLKernel_setArg},
    {"WebCLCommandQueue_enqueueWriteBuffer", WebCLCommandQueue_enqueueWriteBuffer},
	{"WebCLCommandQueue_enqueueReadBuffer", WebCLCommandQueue_enqueueReadBuffer},
	{"WebCLCommandQueue_enqueueNDRangeKernel", WebCLCommandQueue_enqueueNDRangeKernel},
	{"WebCLCommandQueue_finish", WebCLCommandQueue_finish},
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

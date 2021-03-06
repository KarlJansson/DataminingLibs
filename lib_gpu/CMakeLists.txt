# required cmake version                                                                                                                                                                                   
cmake_minimum_required(VERSION 2.8)

set(dir_list
	./include
	./source
	../source_shared
	../source_shared/include
	../lib_core/include
	${TBB_INCLUDE_DIR}
)

set(cpp_files
	./test/test_gpu_device.h

	../source_shared/precomp.h
	./include/lib_gpu.h
	./include/gpu_interface.h
	./include/gpu_device.h
	./include/cuda_kernel_helpers.hpp
	./source/gpu_device.cc
	./source/gpu_device_cuda.h
	./source/gpu_device_opencl.h

	../source_shared/precomp.cc
	./source/gpu_interface.cc
	./source/gpu_device_cuda.cc
	./source/gpu_device_opencl.cc
)

source_group(source FILES
	./source/gpu_device_cuda.h
	./source/gpu_device_opencl.h

	./source/gpu_device_cuda.cc
	./source/gpu_device_opencl.cc
)

source_group(interface FILES
	./include/lib_gpu.h
	./include/gpu_interface.h
	./include/gpu_device.h
	./source/gpu_device.cc
	./include/cuda_kernel_helpers.hpp

	./source/gpu_interface.cc
)

source_group(test FILES
	./test/test_gpu_device.h
)

source_group(precomp FILES
	../source_shared/precomp.h

	../source_shared/precomp.cc
)

add_library(lib_gpu SHARED ${cpp_files})
if(UNIX)
	target_compile_features(lib_gpu PRIVATE cxx_range_for cxx_contextual_conversions cxx_generic_lambdas)
	set(cuda_lib cuda)
endif(UNIX)

if(WIN32)
	set(cuda_lib ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/cuda.lib)
endif(WIN32)

set(Cuda_Libs)
set(Cuda_Include)
if(CUDA_FOUND)
	set(Cuda_Libs
		${CUDA_cusparse_LIBRARY}
		${CUDA_curand_LIBRARY}
		${CUDA_LIBRARIES}
		${cuda_lib}
	)
	set(Cuda_Include
		${CUDA_INCLUDE_DIRS}
	)
endif(CUDA_FOUND)

target_link_libraries(lib_gpu
	${Cuda_Libs}

	lib_core
)

include_directories(lib_gpu
	${Cuda_Include}

	${dir_list}
)

if (MSVC)
   set_target_properties(lib_gpu PROPERTIES COMPILE_FLAGS "/Yuprecomp.h")
   set_source_files_properties(../source_shared/precomp.cc PROPERTIES COMPILE_FLAGS "/Ycprecomp.h")
endif(MSVC)

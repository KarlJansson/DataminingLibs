# required cmake version                                                                                                                                                                                   
cmake_minimum_required(VERSION 2.8)

set(dir_list
	./include
	./source
	../source_shared
	../source_shared/include
	../lib_models/include
	../lib_data/include
	../lib_gpu/include
	../lib_core/include
	../lib_parsing/include
	../lib_ensembles/include
	${TBB_INCLUDE_DIR}
)

set(cpp_files
	../source_shared/precomp.h
	../source_shared/include/global_defines.h
	./include/dte_algorithm_shared.h

	./include/ml_algorithm_params.h
	./include/lib_algorithms.h
	./include/ml_algorithm.h
	./include/algorithms_interface.h

	./source/ml_algorithm_params_impl.h
	./source/gpu_algorithm.h
	./source/cpu_algorithm.h
	./source/hybrid_algorithm.h


	./source/ml_algorithm_params_impl.cc
	./source/algorithms_interface.cc
	../source_shared/precomp.cc
	./source/gpu_algorithm.cc
	./source/cpu_algorithm.cc
	./source/hybrid_algorithm.cc
)

source_group(source FILES
	./source/ml_algorithm_params_impl.h
	./source/gpu_algorithm.h
	./source/cpu_algorithm.h
	./source/hybrid_algorithm.h

	./source/ml_algorithm_params_impl.cc
	./source/gpu_algorithm.cc
	./source/cpu_algorithm.cc
	./source/hybrid_algorithm.cc
)

source_group(interface FILES
	./include/ml_algorithm_params.h
	./include/lib_algorithms.h
	./include/ml_algorithm.h
	./include/algorithms_interface.h
	./include/dte_algorithm_shared.h

	./source/algorithms_interface.cc
)

source_group(test FILES
)

source_group(precomp FILES
	../source_shared/include/global_defines.h
	../source_shared/precomp.h

	../source_shared/precomp.cc
)

add_library(lib_algorithms SHARED ${cpp_files})
if(UNIX)
	target_compile_features(lib_algorithms PRIVATE cxx_range_for cxx_contextual_conversions cxx_generic_lambdas)
endif(UNIX)

target_link_libraries(lib_algorithms
	lib_models
	lib_data
	lib_parsing
	lib_gpu
	lib_core
)

include_directories(
	${dir_list}
)

if (MSVC)
   set_target_properties(lib_algorithms PROPERTIES COMPILE_FLAGS "/Yuprecomp.h")
   set_source_files_properties(../source_shared/precomp.cc PROPERTIES COMPILE_FLAGS "/Ycprecomp.h")
endif(MSVC)

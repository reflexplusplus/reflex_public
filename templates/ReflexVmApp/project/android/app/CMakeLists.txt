# For more information about using CMake with Android Studio, read the documentation: https://d.android.com/studio/projects/add-native-code.html
cmake_minimum_required(VERSION 3.22.1)
project(CppLib C CXX)
set(CMAKE_CXX_STANDARD 20)
set(PROJECT_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

find_package(oboe REQUIRED CONFIG)
find_package(reflex REQUIRED CONFIG)

add_library(CppLib SHARED
	# Bootstrap files
	${REFLEX_DIR}/src/reflex_ext.cpp

	# User app itself (platform-independent)
	${PROJECT_DIR}/code/entry.cpp
	${PROJECT_DIR}/code/resources.cpp
)

target_include_directories(CppLib PRIVATE
	${REFLEX_DIR}/include
)

target_compile_options(CppLib PRIVATE
	-Werror -Wno-ambiguous-reversed-operator -Wno-deprecated-volatile -Wno-switch -Wno-uninitialized -Wno-invalid-offsetof -fno-rtti)

target_compile_definitions(CppLib PRIVATE
	REFLEX_BOOTSTRAP_TYPE_VM_APP
)

# Refer to: https://github.com/android-ndk/ndk/issues/381
target_link_options(CppLib PRIVATE "-Wl,-u,ANativeActivity_onCreate")

target_link_libraries(CppLib
	oboe::oboe
	reflex::ReflexCommon
	reflex::ReflexCommonUi
	reflex::ReflexCommonVm
	reflex::ReflexCommonVmUi
	reflex::ReflexTargetApp

	# EGL and other dependent libraries required for drawing and interacting with Android system
	EGL
	GLESv3
	android
	log
)

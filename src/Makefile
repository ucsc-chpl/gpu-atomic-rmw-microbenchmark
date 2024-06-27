CXXFLAGS = -std=c++17
CLSPVFLAGS = --cl-std=CL2.0 --spv-version=1.3 --inline-entry-points -O0
SHADERS = $(wildcard *.cl)
CINITS = $(patsubst %.cl,%.cinit,$(SHADERS))
BENCHMARK = atomic_rmw_test
SOURCES = $(wildcard *cpp)

all: easyvk $(BENCHMARK)

easyvk:
	make -C ../easyvk

$(BENCHMARK): $(SOURCES) $(CINITS)
	$(CXX) $(CXXFLAGS) -I../easyvk/src -I. ../easyvk/build/easyvk.o $(SOURCES) -lvulkan -o $(BENCHMARK).run

android: easyvk $(BENCHMARK)
	ndk-build APP_BUILD_SCRIPT=./Android.mk  NDK_PROJECT_PATH=. NDK_APPLICATION_MK=./Application.mk NDK_LIBS_OUT=./build/android/libs NDK_OUT=./build/android/obj

%.spv: %.cl
	clspv $(CLSPVFLAGS) $< -o $@

%.cinit: %.cl
	clspv $(CLSPVFLAGS) --output-format=c  $< -o $@

clean:
	rm -rf *.cinit
	rm -rf *.spv
	rm *.run

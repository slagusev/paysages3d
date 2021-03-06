BUILDMODE=release
QTSDK=
BUILDPATH=./build/${BUILDMODE}
LIBRARY_PATH=${BUILDPATH}/system:${BUILDPATH}/basics:${BUILDPATH}/definition:${BUILDPATH}/render/software:${BUILDPATH}/render/preview:${BUILDPATH}/render/opengl:${BUILDPATH}/tests/googletest
BUILD_SPEC=linux-g++
PATH:=${QTSDK}/bin:${PATH}

all:build

format:
	find src \( \( -name '*.cpp' -or -name '*.h' \) -and \! -path '*/googletest/*' \) -exec clang-format -i \{\} \;

dirs:
	mkdir -p ${BUILDPATH}

makefiles:dirs
ifeq (${BUILDMODE}, release)
	@+cd ${BUILDPATH} && qmake ../../src/paysages.pro -r -spec $(BUILD_SPEC)
else
	@+cd ${BUILDPATH} && qmake ../../src/paysages.pro -r -spec $(BUILD_SPEC) CONFIG+=debug CONFIG+=declarative_debug CONFIG+=qml_debug
endif

build:makefiles
	@+cd ${BUILDPATH} && $(MAKE)

clean:makefiles
	@+cd ${BUILDPATH} && $(MAKE) clean
ifeq (${BUILDMODE}, release)
	make BUILDMODE=debug clean
endif

docs:
	doxygen Doxyfile

debug:
	+make BUILDMODE=debug all

release:
	+make BUILDMODE=release all

testscheck:
	+make RUNNER='valgrind --leak-check=full' BUILDMODE=debug tests

tests:build
ifdef TESTCASE
	LD_LIBRARY_PATH=$(LIBRARY_PATH) ${RUNNER} ${BUILDPATH}/tests/paysages-tests $(ARGS) --gtest_filter=$(TESTCASE).*
else
	LD_LIBRARY_PATH=$(LIBRARY_PATH) ${RUNNER} ${BUILDPATH}/tests/paysages-tests $(ARGS)
endif

run:build
	LD_LIBRARY_PATH=$(LIBRARY_PATH) ${RUNNER} ${BUILDPATH}/interface/modeler/paysages-modeler $(ARGS)

run_cli:build
	LD_LIBRARY_PATH=$(LIBRARY_PATH) ${RUNNER} ${BUILDPATH}/interface/commandline/paysages-cli $(ARGS)

profile:build
	LD_LIBRARY_PATH=${LIBRARY_PATH} perf record -g ${BUILDPATH}/interface/modeler/paysages-modeler $(ARGS)
	perf report -g

profile_cli:build
	LD_LIBRARY_PATH=${LIBRARY_PATH} perf record -g ${BUILDPATH}/interface/commandline/paysages-cli $(ARGS)
	perf report -g

gltrace:build
	rm -f *.trace
	LD_PRELOAD="$(wildcard /usr/lib/x86_64-linux-gnu/apitrace/wrappers/glxtrace.so /usr/local/lib/x86_64-linux-gnu/apitrace/wrappers/glxtrace.so)" LD_LIBRARY_PATH=$(LIBRARY_PATH) ${BUILDPATH}/interface/modeler/paysages-modeler $(ARGS)
	qapitrace paysages-modeler.trace

package:build
	rm -rf paysages3d-linux
	rm -f paysages3d-linux.tar.bz2
	mkdir paysages3d-linux
	mkdir paysages3d-linux/lib
	cp $(BUILDPATH)/system/libpaysages_system.so* paysages3d-linux/lib/
	cp $(BUILDPATH)/basics/libpaysages_basics.so* paysages3d-linux/lib/
	cp $(BUILDPATH)/definition/libpaysages_definition.so* paysages3d-linux/lib/
	cp $(BUILDPATH)/render/software/libpaysages_render_software.so* paysages3d-linux/lib/
	cp $(BUILDPATH)/render/preview/libpaysages_render_preview.so* paysages3d-linux/lib/
	cp $(BUILDPATH)/render/opengl/libpaysages_render_opengl.so* paysages3d-linux/lib/
	cp $(BUILDPATH)/interface/modeler/paysages-modeler paysages3d-linux/lib/
	chmod +x paysages3d-linux/lib/paysages-modeler
	cp -r data paysages3d-linux/
	cp dist/paysages3d.sh paysages3d-linux/
	chmod +x paysages3d-linux/paysages3d.sh
	cp dist/collectlib.sh paysages3d-linux/
	chmod +x paysages3d-linux/collectlib.sh
	cd paysages3d-linux && ./collectlib.sh && rm collectlib.sh && cd -
	tar -cjvvf paysages3d-linux.tar.bz2  paysages3d-linux/

.PHONY:all clean release build

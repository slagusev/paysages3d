TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    system \
    basics \
    definition \
    render/software \
    render/opengl \
    interface/commandline \
    interface/modeler

exists( tests/googletest/sources/src/gtest-all.cc ) {
    SUBDIRS += \
        tests/googletest \
        tests
}

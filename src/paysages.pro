TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    system \
    basics \
    definition \
    rendering \
    render/software \
    render/opengl \
    editing \
    controlling

#unix:SUBDIRS += testing tests

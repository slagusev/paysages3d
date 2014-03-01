#ifndef TEXTURE3D_H
#define TEXTURE3D_H

#include "basics_global.h"

namespace paysages {
namespace basics {

class BASICSSHARED_EXPORT Texture3D
{
public:
    Texture3D(int xsize, int ysize, int zsize);
    ~Texture3D();

    void getSize(int* xsize, int* ysize, int* zsize) const;
    void setPixel(int x, int y, int z, Color col);
    Color getPixel(int x, int y, int z) const;
    Color getNearest(double dx, double dy, double dz) const;
    Color getLinear(double dx, double dy, double dz) const;
    Color getCubic(double dx, double dy, double dz) const;
    void fill(Color col);
    void add(Texture3D* other);
    void save(PackStream* stream) const;
    void load(PackStream* stream);
    void saveToFile(const std::string &filepath) const;

private:
    int xsize;
    int ysize;
    int zsize;
    Color* data;
};

}
}

#endif // TEXTURE3D_H

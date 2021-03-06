#include "Texture3D.h"

#include "Color.h"
#include "PackStream.h"
#include "PictureWriter.h"
#include <cassert>
#include <cmath>

Texture3D::Texture3D(int xsize, int ysize, int zsize) {
    assert(xsize > 0 && ysize > 0 && zsize > 0);

    this->xsize = xsize;
    this->ysize = ysize;
    this->zsize = zsize;
    this->data = new Color[xsize * ysize * zsize];
}

Texture3D::~Texture3D() {
    delete[] data;
}

void Texture3D::getSize(int *xsize, int *ysize, int *zsize) const {
    *xsize = this->xsize;
    *ysize = this->ysize;
    *zsize = this->zsize;
}

void Texture3D::setPixel(int x, int y, int z, Color col) {
    assert(x >= 0 && x < this->xsize);
    assert(y >= 0 && y < this->ysize);
    assert(z >= 0 && z < this->ysize);

    this->data[z * this->xsize * this->ysize + y * this->xsize + x] = col;
}

Color Texture3D::getPixel(int x, int y, int z) const {
    assert(x >= 0 && x < this->xsize);
    assert(y >= 0 && y < this->ysize);
    assert(z >= 0 && z < this->zsize);

    return this->data[z * this->xsize * this->ysize + y * this->xsize + x];
}

Color Texture3D::getNearest(double dx, double dy, double dz) const {
    if (dx < 0.0)
        dx = 0.0;
    if (dx > 1.0)
        dx = 1.0;
    if (dy < 0.0)
        dy = 0.0;
    if (dy > 1.0)
        dy = 1.0;
    if (dz < 0.0)
        dz = 0.0;
    if (dz > 1.0)
        dz = 1.0;

    int ix = trunc_to_int(dx * to_double(this->xsize - 1));
    int iy = trunc_to_int(dy * to_double(this->ysize - 1));
    int iz = trunc_to_int(dz * to_double(this->zsize - 1));

    assert(ix >= 0 && ix < this->xsize);
    assert(iy >= 0 && iy < this->ysize);
    assert(iz >= 0 && iz < this->zsize);

    return this->data[iz * this->xsize * this->ysize + iy * this->xsize + ix];
}

Color Texture3D::getLinear(double dx, double dy, double dz) const {
    if (dx < 0.0)
        dx = 0.0;
    if (dx > 1.0)
        dx = 1.0;
    if (dy < 0.0)
        dy = 0.0;
    if (dy > 1.0)
        dy = 1.0;
    if (dz < 0.0)
        dz = 0.0;
    if (dz > 1.0)
        dz = 1.0;

    dx *= to_double(this->xsize - 1);
    dy *= to_double(this->ysize - 1);
    dz *= to_double(this->zsize - 1);

    int ix = floor_to_int(dx);
    if (ix == this->xsize - 1) {
        ix--;
    }
    int iy = floor_to_int(dy);
    if (iy == this->ysize - 1) {
        iy--;
    }
    int iz = floor_to_int(dz);
    if (iz == this->zsize - 1) {
        iz--;
    }

    dx -= to_double(ix);
    dy -= to_double(iy);
    dz -= to_double(iz);

    Color *data = this->data + iz * this->xsize * this->ysize + iy * this->xsize + ix;

    Color cx1 = data->lerp(*(data + 1), dx);
    Color cx2 = (data + this->xsize)->lerp(*(data + this->xsize + 1), dx);
    Color cy1 = cx1.lerp(cx2, dy);

    data += this->xsize * this->ysize;
    cx1 = data->lerp(*(data + 1), dx);
    cx2 = (data + this->xsize)->lerp(*(data + this->xsize + 1), dx);
    Color cy2 = cx1.lerp(cx2, dy);

    return cy1.lerp(cy2, dz);
}

Color Texture3D::getCubic(double dx, double dy, double dz) const {
    /* TODO */
    return getLinear(dx, dy, dz);
}

void Texture3D::fill(Color col) {
    int i, n;
    n = this->xsize * this->ysize * this->zsize;
    for (i = 0; i < n; i++) {
        this->data[i] = col;
    }
}

void Texture3D::add(Texture3D *source) {
    int i, n;

    assert(source->xsize == this->xsize);
    assert(source->ysize == this->ysize);
    assert(source->zsize == this->zsize);

    n = source->xsize * source->ysize * source->zsize;
    for (i = 0; i < n; i++) {
        this->data[i].r += source->data[i].r;
        this->data[i].g += source->data[i].g;
        this->data[i].b += source->data[i].b;
        /* destination->data[i].a += source->data[i].a; */
    }
}

void Texture3D::save(PackStream *stream) const {
    int i, n;
    stream->write(&this->xsize);
    stream->write(&this->ysize);
    stream->write(&this->zsize);
    n = this->xsize * this->ysize * this->zsize;
    for (i = 0; i < n; i++) {
        (this->data + i)->save(stream);
    }
}

void Texture3D::load(PackStream *stream) {
    int i, n;
    stream->read(&this->xsize);
    stream->read(&this->ysize);
    stream->read(&this->zsize);
    n = this->xsize * this->ysize * this->zsize;
    delete[] this->data;
    this->data = new Color[n];
    for (i = 0; i < n; i++) {
        (this->data + i)->load(stream);
    }
}

class Texture3DWriter : public PictureWriter {
  public:
    Texture3DWriter(const Texture3D *tex) : tex(tex) {
    }

    virtual unsigned int getPixel(int x, int y) override {
        int xsize, ysize, zsize;
        tex->getSize(&xsize, &ysize, &zsize);

        int z = y / ysize;
        y = y % ysize;

        return tex->getPixel(x, y, z).to32BitBGRA();
    }

  private:
    const Texture3D *tex;
};

void Texture3D::saveToFile(const string &filepath) const {
    Texture3DWriter writer(this);
    writer.save(filepath, xsize, ysize * zsize);
}

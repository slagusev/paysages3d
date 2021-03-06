#include "Texture2D.h"

#include "Color.h"
#include "PackStream.h"
#include "PictureWriter.h"
#include <cassert>
#include <cmath>

Texture2D::Texture2D(int xsize, int ysize) {
    assert(xsize > 0 && ysize > 0);

    this->xsize = xsize;
    this->ysize = ysize;
    this->data = new Color[xsize * ysize];
}

Texture2D::~Texture2D() {
    delete[] data;
}

void Texture2D::getSize(int *xsize, int *ysize) const {
    *xsize = this->xsize;
    *ysize = this->ysize;
}

void Texture2D::setPixel(int x, int y, Color col) {
    assert(x >= 0 && x < xsize);
    assert(y >= 0 && y < ysize);

    data[y * xsize + x] = col;
}

Color Texture2D::getPixel(int x, int y) const {
    assert(x >= 0 && x < xsize);
    assert(y >= 0 && y < ysize);

    return data[y * xsize + x];
}

Color Texture2D::getNearest(double dx, double dy) const {
    if (dx < 0.0)
        dx = 0.0;
    if (dx > 1.0)
        dx = 1.0;
    if (dy < 0.0)
        dy = 0.0;
    if (dy > 1.0)
        dy = 1.0;

    int ix = trunc_to_int(dx * to_double(this->xsize - 1));
    int iy = trunc_to_int(dy * to_double(this->ysize - 1));

    assert(ix >= 0 && ix < this->xsize);
    assert(iy >= 0 && iy < this->ysize);

    return this->data[iy * this->xsize + ix];
}

Color Texture2D::getLinear(double dx, double dy) const {
    if (dx < 0.0)
        dx = 0.0;
    if (dx > 1.0)
        dx = 1.0;
    if (dy < 0.0)
        dy = 0.0;
    if (dy > 1.0)
        dy = 1.0;

    dx *= to_double(this->xsize - 1);
    dy *= to_double(this->ysize - 1);

    int ix = floor_to_int(dx);
    if (ix == this->xsize - 1) {
        ix--;
    }
    int iy = floor_to_int(dy);
    if (iy == this->ysize - 1) {
        iy--;
    }

    dx -= to_double(ix);
    dy -= to_double(iy);

    Color *data = this->data + iy * this->xsize + ix;

    Color c1 = data->lerp(*(data + 1), dx);
    Color c2 = (data + this->xsize)->lerp(*(data + this->xsize + 1), dx);

    return c1.lerp(c2, dy);
}

Color Texture2D::getCubic(double dx, double dy) const {
    /* TODO */
    return getLinear(dx, dy);
}

void Texture2D::fill(Color col) {
    int i, n;
    n = this->xsize * this->ysize;
    for (i = 0; i < n; i++) {
        this->data[i] = col;
    }
}

void Texture2D::add(Texture2D *source) {
    int i, n;

    assert(source->xsize == this->xsize);
    assert(source->ysize == this->ysize);

    n = source->xsize * source->ysize;
    for (i = 0; i < n; i++) {
        this->data[i].r += source->data[i].r;
        this->data[i].g += source->data[i].g;
        this->data[i].b += source->data[i].b;
        /* destination->data[i].a += source->data[i].a; */
    }
}

void Texture2D::save(PackStream *stream) const {
    int i, n;
    stream->write(&this->xsize);
    stream->write(&this->ysize);
    n = this->xsize * this->ysize;
    for (i = 0; i < n; i++) {
        (this->data + i)->save(stream);
    }
}

void Texture2D::load(PackStream *stream) {
    int i, n;
    stream->read(&this->xsize);
    stream->read(&this->ysize);
    n = this->xsize * this->ysize;
    delete[] this->data;
    this->data = new Color[n];
    for (i = 0; i < n; i++) {
        (this->data + i)->load(stream);
    }
}

class Texture2DWriter : public PictureWriter {
  public:
    Texture2DWriter(const Texture2D *tex) : tex(tex) {
    }

    virtual unsigned int getPixel(int x, int y) override {
        return tex->getPixel(x, y).to32BitBGRA();
    }

  private:
    const Texture2D *tex;
};

void Texture2D::saveToFile(const string &filepath) const {
    Texture2DWriter writer(this);
    writer.save(filepath, xsize, ysize);
}

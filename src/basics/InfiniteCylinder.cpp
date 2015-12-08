#include "InfiniteCylinder.h"

#include <cmath>
#include "PackStream.h"

#define EPS 1E-8

InfiniteCylinder::InfiniteCylinder() {
}

InfiniteCylinder::InfiniteCylinder(const InfiniteRay &axis, double radius) : axis(axis), radius(radius) {
}

int InfiniteCylinder::checkRayIntersection(const InfiniteRay &ray, Vector3 *first_intersection,
                                           Vector3 *second_intersection) const {
    /*
     * Original algorithm has been altered, because it didn't work with non-(0,0,0) axis origin.
     * Maybe some optimizations could be made from this.
     */

    Vector3 U, V, F = axis.getDirection(), P, B, Q, G, AG, AQ;
    double length, invLength, prod;
    double R[3][3], A[3][3];
    double e0, e1, C, c0, c1, c2, discr, invC2, root0, root1, root;

    /* choose U, V so that U,V,F is orthonormal set */

    if (fabs(F.x) > fabs(F.y) && fabs(F.x) > fabs(F.z)) {
        length = sqrt(F.x * F.x + F.y * F.y);
        invLength = 1.0 / length;
        U.x = -F.y * invLength;
        U.y = +F.x * invLength;
        U.z = 0.0;
        prod = -F.z * invLength;
        V.x = F.x * prod;
        V.y = F.y * prod;
        V.z = length;
    } else {
        length = sqrt(F.y * F.y + F.z * F.z);
        invLength = 1.0 / length;
        U.x = 0.0;
        U.y = -F.z * invLength;
        U.z = +F.y * invLength;
        prod = -F.x * invLength;
        V.x = length;
        V.y = F.y * prod;
        V.z = F.z * prod;
    }

    /* orthonormal matrix */
    R[0][0] = U.x;
    R[0][1] = U.y;
    R[0][2] = U.z;
    R[1][0] = V.x;
    R[1][1] = V.y;
    R[1][2] = V.z;
    R[2][0] = F.x;
    R[2][1] = F.y;
    R[2][2] = F.z;

    /* matrix A */
    A[0][0] = R[0][0] * R[0][0] + R[1][0] * R[1][0];
    A[0][1] = R[0][0] * R[0][1] + R[1][0] * R[1][1];
    A[0][2] = R[0][0] * R[0][2] + R[1][0] * R[1][2];

    A[1][0] = R[0][1] * R[0][0] + R[1][1] * R[1][0];
    A[1][1] = R[0][1] * R[0][1] + R[1][1] * R[1][1];
    A[1][2] = R[0][1] * R[0][2] + R[1][1] * R[1][2];

    A[2][0] = R[0][2] * R[0][0] + R[1][2] * R[1][0];
    A[2][1] = R[0][2] * R[0][1] + R[1][2] * R[1][1];
    A[2][2] = R[0][2] * R[0][2] + R[1][2] * R[1][2];

    /* vector B */
    P = Vector3(0.0, 0.0, 0.0);
    B.x = -2.0 * P.x;
    B.y = -2.0 * P.y;
    B.z = -2.0 * P.z;

    /* constant C */
    e0 = -2.0 * (R[0][0] * P.x + R[0][1] * P.y + R[0][2] * P.z);
    e1 = -2.0 * (R[1][0] * P.x + R[1][1] * P.y + R[1][2] * P.z);
    C = 0.25 * (e0 * e0 + e1 * e1) - radius * radius;

    /* line */
    Q = ray.getOrigin().sub(axis.getOrigin());
    G = ray.getDirection();

    /* compute A*G */
    AG.x = A[0][0] * G.x + A[0][1] * G.y + A[0][2] * G.z;
    AG.y = A[1][0] * G.x + A[1][1] * G.y + A[1][2] * G.z;
    AG.z = A[2][0] * G.x + A[2][1] * G.y + A[2][2] * G.z;

    /* compute A*Q */
    AQ.x = A[0][0] * Q.x + A[0][1] * Q.y + A[0][2] * Q.z;
    AQ.y = A[1][0] * Q.x + A[1][1] * Q.y + A[1][2] * Q.z;
    AQ.z = A[2][0] * Q.x + A[2][1] * Q.y + A[2][2] * Q.z;

    /* compute quadratic equation c0+c1*t+c2*t^2 = 0 */
    c2 = G.x * AG.x + G.y * AG.y + G.z * AG.z;
    c1 = B.x * G.x + B.y * G.y + B.z * G.z + 2.0f * (Q.x * AG.x + Q.y * AG.y + Q.z * AG.z);
    c0 = Q.x * AQ.x + Q.y * AQ.y + Q.z * AQ.z + B.x * Q.x + B.y * Q.y + B.z * Q.z + C;

    /* solve for intersections */
    int numIntersections;
    discr = c1 * c1 - 4.0 * c0 * c2;
    if (discr > EPS) {
        numIntersections = 2;
        discr = sqrt(discr);
        invC2 = 1.0 / c2;
        root0 = -0.5 * (c1 + discr) * invC2;
        root1 = -0.5 * (c1 - discr) * invC2;
        first_intersection->x = Q.x + root0 * G.x;
        first_intersection->y = Q.y + root0 * G.y;
        first_intersection->z = Q.z + root0 * G.z;
        second_intersection->x = Q.x + root1 * G.x;
        second_intersection->y = Q.y + root1 * G.y;
        second_intersection->z = Q.z + root1 * G.z;

        *first_intersection = first_intersection->add(axis.getOrigin());
        *second_intersection = second_intersection->add(axis.getOrigin());
    } else if (discr < -EPS) {
        numIntersections = 0;
    } else {
        numIntersections = 1;
        root = -0.5 * c1 / c2;
        first_intersection->x = Q.x + root * G.x;
        first_intersection->y = Q.y + root * G.y;
        first_intersection->z = Q.z + root * G.z;

        *first_intersection = first_intersection->add(axis.getOrigin());
    }

    return numIntersections;
}

void InfiniteCylinder::save(PackStream *stream) const {
    axis.save(stream);
    stream->write(&radius);
}

void InfiniteCylinder::load(PackStream *stream) {
    axis.load(stream);
    stream->read(&radius);
}

#ifndef _PAYSAGES_EUCLID_H_
#define _PAYSAGES_EUCLID_H_

#include "shared/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void v3Save(FILE* f, Vector3* v);
void v3Load(FILE* f, Vector3* v);
Vector3 v3Translate(Vector3 v1, double x, double y, double z);
Vector3 v3Add(Vector3 v1, Vector3 v2);
Vector3 v3Sub(Vector3 v1, Vector3 v2);
Vector3 v3Neg(Vector3 v);
Vector3 v3Scale(Vector3 v, double scale);
double v3Norm(Vector3 v);
Vector3 v3Normalize(Vector3 v);
double v3Dot(Vector3 v1, Vector3 v2);
Vector3 v3Cross(Vector3 v1, Vector3 v2);

void m4Save(FILE* f, Matrix4* m);
void m4Load(FILE* f, Matrix4* m);
Matrix4 m4NewIdentity();
Matrix4 m4Mult(Matrix4 m1, Matrix4 m2);
Vector3 m4MultPoint(Matrix4 m, Vector3 v);
Vector3 m4Transform(Matrix4 m, Vector3 v);
Matrix4 m4Transpose(Matrix4 m);
Matrix4 m4NewScale(double x, double y, double z);
Matrix4 m4NewTranslate(double x, double y, double z);
Matrix4 m4NewRotateX(double angle);
Matrix4 m4NewRotateY(double angle);
Matrix4 m4NewRotateZ(double angle);
Matrix4 m4NewRotateAxis(double angle, Vector3 axis);
Matrix4 m4NewRotateEuler(double heading, double attitude, double bank);
Matrix4 m4NewRotateTripleAxis(Vector3 x, Vector3 y, Vector3 z);
Matrix4 m4NewLookAt(Vector3 eye, Vector3 at, Vector3 up);
Matrix4 m4NewPerspective(double fov_y, double aspect, double near, double far);
double m4Determinant(Matrix4 m);
Matrix4 m4Inverse(Matrix4 m);

#ifdef __cplusplus
}
#endif

#endif
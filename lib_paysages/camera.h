#ifndef _PAYSAGES_CAMERA_H_
#define _PAYSAGES_CAMERA_H_

#include <stdio.h>
#include "shared/types.h"
#include "renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definition of a 3D scene camera.
 * 
 * Don't modify this structure directly, use the provided functions.
 */
typedef struct
{
    Vector3 location;
    double yaw;
    double pitch;
    double roll;
    
    Vector3 target;
    Vector3 forward;
    Vector3 right;
    Vector3 up;
    
    Matrix4 project;
    Matrix4 unproject;
} CameraDefinition;

void cameraInit();
void cameraSave(FILE* f, CameraDefinition* camera);
void cameraLoad(FILE* f, CameraDefinition* camera);

CameraDefinition cameraCreateDefinition();
void cameraDeleteDefinition(CameraDefinition* definition);
void cameraCopyDefinition(CameraDefinition* source, CameraDefinition* destination);
void cameraValidateDefinition(CameraDefinition* definition, int check_above);

void cameraSetLocation(CameraDefinition* camera, double x, double y, double z);
void cameraSetTarget(CameraDefinition* camera, double x, double y, double z);
void cameraSetRoll(CameraDefinition* camera, double angle);

void cameraStrafeForward(CameraDefinition* camera, double value);
void cameraStrafeRight(CameraDefinition* camera, double value);
void cameraStrafeUp(CameraDefinition* camera, double value);
void cameraRotateYaw(CameraDefinition* camera, double value);
void cameraRotatePitch(CameraDefinition* camera, double value);
void cameraRotateRoll(CameraDefinition* camera, double value);

Vector3 cameraProject(CameraDefinition* camera, Renderer* renderer, Vector3 point);
Vector3 cameraUnproject(CameraDefinition* camera, Renderer* renderer, Vector3 point);
void cameraProjectToFragment(CameraDefinition* camera, Renderer* renderer, double x, double y, double z, RenderFragment* result);
/*void cameraPushOverlay(CameraDefinition* camera, Color col, f_RenderFragmentCallback callback);*/

#ifdef __cplusplus
}
#endif

#endif

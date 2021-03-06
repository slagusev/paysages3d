#include "AtmosphereDefinition.h"
#include "CameraDefinition.h"
#include "Logs.h"
#include "RenderConfig.h"
#include "Scenery.h"
#include "SoftwareCanvasRenderer.h"
#include "TimeManager.h"

#include <cstring>

void startRender(SoftwareCanvasRenderer *renderer, const char *outputpath);
void runTestSuite();

static void displayHelp() {
    printf("Usage : paysages-cli [options]\n");
    printf("Options :\n");
    printf(" -h      Show this help\n");
    printf(" -ts     Run the render test suite\n");
    printf(" -f x    Saved file to load (str)\n");
    printf(" -n      Number of pictures in the sequence\n");
    printf(" -rw x   Render width (int)\n");
    printf(" -rh x   Render height (int)\n");
    printf(" -rq x   Render quality (int, 1 to 10)\n");
    printf(" -ra x   Render anti-aliasing (int, 1 to 4)\n");
    printf(" -wx x   Wind power in X direction (double)\n");
    printf(" -wz z   Wind power in Z direction (double)\n");
    printf(" -di x   Day start time (double, 0.0 to 1.0)\n");
    printf(" -ds x   Day step time (double)\n");
    printf(" -cx x   Camera X step (double)\n");
    printf(" -cy y   Camera Y step (double)\n");
    printf(" -cz z   Camera Z step (double)\n");
}

int main(int argc, char **argv) {
    SoftwareCanvasRenderer *renderer;
    char *conf_file_path = NULL;
    RenderConfig conf_render_params(480, 270, 1, 3);
    int conf_first_picture = 0;
    int conf_nb_pictures = 1;
    double conf_daytime_start = -1.0;
    double conf_daytime_step = 0.0;
    double conf_camera_step_x = 0.0;
    double conf_camera_step_y = 0.0;
    double conf_camera_step_z = 0.0;
    double conf_wind_x = 0.0;
    double conf_wind_z = 0.0;
    int outputcount;
    char outputpath[500];

    argc--;
    argv++;

    while (argc--) {
        if (strcmp(*argv, "-h") == 0 || strcmp(*argv, "--help") == 0) {
            displayHelp();
            return 0;
        } else if (strcmp(*argv, "-ts") == 0 || strcmp(*argv, "--testsuite") == 0) {
            runTestSuite();
            return 0;
        } else if (strcmp(*argv, "-f") == 0 || strcmp(*argv, "--file") == 0) {
            if (argc--) {
                conf_file_path = *(++argv);
            }
        } else if (strcmp(*argv, "-s") == 0 || strcmp(*argv, "--start") == 0) {
            if (argc--) {
                conf_first_picture = atoi(*(++argv));
            }
        } else if (strcmp(*argv, "-n") == 0 || strcmp(*argv, "--count") == 0) {
            if (argc--) {
                conf_nb_pictures = atoi(*(++argv));
            }
        } else if (strcmp(*argv, "-rw") == 0 || strcmp(*argv, "--width") == 0) {
            if (argc--) {
                conf_render_params.width = atoi(*(++argv));
            }
        } else if (strcmp(*argv, "-rh") == 0 || strcmp(*argv, "--height") == 0) {
            if (argc--) {
                conf_render_params.height = atoi(*(++argv));
            }
        } else if (strcmp(*argv, "-rq") == 0 || strcmp(*argv, "--quality") == 0) {
            if (argc--) {
                conf_render_params.quality = atoi(*(++argv));
            }
        } else if (strcmp(*argv, "-ra") == 0 || strcmp(*argv, "--antialias") == 0) {
            if (argc--) {
                conf_render_params.antialias = atoi(*(++argv));
            }
        } else if (strcmp(*argv, "-wx") == 0 || strcmp(*argv, "--windx") == 0) {
            if (argc--) {
                conf_wind_x = atof(*(++argv));
            }
        } else if (strcmp(*argv, "-wz") == 0 || strcmp(*argv, "--windz") == 0) {
            if (argc--) {
                conf_wind_z = atof(*(++argv));
            }
        } else if (strcmp(*argv, "-di") == 0 || strcmp(*argv, "--daystart") == 0) {
            if (argc--) {
                conf_daytime_start = atof(*(++argv));
            }
        } else if (strcmp(*argv, "-ds") == 0 || strcmp(*argv, "--daystep") == 0) {
            if (argc--) {
                conf_daytime_step = atof(*(++argv));
            }
        } else if (strcmp(*argv, "-cx") == 0 || strcmp(*argv, "--camerastepx") == 0) {
            if (argc--) {
                conf_camera_step_x = atof(*(++argv));
            }
        } else if (strcmp(*argv, "-cy") == 0 || strcmp(*argv, "--camerastepy") == 0) {
            if (argc--) {
                conf_camera_step_y = atof(*(++argv));
            }
        } else if (strcmp(*argv, "-cz") == 0 || strcmp(*argv, "--camerastepz") == 0) {
            if (argc--) {
                conf_camera_step_z = atof(*(++argv));
            }
        }

        argv++;
    }

    printf("Initializing ...\n");
    Scenery *scenery = new Scenery();

    if (conf_file_path) {
        scenery->loadGlobal(conf_file_path);
    } else {
        scenery->autoPreset();
    }

    Logs::debug("CommandLine") << "Rendered scenery :" << endl << scenery->toString() << endl;

    TimeManager time;
    time.setWind(conf_wind_x, conf_wind_z);
    if (conf_daytime_start >= 0.0) {
        scenery->getAtmosphere()->setDayTime(conf_daytime_start);
    }

    for (outputcount = 0; outputcount < conf_first_picture + conf_nb_pictures; outputcount++) {
        CameraDefinition *camera = scenery->getCamera();
        Vector3 step = {conf_camera_step_x, conf_camera_step_y, conf_camera_step_z};
        camera->setLocation(camera->getLocation().add(step));

        renderer = new SoftwareCanvasRenderer(scenery);
        renderer->setConfig(conf_render_params);

        if (outputcount >= conf_first_picture) {
            sprintf(outputpath, "pic%05d.png", outputcount);
            startRender(renderer, outputpath);
        }

        delete renderer;

        time.moveForward(scenery, conf_daytime_step);
    }

    printf("Cleaning up ...\n");
    delete scenery;

    printf("\rDone.                         \n");

    return 0;
}


#include <stdlib.h>

bool app_init(struct android_app *state);
void app_handle_cmd(android_app *evt_app, int32_t cmd);
void app_step();
void app_exit();

android_app *app;
bool app_run = true;

void android_main(struct android_app *state)
{
    app = state;
    app->onAppCmd = app_handle_cmd;

    if (!app_init(state)) return;

    while (app_run)
    {
        int events;
        struct android_poll_source *source;
        while (ALooper_pollAll(0, nullptr, &events, (void **)&source) >= 0)
        {
            if (source != nullptr)
                source->process(state, source);
            if (state->destroyRequested != 0)
                app_run = false;
        }
        app_step();
    }

    app_exit();
}

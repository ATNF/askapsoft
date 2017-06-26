// expose the member functions to the outside world as C functions
// If Daliuge changes its API - you will have to change these functions
// but hopefully all the issues will be hidden behind the structures

#include "askap_daliuge_pipeline.h"

#include<daliuge/DaliugeApplication.h>
#include<factory/DaliugeApplicationFactory.h>
#include<factory/Interface.h>


    int init(dlg_app_info *app, const char ***arguments) {
        // this means we have to instantiate an application
        // and call its init
        const char **param = arguments[0];
        bool got_name = false;
        while (1) {

            // Sentinel
            if (*param == NULL) {
                break;
            }

            if (strcmp(param[0], "name") == 0) {
                app->appname = strdup(param[1]);
                got_name = true;
            }

            param++;
        }
        if (got_name == false) {
            app->appname = strdup("Example");
        }
        // need to set the app->appname here .... from the arguments ....
        askap::DaliugeApplication::ShPtr thisApp = askap::DaliugeApplicationFactory::make(app->appname);
        return thisApp->init(app, arguments);
    }
    int run(dlg_app_info *app) {
        askap::DaliugeApplication::ShPtr thisApp = askap::DaliugeApplicationFactory::make(app->appname);
        return thisApp->run(app);
    }
    void data_written(dlg_app_info *app, const char *uid,
        const char *data, size_t n) {
            askap::DaliugeApplication::ShPtr thisApp = askap::DaliugeApplicationFactory::make(app->appname);
            thisApp->data_written(app, uid, data, n);
    }
    void drop_completed(dlg_app_info *app, const char *uid,
        drop_status status) {
            askap::DaliugeApplication::ShPtr thisApp = askap::DaliugeApplicationFactory::make(app->appname);
            thisApp->drop_completed(app, uid, status);
    }

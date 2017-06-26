
#ifndef ASKAP_DALIUGE_INTERFACE_H
#define ASKAP_DALIUGE_INTERFACE_H
// expose the member functions to the outside world as C functions
// If Daliuge changes its API - you will have to change these functions
// but hopefully all the issues will be hidden behind the structures

#include<daliuge/DaliugeApplication.h>
#include<factory/DaliugeApplicationFactory.h>



extern "C" {

    int init(dlg_app_info *app, const char ***arguments);
    int run(dlg_app_info *app);
    void data_written(dlg_app_info *app, const char *uid,const char *data, size_t n);
    void drop_completed(dlg_app_info *app, const char *uid,drop_status status);

}
#endif

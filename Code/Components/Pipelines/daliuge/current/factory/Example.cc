/// @file GaussianPB.cc
///
/// @abstract
/// Derived from PrimaryBeams this is the Gaussian beam as already implemented
/// @ details
/// Implements the methods that evaluate the primary beam gain ain the case of a
/// Gaussian
///
#include "askap_daliuge_pipeline.h"


#include <daliuge/DaliugeApplication.h>
#include <factory/Example.h>
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>
#include <Common/ParameterSet.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>

#include <sys/time.h>

ASKAP_LOGGER(logger, ".daliuge.factory");
namespace askap {


    struct app_data {
        short print_stats;
        unsigned long total;
        unsigned long write_duration;
    };

    static inline
    struct app_data *to_app_data(dlg_app_info *app)
    {
        return (struct app_data *)app->data;
    }

    static inline
    unsigned long usecs(struct timeval *start, struct timeval *end)
    {
        return (end->tv_sec - start->tv_sec) * 1000000 + (end->tv_usec - start->tv_usec);
    }


    Example::Example() {
        ASKAPLOG_DEBUG_STR(logger,"Example default contructor");
    }


    Example::~Example() {

    }

    DaliugeApplication::ShPtr Example::createDaliugeApplication(const std::string &name)
    {
        ASKAPLOG_DEBUG_STR(logger, "createDaliugeApplication for Example ");

        Example::ShPtr ptr;

        // We need to pull all the parameters out of the parset - and set
        // all the private variables required to define the beam


        ptr.reset( new Example());

        ASKAPLOG_DEBUG_STR(logger,"Created Example DaliugeApplication instance");
        return ptr;

    }
    int Example::init(dlg_app_info *app, const char ***arguments) {

        short print_stats = 0;

        const char **param = arguments[0];
        while (1) {

            // Sentinel
            if (*param == NULL) {
                break;
            }

            if (strcmp(param[0], "print_stats") == 0) {
                print_stats = strcmp(param[1], "1") == 0 ||
                strcmp(param[1], "true") == 0;
                break;
            }
            if (strcmp(param[0], "name") == 0) {
                app->appname = strdup(param[1]);
            }

            param++;
        }

        app->data = malloc(sizeof(struct app_data));
        if (!app->data) {
            return 1;
        }
        to_app_data(app)->print_stats = print_stats;
        to_app_data(app)->total = 0;
        to_app_data(app)->write_duration = 0;
        return 0;
    }

    int Example::run(dlg_app_info *app) {

        char buf[64*1024];
        unsigned int total = 0, i;
        unsigned long read_duration = 0, write_duration = 0;
        struct timeval start, end;

        if (to_app_data(app)->print_stats) {
            printf("running / done methods addresses are %p / %p\n", app->running, app->done);
        }

        while (1) {

            gettimeofday(&start, NULL);
            size_t n_read = app->inputs[0].read(buf, 64*1024);
            gettimeofday(&end, NULL);
            read_duration += usecs(&start, &end);
            if (!n_read) {
                break;
            }

            gettimeofday(&start, NULL);
            for (i = 0; i < app->n_outputs; i++) {
                app->outputs[i].write(buf, n_read);
            }
            gettimeofday(&end, NULL);
            write_duration += usecs(&start, &end);
            total += n_read;
        }

        double duration = (read_duration + write_duration) / 1000000.;
        double total_mb = total / 1024. / 1024.;

        if (to_app_data(app)->print_stats) {
            printf("Read %.3f [MB] of data at %.3f [MB/s]\n", total_mb, total_mb / (read_duration / 1000000.));
            printf("Wrote %.3f [MB] of data at %.3f [MB/s]\n", total_mb, total_mb / (write_duration / 1000000.));
            printf("Copied %.3f [MB] of data at %.3f [MB/s]\n", total_mb, total_mb / duration);
        }

        return 0;
    }

    void Example::data_written(dlg_app_info *app, const char *uid,
        const char *data, size_t n) {
            unsigned int i;
            struct timeval start, end;

            app->running();
            gettimeofday(&start, NULL);
            for (i = 0; i < app->n_outputs; i++) {
                app->outputs[i].write(data, n);
            }
            gettimeofday(&end, NULL);

            to_app_data(app)->total += n;
            to_app_data(app)->write_duration += usecs(&start, &end);
    }

    void Example::drop_completed(dlg_app_info *app, const char *uid,
            drop_status status) {
                /* We only have one output so we're finished */
                double total_mb = (to_app_data(app)->total / 1024. / 1024.);
                if (to_app_data(app)->print_stats) {
                    printf("Wrote %.3f [MB] of data to %u outputs in %.3f [ms] at %.3f [MB/s]\n",
                    total_mb, app->n_outputs,
                    to_app_data(app)->write_duration / 1000.,
                    total_mb / (to_app_data(app)->write_duration / 1000000.));
                }
                app->done(APP_FINISHED);
                free(app->data);
    }


} // namespace

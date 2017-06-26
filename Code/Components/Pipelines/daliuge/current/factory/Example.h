/// @file GaussianPB.h

/// @brief Standard Gaussian Primary Beam
/// @details
///

#ifndef ASKAP_FACTORY_EXAMPLE_H
#define ASKAP_FACTORY_EXAMPLE_H

#include <daliuge/DaliugeApplication.h>

#include <boost/shared_ptr.hpp>


#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>

namespace askap {

    class Example : public DaliugeApplication

    {

    public:

        typedef boost::shared_ptr<Example> ShPtr;

        Example();

        static inline std::string ApplicationName() { return "Example";}

        virtual ~Example();

        static DaliugeApplication::ShPtr createDaliugeApplication(const std::string &name);

        virtual int init(dlg_app_info *app, const char ***arguments);

        virtual int run(dlg_app_info *app);

        virtual void data_written(dlg_app_info *app, const char *uid,
            const char *data, size_t n);

        virtual void drop_completed(dlg_app_info *app, const char *uid,
            drop_status status);

        private:

    };

} // namespace askap


#endif //

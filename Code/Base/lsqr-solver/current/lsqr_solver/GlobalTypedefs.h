/*
 * GlobalTypedefs.h
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#ifndef SRC_GLOBALTYPEDEFS_H_
#define SRC_GLOBALTYPEDEFS_H_

#include <vector>
#include <cstddef>

#include "Point.h"

//#define PARALLEL

namespace askap { namespace lsqr {

    typedef std::vector<double> Vector;

    typedef std::vector<Point> Vector3D;

}} // namespace askap.lsqr

#endif /* SRC_GLOBALTYPEDEFS_H_ */

/*
 * ParallelTools.h
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#ifndef SRC_UTILS_PARALLELTOOLS_H_
#define SRC_UTILS_PARALLELTOOLS_H_

#include <lsqr_solver/GlobalTypedefs.h>

namespace askap { namespace lsqr { namespace ParallelTools {

typedef std::vector<int> IVector;

/*
 * Gather the nelements from other CPUs.
 */
void get_number_elements_on_other_cpus(size_t nelements, IVector& nelements_at_cpu, int nbproc, void *comm);

/*
 * Returns the total number of elements (on all CPUs).
 */
size_t get_total_number_elements(size_t nelements, int nbproc, void *comm);

/*
 * Calculates the number of elements on CPUs with rank smaller than myrank.
 */
size_t get_nsmaller(size_t nelements, int myrank, int nbproc, void *comm);

/*
 * Calculates index arrays (displacement and number of elements) for mpi_scatterv / mpi_gatherv.
 */
void get_mpi_partitioning(size_t nelements, IVector& displs, IVector& nelements_at_cpu, int nbproc, void *comm);

/*
 * Returns the full array (that is split between CPUs).
 *     bcast = false: to only master CPU.
 *     bcast = true:  to all CPUs.
 */
void get_full_array(const Vector& localArray, size_t nelements, Vector& fullArray, bool bcast, int nbproc, void *comm);

/*
 * Returns the full array (that is split between CPUs).
 * The same as get_full_array() but uses the same send and recv buffer in MPI_Gatherv.
 *     bcast = false: to only master CPU.
 *     bcast = true:  to all CPUs.
 */
void get_full_array_in_place(size_t nelements, Vector& array, bool bcast, int myrank, int nbproc, void *comm);

}}} // namespace askap.lsqr.ParallelTools

#endif /* SRC_UTILS_PARALLELTOOLS_H_ */

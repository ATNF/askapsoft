/*
 * ParallelTools.cc
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

// MPI-specific includes
#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include <cstddef>
#include <cassert>
#include <stdexcept>

#include <lsqr_solver/ParallelTools.h>

namespace askap { namespace lsqr { namespace ParallelTools {

void get_number_elements_on_other_cpus(size_t nelements, IVector& nelements_at_cpu, int nbproc, const void *comm)
{
    if (nelements_at_cpu.size() != size_t(nbproc))
    {
        throw std::invalid_argument("Wrong vector size in get_number_elements_on_other_cpus!");
    }

    if (nbproc == 1)
    { // Single CPU: no MPI needed.
        nelements_at_cpu[0] = nelements;
        return;
    }

#ifdef HAVE_MPI
    const MPI_Comm *mpi_comm = static_cast<const MPI_Comm*>(comm);

    MPI_Gather(&nelements, 1, MPI_INTEGER, nelements_at_cpu.data(), 1, MPI_INTEGER, 0, *mpi_comm);
    MPI_Bcast(nelements_at_cpu.data(), nbproc, MPI_INTEGER, 0, *mpi_comm);
#else
    // Should not come here.
    assert(false);
#endif
}

size_t get_total_number_elements(size_t nelements, int nbproc, const void *comm)
{
    if (nbproc == 1)
    {
        return nelements;
    }

    IVector nelements_at_cpu(nbproc);
    get_number_elements_on_other_cpus(nelements, nelements_at_cpu, nbproc, comm);

    size_t nelements_total = 0;
    for (size_t i = 0; i < size_t(nbproc); ++i)
    {
        nelements_total += nelements_at_cpu[i];
    }
    return nelements_total;
}

size_t get_nsmaller(size_t nelements, int myrank, int nbproc, const void *comm)
{
    if (nbproc == 1)
    {
        return 0;
    }

    IVector nelements_at_cpu(nbproc);
    get_number_elements_on_other_cpus(nelements, nelements_at_cpu, nbproc, comm);

    size_t nsmaller = 0;
    for (size_t i = 0; i < size_t(myrank); ++i)
    {
        nsmaller += nelements_at_cpu[i];
    }
    return nsmaller;
}

void get_mpi_partitioning(size_t nelements, IVector& displs, IVector& nelements_at_cpu, int nbproc, const void *comm)
{
    // Sanity check.
    if (displs.size() != size_t(nbproc) || nelements_at_cpu.size() != size_t(nbproc))
    {
        throw std::invalid_argument("Wrong container size in get_mpi_partitioning!");
    }
    get_number_elements_on_other_cpus(nelements, nelements_at_cpu, nbproc, comm);

    displs[0] = 0;
    for (size_t i = 1; i < size_t(nbproc); ++i)
    {
        displs[i] = displs[i - 1] + nelements_at_cpu[i - 1];
    }
}

void get_full_array(const Vector& localArray, size_t nelements, Vector& fullArray, bool bcast, int nbproc, const void *comm)
{
    if (nbproc == 1)
    { // Single CPU: no MPI needed.
        fullArray = localArray;
        return;
    }

#ifdef HAVE_MPI
    const MPI_Comm *mpi_comm = static_cast<const MPI_Comm*>(comm);

    IVector displs(nbproc);
    IVector nelements_at_cpu(nbproc);

    // Get partitioning for MPI_Gatherv.
    get_mpi_partitioning(nelements, displs, nelements_at_cpu, nbproc, comm);

    // Gather the full vector.
    // Perform const cast for back compatibility, because interface was changed in OpenMPI v.1.7 (const was added to the first argument).
    MPI_Gatherv(const_cast<Vector&>(localArray).data(), nelements, MPI_DOUBLE, fullArray.data(),
                nelements_at_cpu.data(), displs.data(), MPI_DOUBLE, 0, *mpi_comm);

    if (bcast)
    { // Cast the array to all CPUs.
        size_t nelements_total = 0;
        for (size_t i = 0; i < size_t(nbproc); ++i)
        {
            nelements_total += nelements_at_cpu[i];
        }
        MPI_Bcast(fullArray.data(), nelements_total, MPI_DOUBLE, 0, *mpi_comm);
    }
#else
    // Should not come here.
    assert(false);
#endif
}

void get_full_array_in_place(size_t nelements, Vector& array, bool bcast, int myrank, int nbproc, const void *comm)
{
    if (nbproc == 1)
    { // Single CPU: no MPI needed.
        return;
    }

#ifdef HAVE_MPI
    const MPI_Comm *mpi_comm = static_cast<const MPI_Comm*>(comm);

    IVector displs(nbproc);
    IVector nelements_at_cpu(nbproc);

    // Get partitioning for MPI_Gatherv.
    get_mpi_partitioning(nelements, displs, nelements_at_cpu, nbproc, comm);

    // Gather the full vector.
    if (myrank == 0)
    {
        MPI_Gatherv(MPI_IN_PLACE, nelements, MPI_DOUBLE, array.data(),
                    nelements_at_cpu.data(), displs.data(), MPI_DOUBLE, 0, *mpi_comm);
    }
    else
    {
        MPI_Gatherv(array.data(), nelements, MPI_DOUBLE, array.data(),
                    nelements_at_cpu.data(), displs.data(), MPI_DOUBLE, 0, *mpi_comm);
    }

    if (bcast)
    { // Cast the array to all CPUs.
        size_t nelements_total = 0;
        for (size_t i = 0; i < size_t(nbproc); ++i)
        {
            nelements_total += nelements_at_cpu[i];
        }
        MPI_Bcast(array.data(), nelements_total, MPI_DOUBLE, 0, *mpi_comm);
    }
#else
    // Should not come here.
    assert(false);
#endif
}

}}} // namespace askap.lsqr.ParallelTools



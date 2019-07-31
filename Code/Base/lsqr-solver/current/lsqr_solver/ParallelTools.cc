/*
 * ParallelTools.cc
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#ifdef HAVE_MPI

#include <cstddef>
#include <stdexcept>

#include <lsqr_solver/ParallelTools.h>

namespace askap { namespace lsqr { namespace ParallelTools {

// Private function.
static void get_number_elements_on_other_cpus(size_t nelements, IVector& nelements_at_cpu, int nbproc, const MPI_Comm &comm)
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

    MPI_Gather(&nelements, 1, MPI_INTEGER, nelements_at_cpu.data(), 1, MPI_INTEGER, 0, comm);
    MPI_Bcast(nelements_at_cpu.data(), nbproc, MPI_INTEGER, 0, comm);
}

size_t get_total_number_elements(size_t nelements, int nbproc, const MPI_Comm &comm)
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

size_t get_nsmaller(size_t nelements, int myrank, int nbproc, const MPI_Comm &comm)
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

// Private function.
static void get_mpi_partitioning(size_t nelements, IVector& displs, IVector& nelements_at_cpu, int nbproc, const MPI_Comm &comm)
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

void get_full_array(const Vector& localArray, size_t nelements, Vector& fullArray, bool bcast, int nbproc, const MPI_Comm &comm)
{
    if (nbproc == 1)
    { // Single CPU: no MPI needed.
        fullArray = localArray;
        return;
    }

    IVector displs(nbproc);
    IVector nelements_at_cpu(nbproc);

    // Get partitioning for MPI_Gatherv.
    get_mpi_partitioning(nelements, displs, nelements_at_cpu, nbproc, comm);

    // Gather the full vector.
    // Perform const cast for back compatibility, because interface was changed in OpenMPI v.1.7 (const was added to the first argument).
    MPI_Gatherv(const_cast<Vector&>(localArray).data(), nelements, MPI_DOUBLE, fullArray.data(),
                nelements_at_cpu.data(), displs.data(), MPI_DOUBLE, 0, comm);

    if (bcast)
    { // Cast the array to all CPUs.
        size_t nelements_total = 0;
        for (size_t i = 0; i < size_t(nbproc); ++i)
        {
            nelements_total += nelements_at_cpu[i];
        }
        MPI_Bcast(fullArray.data(), nelements_total, MPI_DOUBLE, 0, comm);
    }
}

void get_full_array_in_place(size_t nelements, Vector& array, bool bcast, int myrank, int nbproc, const MPI_Comm &comm)
{
    if (nbproc == 1)
    { // Single CPU: no MPI needed.
        return;
    }

    IVector displs(nbproc);
    IVector nelements_at_cpu(nbproc);

    // Get partitioning for MPI_Gatherv.
    get_mpi_partitioning(nelements, displs, nelements_at_cpu, nbproc, comm);

    // Gather the full vector.
    if (myrank == 0)
    {
        MPI_Gatherv(MPI_IN_PLACE, nelements, MPI_DOUBLE, array.data(),
                    nelements_at_cpu.data(), displs.data(), MPI_DOUBLE, 0, comm);
    }
    else
    {
        MPI_Gatherv(array.data(), nelements, MPI_DOUBLE, array.data(),
                    nelements_at_cpu.data(), displs.data(), MPI_DOUBLE, 0, comm);
    }

    if (bcast)
    { // Cast the array to all CPUs.
        size_t nelements_total = 0;
        for (size_t i = 0; i < size_t(nbproc); ++i)
        {
            nelements_total += nelements_at_cpu[i];
        }
        MPI_Bcast(array.data(), nelements_total, MPI_DOUBLE, 0, comm);
    }
}

}}} // namespace askap.lsqr.ParallelTools

#endif



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


static void __get_full_array_in_place(size_t nelements, void *array, bool bcast, int myrank, int nbproc, const MPI_Comm &comm, MPI_Datatype mpi_dt)
{
    IVector displs(nbproc);
    IVector nelements_at_cpu(nbproc);

    // Get partitioning for MPI_Gatherv.
    get_mpi_partitioning(nelements, displs, nelements_at_cpu, nbproc, comm);

    // Gather the full vector.
    if (myrank == 0)
    {
        MPI_Gatherv(MPI_IN_PLACE, nelements, mpi_dt, array,
                    nelements_at_cpu.data(), displs.data(), mpi_dt, 0, comm);
    }
    else
    {
        MPI_Gatherv(array, nelements, mpi_dt, array,
                    nelements_at_cpu.data(), displs.data(), mpi_dt, 0, comm);
    }

    if (bcast)
    { // Cast the array to all CPUs.
        size_t nelements_total = 0;
        for (size_t i = 0; i < size_t(nbproc); ++i)
        {
            nelements_total += nelements_at_cpu[i];
        }
        MPI_Bcast(array, nelements_total, mpi_dt, 0, comm);
    }
}


template <typename T>
struct mpi_traits {
};

template<>
struct mpi_traits<int> {
    static const MPI_Datatype type = MPI_INT;
};

template<>
struct mpi_traits<double> {
    static const MPI_Datatype type = MPI_DOUBLE;
};


template <typename ValueType>
void get_full_array_in_place(size_t nelements, std::vector<ValueType>& array, bool bcast, int myrank, int nbproc, const MPI_Comm &comm)
{
    if (nbproc == 1) {
    // Single CPU: no MPI needed.
        return;
    }

    MPI_Datatype mpi_dt = mpi_traits<ValueType>::type;

    __get_full_array_in_place(nelements, array.data(), bcast, myrank, nbproc, comm, mpi_dt);
}

template void get_full_array_in_place<double>(size_t, std::vector<double>&, bool, int, int, const MPI_Comm &);
template void get_full_array_in_place<int>(size_t, std::vector<int>&, bool, int, int, const MPI_Comm &);


}}} // namespace askap.lsqr.ParallelTools

#endif



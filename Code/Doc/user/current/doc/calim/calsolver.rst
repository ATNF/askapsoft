Calibration solvers
===================

Two calibration solvers are available in ASKAPsoft. The *solver* parset parameter defines 
the type of the calibration solver to use with the choice between *SVD*, and *LSQR*. The *SVD* solver
is based on calculation of the singular value decomposition. It generally performs well for small problem sizes,
but shows poor performance for large number of anthennas. It does not require setting free parameters.
The iterative least-squares *LSQR* solver is a high performance solver, that is capable of performing calibration 
on the SKA scales (i.e., very large number of antennas). It scales nearly linearly with the number of data, i.e.,
quadratically with the number of antennas.
It requires setting several free parameters, such as stopping criteria and damping parameters.
Due to its general least-square nature, it also allows adding additional constraints into the system of equations 
(for example, solution smoothness).

Common Parameters
-----------------

Parameters for all solvers:

+-------------------+--------------+--------------+--------------------------------------------------------+
|**Parameter**      |**Type**      |**Default**   |**Description**                                         |
+===================+==============+==============+========================================================+
|solver             |string        |SVD           |Selection of solver. Either "SVD" or "LSQR".            |
+-------------------+--------------+--------------+--------------------------------------------------------+

The **SVD** solver does not require any additional parameters.
Additional parameters understood by the **LSQR** solver are given in the following section.

LSQR Solver Parameters
----------------------

All parameters given in the next table have **solver.LSQR** prefix (e.g., Cbpcalibrator.solver.LSQR.niter).

+-------------------+--------------+--------------+--------------------------------------------------------+
|**Parameter**      |**Type**      |**Default**   |**Description**                                         |
+===================+==============+==============+========================================================+
|rmin               |float         |1.e-13        |Minimum relative residual stopping criterion.           |
+-------------------+--------------+--------------+--------------------------------------------------------+
|niter              |int           |100           |Maximum number of minor cycles.                         |
|                   |              |              |This stopping criterion is used when minimum relative   |
|                   |              |              |residual stopping criterion could not be reached within |
|                   |              |              |the specified number of solver iterations.              |
+-------------------+--------------+--------------+--------------------------------------------------------+
|alpha              |float         |0.01          |The damping on the solution perturbation (that is       |
|                   |              |              |performed at every major cycle). Nonzero damping values |
|                   |              |              |help dealing with rank deficient systems.               |
+-------------------+--------------+--------------+--------------------------------------------------------+
|verbose            |string        |false         |The value of "true" enables lots of output.             |
+-------------------+--------------+--------------+--------------------------------------------------------+


Examples
--------

LSQR Solver
~~~~~~~~~~~

.. code-block:: bash

    Cbpcalibrator.solver                            = LSQR
    Cbpcalibrator.solver.LSQR.niter                 = 100
    Cbpcalibrator.solver.LSQR.rmin                  = 1.e-13
    Cbpcalibrator.solver.LSQR.alpha                 = 0.01
    Cbpcalibrator.solver.LSQR.verbose               = false

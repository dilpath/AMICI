#include "amici/steadystateproblem.h"
#include "amici/defines.h"
#include "amici/model.h"
#include "amici/solver.h"
#include "amici/solver_cvodes.h"
#include "amici/edata.h"
#include "amici/forwardproblem.h"
#include "amici/newton_solver.h"
#include "amici/rdata.h"

#include <cmath>
#include <cstring>
#include <ctime>
#include <sundials/sundials_dense.h>
#include <memory>
#include <cvodes/cvodes.h>

namespace amici {

SteadystateProblem::SteadystateProblem(const Solver *solver,
                                       const AmiVector &x0):
    t(solver->gett()), delta(solver->nx()), ewt(solver->nx()),
    rel_x_newton(solver->nx()), x_newton(solver->nx()), x(x0),
    x_old(solver->nx()), dx(solver->nx()), xdot(solver->nx()),
    xdot_old(solver->nx()), sx(solver->getStateSensitivity(solver->gett())),
    sdx(solver->nx(), solver->nplist()), xQB(solver->nquad()) {}

void SteadystateProblem::workSteadyStateProblem(ReturnData *rdata,
                                               Solver *solver, Model *model,
                                               int it) {
    /**
     * Tries to determine the steady state of the ODE system by a Newton
     * solver, uses forward intergration, if the Newton solver fails,
     * restarts Newton solver, if integration fails.
     * Computes steady state sensitivities
     *
     * @param solver pointer to the AMICI solver object
     * @param model pointer to the AMICI model object
     * @param it integer with the index of the current time step
     * @param rdata pointer to the return data object
     */
    double run_time;
    clock_t starttime;

    /* First, try to do Newton steps */
    starttime = clock();

    auto newtonSolver = NewtonSolver::getSolver(
        &t, &x, solver->getLinearSolver(), model, rdata,
        solver->getNewtonMaxLinearSteps(), solver->getNewtonMaxSteps(),
        solver->getAbsoluteTolerance(), solver->getRelativeTolerance(),
        solver->getNewtonDampingFactorMode(),
        solver->getNewtonDampingFactorLowerBound());

    auto newton_status = NewtonStatus::failed;
    try {
        applyNewtonsMethod(rdata, model, newtonSolver.get(),
                           NewtonStatus::newt);
        newton_status = NewtonStatus::newt;
    } catch (NewtonFailure const &ex1) {
        try {
            /* Newton solver did not work, so try a simulation */
            if (it < 1) /* No previous time point computed, set t = t0 */
                t = model->t0();
            else /* Carry on simulating from last point */
                t = model->getTimepoint(it - 1);
            if (it < 0) {
                /* Preequilibration? -> Create a new CVode object for sim */
                auto newtonSimSolver =
                    createSteadystateSimSolver(solver, model, false);
                getSteadystateSimulation(rdata, newtonSimSolver.get(), model);
            } else {
                /* Solver was already created, use this one */
                getSteadystateSimulation(rdata, solver, model);
            }
            newton_status = NewtonStatus::newt_sim;
        } catch (AmiException const &ex2) {
            /* may be integration failure from AmiSolve, so NewtonFailure
               won't do for all cases */
            try {
                applyNewtonsMethod(rdata, model, newtonSolver.get(),
                                   NewtonStatus::newt_sim_newt);
                newton_status = NewtonStatus::newt_sim_newt;
            } catch (NewtonFailure const &ex3) {
                if (ex3.error_code == AMICI_TOO_MUCH_WORK)
                    throw AmiException("Steady state computation failed to "
                                       "converge within the allowed maximum "
                                       "number of iterations");
                throw;
            }
        }
    }
    run_time = (double)((clock() - starttime) * 1000) / CLOCKS_PER_SEC;

    /* Compute steady state sensitvities */

    if (solver->getSensitivityOrder() >= SensitivityOrder::first &&
        (newton_status == NewtonStatus::newt ||
         newton_status == NewtonStatus::newt_sim_newt ||
         model->getSteadyStateSensitivityMode() != SteadyStateSensitivityMode::simulationFSA))
        // for newton_status == 2 the sensis were computed via FSA
        newtonSolver->computeNewtonSensis(sx);

    /* Get output of steady state solver, write it to x0 and reset time if necessary */
    writeNewtonOutput(rdata, model, newton_status, run_time, it);
}

void SteadystateProblem::workSteadyStateBackwardProblem(ReturnData *rdata, Solver *solver,
                                                        const ExpData *edata, Model *model)
{
    realtype T = std::numeric_limits<realtype>::infinity();

    // Compute the linear system JB*v = dJydy
    auto newtonSolver = NewtonSolver::getSolver(
        &T, &x, solver->getLinearSolver(), model, rdata,
        solver->getNewtonMaxLinearSteps(), solver->getNewtonMaxSteps(),
        solver->getAbsoluteTolerance(), solver->getRelativeTolerance(),
        solver->getNewtonDampingFactorMode(),
        solver->getNewtonDampingFactorLowerBound());

    // Construct the right hand size
    // FIXME: Could be simplified? What is the order on dJydx?
    std::vector<realtype> dJydx(model->nJ * model->nx_solver, 0.0);
    model->getAdjointStateObservableUpdate(dJydx, rdata->nt-1, x, *edata);

    amici::AmiVector rhs(model->nx_solver);
    for (int ix = 0; ix < model->nxtrue_solver; ix++) {
        for (int iJ = 0; iJ < model->nJ; iJ++)
            rhs[ix + iJ * model->nxtrue_solver] = dJydx[iJ + ix * model->nJ];
    }

    try {
        /* If no factorization of Jacobian wanted, skip this step */
        if (solver->getNewtonSolverBackward() == false)
            throw NewtonFailure(AMICI_TOO_MUCH_WORK, "workSteadyStateBackwardProblem");

        /* Try to algebraically solve the problem */
        newtonSolver->prepareLinearSystemB(0, -1);
        newtonSolver->solveLinearSystem(rhs);
    } catch (NewtonFailure const &ex1) {
        /* Integrate the simplified backward ODE with quadrature over xB */
        auto newtonSimSolverB =
            createSteadystateSimSolver(solver, model, true);
        getSteadystateSimulation(rdata, newtonSimSolver.get(), model);
    }

    // Compute the inner product v*dxotdp
    if (model->pythonGenerated)
    {
        const auto& plist = model->getParameterList();
        if (model->ndxdotdp_explicit > 0)
            model->dxdotdp_explicit.multiply(xQB.getNVector(), rhs.getNVector(), plist, true);
        if (model->ndxdotdp_implicit > 0)
            model->dxdotdp_implicit.multiply(xQB.getNVector(), rhs.getNVector(), plist, true);
    }
    else
    {
        // Matlab generated
        for (int ip=0; ip<model->nplist(); ++ip)
        xQB[ip] = N_VDotProd(rhs.getNVector(), model->dxdotdp.getNVector(ip));
    }
}

realtype SteadystateProblem::getWrmsNorm(const AmiVector &x,
                                         const AmiVector &xdot,
                                         realtype atol,
                                         realtype rtol
                                         ) {
    N_VAbs(x.getNVector(), ewt.getNVector());
    N_VScale(rtol, ewt.getNVector(), ewt.getNVector());
    N_VAddConst(ewt.getNVector(), atol, ewt.getNVector());
    N_VInv(ewt.getNVector(), ewt.getNVector());
    return N_VWrmsNorm(xdot.getNVector(), ewt.getNVector());
}

bool SteadystateProblem::checkConvergence(
                                         const Solver *solver,
                                         Model *model
                                         ) {
    model->fxdot(t, x, dx, xdot);
    wrms = getWrmsNorm(x, xdot, solver->getAbsoluteToleranceSteadyState(), solver->getRelativeToleranceSteadyState());
    bool converged = wrms < RCONST(1.0);
    if (solver->getSensitivityOrder()>SensitivityOrder::none &&
        solver->getSensitivityMethod() == SensitivityMethod::forward) {
        for (int ip = 0; ip < model->nplist(); ++ip) {
            if (converged) {
                sx = solver->getStateSensitivity(t);
                model->fsxdot(t, x, dx, ip, sx[ip], dx, xdot);
                wrms = getWrmsNorm(x, xdot,
                                   solver->getAbsoluteToleranceSteadyStateSensi(),
                                   solver->getRelativeToleranceSteadyStateSensi());
                converged = wrms < RCONST(1.0);
            }
        }
    }
    return converged;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */

void SteadystateProblem::applyNewtonsMethod(ReturnData *rdata, Model *model,
                                            NewtonSolver *newtonSolver,
                                            NewtonStatus steadystate_try) {
    int i_newtonstep = 0;
    int ix = 0;
    double gamma = 1.0;
    bool compNewStep = TRUE;

    /* initialize output of linear solver for Newton step */
    delta.reset();

    model->fxdot(t, x, dx,xdot);

    /* Check for relative error, but make sure not to divide by 0!
        Ensure positivity of the state */
    x_newton = x;
    x_old = x;
    xdot_old = xdot;

    //rdata->newton_numsteps[newton_try - 1] = 0.0;
    wrms = getWrmsNorm(x_newton, xdot, newtonSolver->atol, newtonSolver->rtol);
    bool converged = wrms < RCONST(1.0);
    while (!converged && i_newtonstep < newtonSolver->maxsteps) {

        /* If Newton steps are necessary, compute the inital search direction */
        if (compNewStep) {
            try {
                delta = xdot;
                newtonSolver->getStep(steadystate_try == NewtonStatus::newt ? 1
                                                                            : 2,
                                      i_newtonstep, delta);
            } catch (NewtonFailure const &ex) {
                rdata->newton_numsteps.at(steadystate_try == NewtonStatus::newt
                                              ? 0
                                              : 2) = i_newtonstep;
                throw;
            } catch (std::exception const &ex) {
                rdata->newton_numsteps.at(steadystate_try == NewtonStatus::newt
                                              ? 0
                                              : 2) = i_newtonstep;
                throw AmiException("Newton solver failed to compute new step: "
                                   "%s", ex.what());
            }
        }

        /* Try a full, undamped Newton step */
        N_VLinearSum(1.0, x_old.getNVector(), gamma, delta.getNVector(),
                     x.getNVector());

        /* Compute new xdot and residuals */
        model->fxdot(t, x, dx, xdot);
        realtype wrms_tmp = getWrmsNorm(x_newton, xdot, newtonSolver->atol,
                                        newtonSolver->rtol);

        if (wrms_tmp < wrms) {
            /* If new residuals are smaller than old ones, update state */
            wrms = wrms_tmp;
            x_old = x;
            xdot_old = xdot;
            /* New linear solve due to new state */
            compNewStep = TRUE;
            /* Check residuals vs tolerances */
            converged = wrms < RCONST(1.0);

            if (converged) {
                /* Ensure positivity of the found state and recheck if
                   the convergence still holds */
                bool recheck_convergence = false;
                for (ix = 0; ix < model->nx_solver; ix++) {
                    if (x[ix] < 0.0) {
                        x[ix] = 0.0;
                        recheck_convergence = true;
                    }
                }
                if (recheck_convergence) {
                  model->fxdot(t, x, dx, xdot);
                  wrms = getWrmsNorm(x_newton, xdot, newtonSolver->atol, newtonSolver->rtol);
                  converged = wrms < RCONST(1.0);
                }
            } else if (newtonSolver->dampingFactorMode==NewtonDampingFactorMode::on) {
                /* increase dampening factor (superfluous, if converged) */
                gamma = fmin(1.0, 2.0 * gamma);
            }
        } else if (newtonSolver->dampingFactorMode==NewtonDampingFactorMode::on) {
            /* Reduce dampening factor and raise an error when becomes too small */
            gamma = gamma / 4.0;
            if (gamma < newtonSolver->dampingFactorLowerBound)
              throw AmiException("Newton solver failed: a damping factor reached its lower bound");

            /* No new linear solve, only try new dampening */
            compNewStep = FALSE;
        }
        /* increase step counter */
        i_newtonstep++;
    }

    /* Set return values */
    rdata->newton_numsteps.at(steadystate_try == NewtonStatus::newt ? 0 : 2) =
        i_newtonstep;
    if (!converged)
        throw NewtonFailure(AMICI_TOO_MUCH_WORK, "applyNewtonsMethod");
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */

void SteadystateProblem::writeNewtonOutput(ReturnData *rdata,
                                           const Model *model,
                                           const NewtonStatus newton_status,
                                           const double run_time, const int it)
{

    /* Get cpu time for Newton solve in seconds */
    rdata->newton_cpu_time = run_time / 1000;
    rdata->newton_status = static_cast<int>(newton_status);
    rdata->wrms_steadystate = wrms;
    if (newton_status == NewtonStatus::newt_sim) {
        rdata->t_steadystate = t;
    }

    /* Steady state was found: set t to t0 if preeq, otherwise to inf */
    if (it == AMICI_PREEQUILIBRATE) {
        t = model->t0();
    } else {
        t = INFINITY;
    }
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */

void SteadystateProblem::getSteadystateSimulation(ReturnData *rdata,
                                                  Solver *solver,
                                                  Model *model)
{
    /* Loop over steps and check for convergence */
    bool converged = checkConvergence(solver, model);

    int steps_newton = 0;
    while(!converged) {
        /* One step of ODE integration
         reason for tout specification:
         max with 1 ensures correct direction (any positive value would do)
         multiplication with 10 ensures nonzero difference and should ensure stable computation
         value is not important for AMICI_ONE_STEP mode, only direction w.r.t. current t
         */
        solver->step(std::max(t, 1.0) * 10);
        solver->writeSolution(&t, x, dx, sx);

        /* Check for convergence */
        converged = checkConvergence(solver, model);
        /* increase counter, check for maxsteps */
        steps_newton++;
        if (steps_newton >= solver->getMaxSteps() && !converged) {
            rdata->newton_numsteps.at(static_cast<int>(NewtonStatus::newt_sim) - 1) =
                steps_newton;
            throw NewtonFailure(AMICI_TOO_MUCH_WORK, "exceeded maximum number of steps");
        }
    }
    rdata->newton_numsteps.at(static_cast<int>(NewtonStatus::newt_sim) - 1) =
        steps_newton;
    if (solver->getSensitivityOrder()>SensitivityOrder::none)
        sx = solver->getStateSensitivity(t);
}

std::unique_ptr<Solver> SteadystateProblem::createSteadystateSimSolver(
        const Solver *solver, Model *model, const bool backward) const
{
    /* Create new CVode solver object */
    auto newton_solver = std::unique_ptr<Solver>(solver->clone());
    switch(solver->getLinearSolver()) {
        case LinearSolver::dense:
        case LinearSolver::KLU:
        case LinearSolver::SuperLUMT:
            break;
        default:
            throw NewtonFailure(AMICI_NOT_IMPLEMENTED, "invalid solver for steadystate simulation");
    }

    /* Do we have to integrate sensitivities? */
    newton_solver->setSensitivityMethod(SensitivityMethod::none);
    if (solver->getSensitivityMethod() != SensitivityMethod::none
        && model->getSensitivityMethod() == SensitivityMethod::forward
        && model->getSteadyStateSensitivityMode() == SteadyStateSensitivityMode::simulationFSA)
        newton_solver->setSensitivityMethod(SensitivityMethod::forward);

    // use x and sx as dummies for dx and sdx (they wont get touched in a CVodeSolver)
    newton_solver->setup(model->t0(), model, x, x, sx, sx);

    // If we do the simplified backward integration, we have some changes
    if (backward) {
        // model->fxdot = <use backward dummy>;
        /* Initialise quadrature calculation */
        // newton_solver->qinit(0, xQB0);
        /* set quadrature tolerances */
        newton_solver->applyQuadTolerancesASA(*which);
    }

    return newton_solver;
}

void SteadystateProblem::writeSolution(realtype *t, AmiVector &x,
                                       AmiVectorArray &sx) const {
    *t = this->t;
    x.copy(this->x);
    sx.copy(this->sx);
}

void SteadystateProblem::writeSolutionBackward(AmiVector &xQB) const {
    xQB.copy(this->xQB);
}

} // namespace amici

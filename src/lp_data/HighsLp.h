/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                       */
/*    This file is part of the HiGHS linear optimization suite           */
/*                                                                       */
/*    Written and engineered 2008-2020 at the University of Edinburgh    */
/*                                                                       */
/*    Available as open-source under the MIT License                     */
/*                                                                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**@file lp_data/HighsLp.h
 * @brief
 * @author Julian Hall, Ivet Galabova, Qi Huangfu and Michael Feldmeier
 */
#ifndef LP_DATA_HIGHS_LP_H_
#define LP_DATA_HIGHS_LP_H_

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "HConfig.h"
#include "lp_data/HConst.h"        // For HiGHS strategy options
#include "simplex/SimplexConst.h"  // For simplex strategy options

enum class LpAction {
  DUALISE = 0,
  PERMUTE,
  SCALE,
  NEW_COSTS,
  NEW_BOUNDS,
  NEW_BASIS,
  NEW_COLS,
  NEW_ROWS,
  DEL_COLS,
  DEL_ROWS,
  DEL_ROWS_BASIS_OK
};

enum class HighsModelStatus {
  NOTSET = 0,
  HIGHS_MODEL_STATUS_MIN = NOTSET,
  LOAD_ERROR,
  MODEL_ERROR,
  PRESOLVE_ERROR,
  SOLVE_ERROR,
  POSTSOLVE_ERROR,
  MODEL_EMPTY,
  PRIMAL_INFEASIBLE,
  PRIMAL_UNBOUNDED,
  OPTIMAL,
  REACHED_DUAL_OBJECTIVE_VALUE_UPPER_BOUND,
  REACHED_TIME_LIMIT,
  REACHED_ITERATION_LIMIT,
  HIGHS_MODEL_STATUS_MAX = REACHED_ITERATION_LIMIT
};

/** SCIP/HiGHS Objective sense */
enum class ObjSense { MINIMIZE = 1, MAXIMIZE = -1 };

class HighsLp;

class HighsLp {
 public:
  // Model data
  int numCol_ = 0;
  int numRow_ = 0;

  std::vector<int> Astart_;
  std::vector<int> Aindex_;
  std::vector<double> Avalue_;
  std::vector<double> colCost_;
  std::vector<double> colLower_;
  std::vector<double> colUpper_;
  std::vector<double> rowLower_;
  std::vector<double> rowUpper_;

  ObjSense sense_ = ObjSense::MINIMIZE;
  double offset_ = 0;

  std::string model_name_ = "";
  std::string lp_name_ = "";

  std::vector<std::string> row_names_;
  std::vector<std::string> col_names_;

  std::vector<int> integrality_;

  bool equalButForNames(const HighsLp& lp);
  bool operator==(const HighsLp& lp);
};

// Cost, column and row scaling factors
struct HighsScale {
  bool is_scaled_ = false;
  double cost_;
  std::vector<double> col_;
  std::vector<double> row_;
};

struct SimplexBasis {
  // The basis for the simplex method consists of basicIndex,
  // nonbasicFlag and nonbasicMove. If HighsSimplexLpStatus has_basis
  // is true then it is assumed that basicIndex_ and nonbasicFlag_ are
  // self-consistent and correpond to the dimensions of an associated
  // HighsLp, but the basis matrix B is not necessarily nonsingular.
  std::vector<int> basicIndex_;
  std::vector<int> nonbasicFlag_;
  std::vector<int> nonbasicMove_;
};

struct HighsSimplexLpStatus {
  // Status of LP solved by the simplex method and its data
  bool valid = false;
  bool is_dualised = false;
  bool is_permuted = false;
  bool scaling_tried = false;
  bool has_basis = false;            // The simplex LP has a valid simplex basis
  bool has_matrix_col_wise = false;  // The HMatrix column-wise matrix is valid
  bool has_matrix_row_wise = false;  // The HMatrix row-wise matrix is valid
  bool has_factor_arrays =
      false;  // Has the arrays for the representation of B^{-1}
  bool has_dual_steepest_edge_weights = false;  // The DSE weights are known
  bool has_nonbasic_dual_values = false;  // The nonbasic dual values are known
  bool has_basic_primal_values = false;   // The basic primal values are known
  bool has_invert =
      false;  // The representation of B^{-1} corresponds to the current basis
  bool has_fresh_invert = false;  // The representation of B^{-1} corresponds to
                                  // the current basis and is fresh
  bool has_fresh_rebuild = false;  // The data are fresh from rebuild
  bool has_dual_objective_value =
      false;  // The dual objective function value is known
  bool has_primal_objective_value =
      false;  // The dual objective function value is known
  SimplexSolutionStatus solution_status =
      SimplexSolutionStatus::UNSET;  // The solution status is UNSET
};

struct HighsSimplexInfo {
  bool initialised = false;
  // Simplex information regarding primal solution, dual solution and
  // objective for this Highs Model Object. This is information which
  // should be retained from one run to the next in order to provide
  // hot starts.
  //
  // Part of working model which are assigned and populated as much as
  // possible when a model is being defined

  // workCost: Originally just costs from the model but, in solve(), may
  // be perturbed or set to alternative values in Phase I??
  //
  // workDual: Values of the dual variables corresponding to
  // workCost. Latter not known until solve() is called since B^{-1}
  // is required to compute them. Knowledge of them is indicated by
  // has_nonbasic_dual_values
  //
  // workShift: Values added to workCost in order that workDual
  // remains feasible, thereby remaining dual feasible in phase 2
  //
  std::vector<double> workCost_;
  std::vector<double> workDual_;
  std::vector<double> workShift_;

  // workLower/workUpper: Originally just lower (upper) bounds from
  // the model but, in solve(), may be perturbed or set to
  // alternative values in Phase I??
  //
  // workRange: Distance between lower and upper bounds
  //
  // workValue: Values of the nonbasic variables corresponding to
  // workLower/workUpper and the basis. Always known.
  //
  std::vector<double> workLower_;
  std::vector<double> workUpper_;
  std::vector<double> workRange_;
  std::vector<double> workValue_;

  // baseLower/baseUpper/baseValue: Lower and upper bounds on the
  // basic variables and their values. Latter not known until solve()
  // is called since B^{-1} is required to compute them. Knowledge of
  // them is indicated by has_basic_primal_values
  //
  std::vector<double> baseLower_;
  std::vector<double> baseUpper_;
  std::vector<double> baseValue_;
  //
  // Vectors of random reals for column cost perturbation, a random
  // permutation of all indices for CHUZR and a random permutation of
  // column indices for permuting the columns
  std::vector<double> numTotRandomValue_;
  std::vector<int> numTotPermutation_;
  std::vector<int> numColPermutation_;

  std::vector<int> devex_index_;

  // Options from HighsOptions for the simplex solver
  int simplex_strategy;
  int dual_edge_weight_strategy;
  int primal_edge_weight_strategy;
  int price_strategy;

  double dual_simplex_cost_perturbation_multiplier;
  int update_limit;

  // Internal options - can't be changed externally
  bool run_quiet = false;
  bool store_squared_primal_infeasibility = false;
#ifndef HiGHSDEV
  bool analyse_lp_solution = false;  // true;//
#else
  bool analyse_lp_solution = true;
  // Options for reporting timing
  bool report_simplex_inner_clock = false;
  bool report_simplex_outer_clock = false;
  bool report_simplex_phases_clock = false;
  bool report_HFactor_clock = false;
  // Option for analysing the LP simplex iterations, INVERT time and rebuild
  // time
  bool analyse_lp = false;
  bool analyse_iterations = false;
  bool analyse_invert_form = false;
  bool analyse_invert_condition = false;
  bool analyse_invert_time = false;
  bool analyse_rebuild_time = false;
#endif
  // Simplex runtime information
  int allow_cost_perturbation = true;
  int costs_perturbed = 0;

  int num_primal_infeasibilities = -1;
  double max_primal_infeasibility;
  double sum_primal_infeasibilities;
  int num_dual_infeasibilities = -1;
  double max_dual_infeasibility;
  double sum_dual_infeasibilities;

  // Records of cumulative iteration counts - updated at the end of a phase
  int dual_phase1_iteration_count = 0;
  int dual_phase2_iteration_count = 0;
  int primal_phase1_iteration_count = 0;
  int primal_phase2_iteration_count = 0;

  int min_threads = 1;
  int num_threads = 1;
  int max_threads = HIGHS_THREAD_LIMIT;

  // Cutoff for PAMI
  double pami_cutoff = 0.95;

  // Info on PAMI iterations
  int multi_iteration = 0;

  // Number of UPDATE operations performed - should be zeroed when INVERT is
  // performed
  int update_count;
  // Value of dual objective - only set when computed from scratch in dual
  // rebuild()
  double dual_objective_value;
  // Value of primal objective - only set when computed from scratch in primal
  // rebuild()
  double primal_objective_value;

  // Value of dual objective that is updated in dual simplex solver
  double updated_dual_objective_value;
  // Value of primal objective that is updated in primal simplex solver
  double updated_primal_objective_value;
  // Number of logical variables in the basis
  int num_basic_logicals;

#ifdef HiGHSDEV
  // Analysis of INVERT
  int num_invert = 0;
  // Analysis of INVERT form
  int num_kernel = 0;
  int num_major_kernel = 0;
  const double major_kernel_relative_dim_threshhold = 0.1;
  double max_kernel_dim = 0;
  double sum_kernel_dim = 0;
  double running_average_kernel_dim = 0;
  double sum_invert_fill_factor = 0;
  double sum_kernel_fill_factor = 0;
  double sum_major_kernel_fill_factor = 0;
  double running_average_invert_fill_factor = 1;
  double running_average_kernel_fill_factor = 1;
  double running_average_major_kernel_fill_factor = 1;

  int total_inverts;
  double total_invert_time;
  double invert_condition = 1;
#endif

  /*
#ifdef HiGHSDEV
  // Move this to Simplex class once it's created
  vector<int> historyColumnIn;
  vector<int> historyColumnOut;
  vector<double> historyAlpha;
#endif
  */
};

struct HighsSolutionParams {
  // Input to solution analysis method
  double primal_feasibility_tolerance;
  double dual_feasibility_tolerance;
  int primal_status = PrimalDualStatus::STATUS_NOTSET;
  int dual_status = PrimalDualStatus::STATUS_NOTSET;
  // Output from solution analysis method
  double objective_function_value;
  int num_primal_infeasibilities;
  double sum_primal_infeasibilities;
  double max_primal_infeasibility;
  int num_dual_infeasibilities;
  double sum_dual_infeasibilities;
  double max_dual_infeasibility;
};

struct HighsIterationCounts {
  int simplex = 0;
  int ipm = 0;
  int crossover = 0;
};

struct HighsSolution {
  std::vector<double> col_value;
  std::vector<double> col_dual;
  std::vector<double> row_value;
  std::vector<double> row_dual;
};

// To be the basis representation given back to the user. Values of
// HighsBasisStatus are defined in HConst.h
struct HighsBasis {
  bool valid_ = false;
  std::vector<HighsBasisStatus> col_status;
  std::vector<HighsBasisStatus> row_status;
};

struct HighsRanging {
  std::vector<double> colCostRangeUpValue_;
  std::vector<double> colCostRangeUpObjective_;
  std::vector<int> colCostRangeUpInCol_;
  std::vector<int> colCostRangeUpOutCol_;
  std::vector<double> colCostRangeDnValue_;
  std::vector<double> colCostRangeDnObjective_;
  std::vector<int> colCostRangeDnInCol_;
  std::vector<int> colCostRangeDnOutCol_;
  std::vector<double> rowBoundRangeUpValue_;
  std::vector<double> rowBoundRangeUpObjective_;
  std::vector<int> rowBoundRangeUpInCol_;
  std::vector<int> rowBoundRangeUpOutCol_;
  std::vector<double> rowBoundRangeDnValue_;
  std::vector<double> rowBoundRangeDnObjective_;
  std::vector<int> rowBoundRangeDnInCol_;
  std::vector<int> rowBoundRangeDnOutCol_;
};

// Make sure the dimensions of solution and basis are the same as
// numRow_ and numCol_
bool isSolutionConsistent(const HighsLp& lp, const HighsSolution& solution);
bool isBasisConsistent(const HighsLp& lp, const HighsBasis& basis);

// If debug this method terminates the program when the status is not OK. If
// standard build it only prints a message.
// void checkStatus(HighsStatus status);

void clearSolutionUtil(HighsSolution& solution);
void clearBasisUtil(HighsBasis& solution);
void clearLp(HighsLp& lp);

#endif

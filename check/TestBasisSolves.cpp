#include "Highs.h"
#include "HighsRandom.h"

#include "catch.hpp"

#ifdef __linux__
#include <unistd.h>
#elif _WIN32
#define NOGDI
#include <windows.h>
#else

#endif

std::string GetBasisSolvesCurrentWorkingDir(void) {
  char buff[FILENAME_MAX];

  #ifdef __linux__ 
    auto result = getcwd(buff, FILENAME_MAX);
    if (result) {
    std::string current_working_dir(buff);
    return current_working_dir;
  }
  #elif _WIN32
    GetModuleFileName( NULL, buff, FILENAME_MAX );
    string::size_type pos = string( buff ).find_last_of( "\\/" );
    return string( buff ).substr( 0, pos);
  #else

  #endif

  return "";
}

double GetBasisSolvesCheckSolution(HighsLp& lp, int* basic_variables, double* rhs, double* solution, const bool transpose=false) {
  double residual_norm = 0;
  //  for (int k=0; k<lp.numRow_; k++) printf("solution[%2d]=%11.4g\n", k, solution[k]);
  if (transpose) {
    for (int k=0; k<lp.numRow_; k++) {
      double residual = 0;
      int var = basic_variables[k];
      if (var < 0) {
	int row = -(1+var);
	residual = fabs(rhs[k] - solution[row]);
	if (residual > 1e-8) printf("Row |[B^Tx-b]_{%2d}| = %11.4g\n", k,  residual);
      } else {
	int col = var;
	for (int el=lp.Astart_[col]; el<lp.Astart_[col+1]; el++) {
	  int row = lp.Aindex_[el];
	  residual += lp.Avalue_[el]*solution[row];
	  //	  printf("k=%1d; col=%1d; el=%1d; row=%1d; lp.Avalue_[col]=%11.4g; solution[row]=%11.4g; residual=%1.4g\n", k, col, el, row, lp.Avalue_[col], solution[row], residual);
	}
	residual = fabs(rhs[k] - residual);
	if (residual > 1e-8) printf("Col |[B^Tx-b]_{%2d}| = %11.4g\n", k,  residual);
      }
      residual_norm += residual;
    }
  } else {
    vector<double> basis_matrix_times_solution;
    basis_matrix_times_solution.assign(lp.numRow_, 0);
    for (int k=0; k<lp.numRow_; k++) {
      int var = basic_variables[k];
      if (var < 0) {
	int row = -(1+var);
	basis_matrix_times_solution[row] += solution[k];
      } else {
	int col = var;
	for (int el=lp.Astart_[col]; el<lp.Astart_[col+1]; el++) {
	  int row = lp.Aindex_[el];
	  basis_matrix_times_solution[row] += lp.Avalue_[el]*solution[k];
	}
      }
    }
    for (int k=0; k<lp.numRow_; k++) {
      double residual = fabs(rhs[k] - basis_matrix_times_solution[k]);
      if (residual > 1e-8)
	printf("|[B^Tx-b]_{%2d}| = %11.4g\n", k,  residual); 
      residual_norm += residual;
    }
  }
  return residual_norm;
}

void GetBasisSolvesFormRHS(HighsLp& lp, int* basic_variables, double* solution, double* rhs, const bool transpose=false) {
  if (transpose) {
    for (int k=0; k<lp.numRow_; k++) {
      rhs[k] = 0;
      int var = basic_variables[k];
      if (var < 0) {
	int row = -(1+var);
	rhs[k] = solution[row];
      } else {
	int col = var;
	for (int el=lp.Astart_[col]; el<lp.Astart_[col+1]; el++) {
	  int row = lp.Aindex_[el];
	  rhs[k] += lp.Avalue_[el]*solution[row];
	}
      }
    }
  } else {
    for (int k=0; k<lp.numRow_; k++) rhs[k] = 0;
    for (int k=0; k<lp.numRow_; k++) {
      int var = basic_variables[k];
      if (var < 0) {
	int row = -(1+var);
	rhs[row] += solution[k];
      } else {
	int col = var;
	for (int el=lp.Astart_[col]; el<lp.Astart_[col+1]; el++) {
	  int row = lp.Aindex_[el];
	  rhs[row] += lp.Avalue_[el]*solution[k];
	}
      }
    }
  }
}

// No commas in test case name.
TEST_CASE("Basis-solves", "[highs_basis_solves]") {

  std::string dir = GetBasisSolvesCurrentWorkingDir();

  std::cout << dir << std::endl;

  // For debugging use the latter.
  std::string filename;
  filename = dir + "/../../check/instances/chip.mps";
  //  filename = dir + "/../../check/instances/blending.mps";
    filename = dir + "/../../check/instances/avgas.mps";
        filename = dir + "/../../check/instances/adlittle.mps";

  //For debugging

  //  filename = dir + "/check/instances/adlittle.mps";

  Highs highs;

  int* basic_variables;
  double* rhs;
  double* known_solution;
  double* solution;

  HighsStatus highs_status;

  highs_status = highs.getBasicVariables(basic_variables);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getBasisInverseRow(0, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getBasisInverseCol(0, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getBasisSolve(rhs, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getBasisTransposeSolve(rhs, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getReducedColumn(0, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.initializeFromFile(filename);
  REQUIRE(highs_status==HighsStatus::OK);
 
  HighsLp lp = highs.getLp();
  REQUIRE(highs_status == HighsStatus::OK);

  highs_status = highs.writeToFile("");
  REQUIRE(highs_status==HighsStatus::Warning);
 
  int numRow = lp.numRow_;
  int numCol = lp.numCol_;
  int check_row = 0;
  int check_col = 0;

  basic_variables = (int*)malloc(sizeof(int) * numRow);
  known_solution = (double*)malloc(sizeof(double) * numRow);
  solution = (double*)malloc(sizeof(double) * numRow);
  rhs = (double*)malloc(sizeof(double) * numRow);

  highs_status = highs.getBasicVariables(basic_variables);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getBasisInverseRow(check_row, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getBasisInverseCol(0, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getBasisSolve(rhs, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getBasisTransposeSolve(rhs, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.getReducedColumn(0, solution);
  REQUIRE(highs_status==HighsStatus::Error);

  highs_status = highs.run();
  REQUIRE(highs_status == HighsStatus::OK);

  highs_status = highs.getBasicVariables(basic_variables);
  REQUIRE(highs_status==HighsStatus::OK);

  /*
  for (int row=0; row < numRow; row++) {
    printf("Basic variable %3d is ", row);
    int var = basic_variables[row];
    if (var<0) {
      printf("row %d\n", -(1+var));
    } else {
      printf("col %d\n", var);
    }
  }
  */

  double residual_norm;
  double max_residual_norm;
  HighsRandom random;

  int max_k = min(numRow, 9);
  int k;

  int basic_col=0;
  /*
  k = 0;
  max_residual_norm=0;
  for (int row=0; row < numRow; row++) {
    int var = basic_variables[row];
    if (var>=0) {
      basic_col = var;
      int rhs_nnz = lp.Astart_[basic_col+1]-lp.Astart_[basic_col];
      printf("Row %2d; Var %3d; RHS nnz = %d\n", row, basic_col, rhs_nnz);
      for (int ix=0; ix<numRow; ix++) rhs[ix]=0;
      for (int el=lp.Astart_[basic_col]; el<lp.Astart_[basic_col+1]; el++) rhs[lp.Aindex_[el]] = lp.Avalue_[el];

      highs_status = highs.getBasisSolve(rhs, solution);
      REQUIRE(highs_status==HighsStatus::OK);
      residual_norm = GetBasisSolvesCheckSolution(lp, basic_variables, rhs, solution, false);
      max_residual_norm = std::max(residual_norm, max_residual_norm);
      //  if (residual_norm > 1e-8)
      printf("getBasisSolve(%d): residual_norm = %g\n\n", k, residual_norm);
      REQUIRE(fabs(residual_norm) < 1e-6);
      if (k<max_k) k++; else k*=2;
    }
    if (k>=numRow) break;
  }
  printf("\n!! Test set 0: max_residual_norm = %11.4g!!\n\n", max_residual_norm);
  */

  max_residual_norm=0;
  for (int ix=0; ix<numRow; ix++) known_solution[ix]=0;
  int col;
  int num_ix=3;
  col = 6;
  basic_col = basic_variables[col];
  known_solution[col] = 1;//random.fraction();
  printf("Known solution col %2d is basic_col %2d\n", col, basic_col);

  col = 15;
  basic_col = basic_variables[col];
  known_solution[col] = 1;//random.fraction();
  printf("Known solution col %2d is basic_col %2d\n", col, basic_col);

  if (num_ix>2) {
    col = 12;
    basic_col = basic_variables[col];
    known_solution[col] = 1;//random.fraction();
    printf("Known solution col %2d is basic_col %2d\n", col, basic_col);
  }

  GetBasisSolvesFormRHS(lp, basic_variables, known_solution, rhs);
  highs_status = highs.getBasisSolve(rhs, solution);
  REQUIRE(highs_status==HighsStatus::OK);
  residual_norm = GetBasisSolvesCheckSolution(lp, basic_variables, rhs, solution, false);
  max_residual_norm = std::max(residual_norm, max_residual_norm);
  //  if (residual_norm > 1e-8)
  printf("getBasisSolve(): residual_norm = %g\n", residual_norm);
  REQUIRE(fabs(residual_norm) < 1e-6);
  double solution_error_norm = 0;
  for (int ix=0; ix<numRow; ix++) {
    double solution_error = fabs(known_solution[ix]-solution[ix]);
    if (solution_error>1e-6) printf("Row %2d: |x-x^|_i = %11.4g\n", ix, solution_error);
    solution_error_norm += solution_error;
  }
  printf("getBasisSolve(): solution_error_norm = %g\n", solution_error_norm);
  
  
  printf("\n!! Test set 0.5: max_residual_norm = %11.4g!!\n\n", max_residual_norm);
  
  max_k = min(numRow, 9);
  /*
  k = 0;
  max_residual_norm=0;
  for (;;) {
    check_row = k;
    // Determine row check_row of B^{-1}
    highs_status = highs.getBasisInverseRow(check_row, solution);
    REQUIRE(highs_status==HighsStatus::OK);
    // Check solution
    // Set up RHS as e_{check_row}
    for (int row=0; row<numRow; row++) rhs[row]=0;
    rhs[check_row] = 1;
    residual_norm = GetBasisSolvesCheckSolution(lp, basic_variables, rhs, solution, true);
      max_residual_norm = std::max(residual_norm, max_residual_norm);
    //    if (residual_norm > 1e-8)
    printf("getBasisInverseRow(%d): residual_norm = %g\n", k, residual_norm);
    REQUIRE(fabs(residual_norm) < 1e-8);
    if (k<max_k) k++; else k*=2; if (k>=numRow) break;
  }
  printf("\n!! Test set 1: max_residual_norm = %11.4g!!\n\n", max_residual_norm);
  
  */
  k = 0;
  max_residual_norm=0;
  for (;;) {
    check_col = k;
    // Determine col check_col of B^{-1}
    highs_status = highs.getBasisInverseCol(check_col, solution);
    REQUIRE(highs_status==HighsStatus::OK);
    // Check solution
    // Set up RHS as e_{check_col}
    for (int row=0; row<numRow; row++) rhs[row]=0;
    rhs[check_col] = 1;
    residual_norm = GetBasisSolvesCheckSolution(lp, basic_variables, rhs, solution, false);
      max_residual_norm = std::max(residual_norm, max_residual_norm);
    //  if (residual_norm > 1e-8)
    printf("getBasisInverseCol(%d): residual_norm = %g\n\n", k, residual_norm);
    REQUIRE(fabs(residual_norm) < 1e-8);
    if (k<max_k) k++; else k*=2; if (k>=numRow) break;
  }
  printf("\n!! Test set 2: max_residual_norm = %11.4g!!\n\n", max_residual_norm);


  k = 0;
  max_residual_norm=0;
  for (;;) {
    for (int row=0; row<numRow; row++) rhs[row]=random.fraction();
    highs_status = highs.getBasisSolve(rhs, solution);
    REQUIRE(highs_status==HighsStatus::OK);
    // Check solution
    residual_norm = GetBasisSolvesCheckSolution(lp, basic_variables, rhs, solution, false);
      max_residual_norm = std::max(residual_norm, max_residual_norm);
    //  if (residual_norm > 1e-8)
    printf("getBasisSolve(%d): residual_norm = %g\n", k, residual_norm);
    REQUIRE(fabs(residual_norm) < 1e-8);
    if (k<max_k) k++; else k*=2; if (k>=numRow) break;
  }
  printf("\n!! Test set 3: max_residual_norm = %11.4g!!\n\n", max_residual_norm);

    

  /*
  k = 0;
  max_residual_norm=0;
  for (;;) {
    for (int row=0; row<numRow; row++) rhs[row]=random.fraction();
    highs_status = highs.getBasisTransposeSolve(rhs, solution);
    REQUIRE(highs_status==HighsStatus::OK);
    // Check solution
    residual_norm = GetBasisSolvesCheckSolution(lp, basic_variables, rhs, solution, true);
      max_residual_norm = std::max(residual_norm, max_residual_norm);
    //  if (residual_norm > 1e-8)
    printf("getBasisTransposeSolve(%d): residual_norm = %g\n", k, residual_norm);
    REQUIRE(fabs(residual_norm) < 1e-8);
    if (k<max_k) k++; else k*=2; if (k>=numRow) break;
  }
  printf("\n!! Test set 4: max_residual_norm = %11.4g!!\n\n", max_residual_norm);

  */
  k = 0;
  max_residual_norm=0;
  max_k = min(numCol, 9);
  for (;;) {
    check_col = k;
    highs_status = highs.getReducedColumn(check_col, solution);
    REQUIRE(highs_status==HighsStatus::OK);
    // Check solution
    for (int row=0; row<numRow; row++) rhs[row]=0;
    for (int el=lp.Astart_[check_col]; el<lp.Astart_[check_col+1]; el++) rhs[lp.Aindex_[el]] = lp.Avalue_[el];
    residual_norm = GetBasisSolvesCheckSolution(lp, basic_variables, rhs, solution, false);
      max_residual_norm = std::max(residual_norm, max_residual_norm);
    //  if (residual_norm > 1e-8)
    printf("getBasisTransposeSolve(%d): residual_norm = %g\n", k, residual_norm);
    REQUIRE(fabs(residual_norm) < 1e-8);
    if (k<max_k) k++; else k*=2; if (k>=numCol) break;
  }
  printf("\n!! Test set 5: max_residual_norm = %11.4g!!\n\n", max_residual_norm);
}
    
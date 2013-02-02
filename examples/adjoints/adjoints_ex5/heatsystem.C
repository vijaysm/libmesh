#include "libmesh/getpot.h"

#include "heatsystem.h"

#include "libmesh/fe_base.h"
#include "libmesh/fe_interface.h"
#include "libmesh/fem_context.h"
#include "libmesh/mesh.h"
#include "libmesh/quadrature.h"
#include "libmesh/string_to_enum.h"
#include "libmesh/system.h"
#include "libmesh/equation_systems.h"
#include "libmesh/zero_function.h"
#include "libmesh/boundary_info.h"
#include "libmesh/dirichlet_boundaries.h"
#include "libmesh/dof_map.h"

void HeatSystem::init_data ()
{
  T_var = this->add_variable ("T", static_cast<Order>(_fe_order),
                      Utility::string_to_enum<FEFamily>(_fe_family));

  // Make sure the input file heat.in exists, and parse it.
  {
    std::ifstream i("heat.in");
    if (!i)
      {
        std::cerr << '[' << libMesh::processor_id() 
                  << "] Can't find heat.in; exiting early." 
                  << std::endl; 
        libmesh_error();
      }
  }
  GetPot infile("heat.in");
  _k = infile("k", 1.0);
  _analytic_jacobians = infile("analytic_jacobians", true);

  parameters.push_back(_k);

  // Set equation system parameters _k and theta, so they can be read by the exact solution
  this->get_equation_systems().parameters.set<Real>("_k") = _k;

  this->time_evolving(T_var);

  const boundary_id_type all_ids[4] = {0, 1, 2, 3};
  std::set<boundary_id_type> all_bdys(all_ids, all_ids+(2*2));

  std::vector<unsigned int> T_only(1, T_var);

  ZeroFunction<Number> zero;

  this->get_dof_map().add_dirichlet_boundary
        (DirichletBoundary (all_bdys, T_only, &zero));
  
  FEMSystem::init_data();
}



void HeatSystem::init_context(DiffContext &context)
{
  FEMContext &c = libmesh_cast_ref<FEMContext&>(context);

  // Now make sure we have requested all the data
  // we need to build the linear system.
  c.element_fe_var[0]->get_JxW();  
  c.element_fe_var[0]->get_dphi();

  // We'll have a more automatic solution to preparing adjoint
  // solutions for time integration, eventually...
  if (c.is_adjoint())
    {
      // A reference to the system context is built with
      const System & sys = c.get_system();
  
      // Get a pointer to the adjoint solution vector
      NumericVector<Number> &adjoint_solution =
        const_cast<System &>(sys).get_adjoint_solution(0);

      // Add this adjoint solution to the vectors that diff context should localize 
      c.add_localized_vector(adjoint_solution, sys);
    }

  FEMSystem::init_context(context);
}

#define optassert(X) {if (!(X)) libmesh_error();}
//#define optassert(X) libmesh_assert(X);

bool HeatSystem::element_time_derivative (bool request_jacobian,
                                          DiffContext &context)
{
  bool compute_jacobian = request_jacobian && _analytic_jacobians;

  FEMContext &c = libmesh_cast_ref<FEMContext&>(context);

  // First we get some references to cell-specific data that
  // will be used to assemble the linear system.

  // Element Jacobian * quadrature weights for interior integration
  const std::vector<Real> &JxW = c.element_fe_var[0]->get_JxW();

//  const std::vector<std::vector<Real> >          &phi = c.element_fe_var[0]->get_phi();
  const std::vector<std::vector<RealGradient> > &dphi = c.element_fe_var[0]->get_dphi();

  // Workaround for weird FC6 bug
  optassert(c.dof_indices_var.size() > 0);

  // The number of local degrees of freedom in each variable
  const unsigned int n_u_dofs = c.dof_indices_var[0].size(); 

  // The subvectors and submatrices we need to fill:
  DenseSubMatrix<Number> &K = *c.elem_subjacobians[0][0];
  DenseSubVector<Number> &F = *c.elem_subresiduals[0];

  // Now we will build the element Jacobian and residual.
  // Constructing the residual requires the solution and its
  // gradient from the previous timestep.  This must be
  // calculated at each quadrature point by summing the
  // solution degree-of-freedom values by the appropriate
  // weight functions.
  unsigned int n_qpoints = c.element_qrule->n_points();

  for (unsigned int qp=0; qp != n_qpoints; qp++)
    {
      // Compute the solution gradient at the Newton iterate
      Gradient grad_T = c.interior_gradient(0, qp);

      for (unsigned int i=0; i != n_u_dofs; i++)
        F(i) += JxW[qp] * -parameters[0] * (grad_T * dphi[i][qp]);
      if (compute_jacobian)
        for (unsigned int i=0; i != n_u_dofs; i++)
          for (unsigned int j=0; j != n_u_dofs; ++j)
            K(i,j) += JxW[qp] * -parameters[0] * (dphi[i][qp] * dphi[j][qp]);
    } // end of the quadrature point qp-loop
  
  return compute_jacobian;
}

// Perturb and accumulate dual weighted residuals
void HeatSystem::perturb_accumulate_residuals(const ParameterVector& parameters)
{
  const unsigned int Np = parameters.size();
  
  for (unsigned int j=0; j != Np; ++j)
    {
      Number old_parameter = *parameters[j];
            
      *parameters[j] = old_parameter - dp;

      this->assembly(true, false);

      this->rhs->close();      
      
      AutoPtr<NumericVector<Number> > R_minus = this->rhs->clone();            

      // The contribution at a single time step would be [f(z;p+dp) - <partialu/partialt, z>(p+dp) - <g(u),z>(p+dp)] * dt
      // But since we compute the residual already scaled by dt, there is no need for the * dt 
      R_minus_dp += -R_minus->dot(this->get_adjoint_solution(0));
            
      *parameters[j] = old_parameter + dp;

      this->assembly(true, false);

      this->rhs->close();

      AutoPtr<NumericVector<Number> > R_plus = this->rhs->clone();            
            
      R_plus_dp += -R_plus->dot(this->get_adjoint_solution(0));
            
      *parameters[j] = old_parameter;

    }
}
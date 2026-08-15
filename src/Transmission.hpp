// --------------------------------------------------------------------------------------------
// TB Funtime Engine
// Copyright (c) 2026 James M. F. Morris
//
// This software is released under the GNU General Public License v3.0 (or any later version).
// --------------------------------------------------------------------------------------------

// AI Disclaimer: No AI was used to generate this code. All code choices and execution was performed
// by a human.

#pragma once
#include <Eigen/Dense>
#include <complex>

static std::complex<double> surface_greens_function(double E, double t) {

	double denom = 2.0 * t * t;
	double real_part = E / denom;
	double argument = (4.0 * t * t) - (E * E);
	if (argument < 0) argument = 0.0;
	double imag_part = -std::sqrt(argument) / denom;

	return std::complex<double>(real_part, imag_part);
}

Eigen::MatrixXcd self_energy(double energy, double coupling, double t, int idx, size_t system_size) {
	Eigen::MatrixXcd sigma = Eigen::MatrixXcd::Zero(system_size, system_size);
	std::complex<double> g_surf = surface_greens_function(energy, t);
	sigma((idx - 1), (idx - 1)) = coupling * coupling * g_surf;
	return sigma;
}

Eigen::MatrixXcd greens_function(const Eigen::MatrixXcd& H, double energy, const Eigen::MatrixXcd& sigma_L, const Eigen::MatrixXcd& sigma_R) {
	size_t N = H.rows();
	Eigen::MatrixXcd I = Eigen::MatrixXcd::Identity(N, N);
	//Eigen::MatrixXcd sigma_L = self_energy(energy, t_L, idx_L, N);
	//Eigen::MatrixXcd sigma_R = self_energy(energy, t_R, idx_R, N);
	Eigen::MatrixXcd G = (std::complex<double>(energy, 1e-10) * I - (H + sigma_L + sigma_R)).inverse();
	return G;
}

double transmission(double energy, const Eigen::MatrixXcd& H, double t_L, double t_R, int idx_L, int idx_R, double hopping) {
	size_t N = H.rows();
	Eigen::MatrixXcd sigma_L = self_energy(energy, t_L, hopping, idx_L, N);
	Eigen::MatrixXcd sigma_R = self_energy(energy, t_R, hopping, idx_R, N);
	Eigen::MatrixXcd G = greens_function(H, energy, sigma_L, sigma_R);
	std::complex<double> im_unit = std::complex<double>(0, 1);
	Eigen::MatrixXcd Gamma_L = im_unit * (sigma_L - sigma_L.adjoint());
	Eigen::MatrixXcd Gamma_R = im_unit * (sigma_R - sigma_R.adjoint());
	Eigen::MatrixXcd prod = Gamma_L * G * Gamma_R * G.adjoint();
	double T = prod.trace().real();
	return T;
}
// --------------------------------------------------------------------------------------------
// TB Funtime Engine
// Copyright (c) 2026 James M. F. Morris
//
// This software is released under the GNU General Public License v3.0 (or any later version).
// --------------------------------------------------------------------------------------------

// AI Disclaimer: Generative AI was used to assist with UI layout and style codeblocks, all underlying
// logic and architecture choices were made by a human and the combined example UI boilerplate and AI
// generated UI boilerplate code was tweaked and tested by a human for graphical/rendering issues.

#ifdef _WIN32
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") // Disable console window on Windows
#endif

#include <Eigen/Dense>
#include <fmt/core.h>
#include <fmt/format.h>
#include <filesystem>
#include <rapidcsv.h>
#include <fstream>
#include <sstream>
#include <fmt/ostream.h>
#include "Transmission.hpp"
#include <chrono>
#include <thread> 

// GUI includes
#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> 

// --- GLOBAL STATE ---
static char inPath[512] = ""; //"D:\\c++\\Repos\\TBFuntimeEngineGUI\\data\\hamiltonian.txt";
static char outPath[512] = ""; // "D:\\c++\\Repos\\TBFuntimeEngineGUI\\data\\transmission.txt";

// Drag and Drop State
static bool fileJustDropped = false;
static std::string droppedFilePath = "";

struct GraphData {
	std::vector<double> x;
	std::vector<double> y;
};
static GraphData current_plot;

// Batch Processing "Slideshow" State
struct BatchState {
	bool isRunning = false;
	std::vector<std::filesystem::path> queue;
	size_t currentIndex = 0;
	float delayTimer = 0.0f;
	float userDelay = 0.5f; // Default 0.5s wait between runs
	std::string currentFile = "Ready";
};
static BatchState batch;

// [Win32] VS2015+ linking (see imgui example_glfw_opengl3)
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

//**********************************************//
//        PURE LOGIC CODE BLOCK NO GUI          //
//**********************************************//

std::string MatFormat(const Eigen::MatrixXcd& mat) {
	static const Eigen::IOFormat NumpyFmt(Eigen::StreamPrecision, 0, ", ", "\n", "[", "]");
	std::stringstream ss;
	ss << mat.format(NumpyFmt);
	return ss.str();
}

Eigen::MatrixXcd data_to_matrix(const rapidcsv::Document& doc) {
	size_t rows = doc.GetRowCount();
	size_t cols = doc.GetColumnCount();
	Eigen::MatrixXcd mat(rows, cols);
	for (size_t i = 0; i < rows; ++i) {
		std::vector<double> row = doc.GetRow<double>(i);
		for (size_t j = 0; j < cols; ++j) {
			mat(i, j) = (j < row.size()) ? row[j] : 0.0;
		}
	}
	return mat;
}

namespace fs = std::filesystem;
Eigen::MatrixXcd H; // Hamiltonian of the system

void RunMySimulation(const std::string& inputPath, const std::string& outputPath) {
	auto start_time = std::chrono::high_resolution_clock::now();
	fs::path data_path = inputPath;
	fs::path out_path = outputPath;

	if (!fs::exists(data_path)) {
		fmt::print("Error: Input file does not exist at {}\n", inputPath);
		return;
	}

	try {
		rapidcsv::Document doc(data_path.string(),
			rapidcsv::LabelParams(-1, -1),
			rapidcsv::SeparatorParams('\t'),
			rapidcsv::ConverterParams(true, 0.0, 0));

		double t_L = doc.GetCell<double>(0, 0);
		double t_R = doc.GetCell<double>(1, 0);
		int idx_L = doc.GetCell<int>(0, 1);
		int idx_R = doc.GetCell<int>(1, 1);
		double t_lead = -1.0; // hardcoded for now, need to change this

		doc.RemoveRow(0);
		doc.RemoveRow(0);
		H = data_to_matrix(doc);

		std::ofstream outfile(out_path);
		if (!outfile.is_open()) {
			fmt::print("Error: Could not open output file for writing: {}\n", out_path.string());
			return;
		}

		current_plot.x.clear();
		current_plot.y.clear();

		double E_range = 2 * abs(t_lead);
		double E_step = E_range / 10000;

		for (double E = -E_range; E <= E_range; E += E_step) {
			double T = transmission(E, H, t_L, t_R, idx_L, idx_R, t_lead);
			outfile << fmt::format("{:.4f}\t{:.6f}\n", E, T);

			current_plot.x.push_back(E);
			current_plot.y.push_back(T);
		}
		outfile.close();
	}
	catch (const std::exception& e) {
		fmt::print("Error: {}\n", e.what());
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	fmt::print("Execution Time: {} ms\n", duration);
}

//**********************************************//
//          PURE LOGIC CODE BLOCK END           //
//**********************************************//

// Updated to support the visual "slideshow" delay

auto lastRunTime = std::chrono::steady_clock::now();

void UpdateBatchLogic() {
	if (!batch.isRunning) return;

	// Check if we are done
	if (batch.currentIndex >= batch.queue.size()) {
		batch.isRunning = false;
		batch.currentFile = "Batch Complete!";
		return;
	}

	// Wait Timer
	//auto currentTime = std::chrono::steady_clock::now();
	//std::chrono::duration<float> dT = currentTime - lastRunTime;
	batch.delayTimer += ImGui::GetIO().DeltaTime; // Add time passed since last frame
	if (batch.delayTimer < batch.userDelay) {
		return;
	}
	//if (dT.count() < batch.userDelay) {
	//	return;
	//}

	// Run simulation for current file
	batch.delayTimer = 0.0f; // Reset timer
	auto& entry = batch.queue[batch.currentIndex];

	std::string fileName = entry.filename().string();
	batch.currentFile = "Processing: " + fileName + " (" + std::to_string(batch.currentIndex + 1) + "/" + std::to_string(batch.queue.size()) + ")";

	// Output path
	std::string outDir = outPath; // Use the user-defined output folder
	if (!std::filesystem::exists(outDir)) std::filesystem::create_directories(outDir);
	std::string outFilePath = (std::filesystem::path(outDir) / ("trans_" + fileName)).string();

	// RUN PHYSICS
	RunMySimulation(entry.string(), outFilePath);

	// Next file
	batch.currentIndex++;
}

// Theme (SetupSleekTheme()) initial generation was AI assisted and tweaked after the fact
void SetupSleekTheme() {
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// --- Geometry ---
	style.WindowRounding = 5.0f;
	style.FrameRounding = 5.0f;
	style.ChildRounding = 0.0f;
	style.GrabRounding = 5.0f;
	style.WindowBorderSize = 0.0f;
	style.FrameBorderSize = 2.0f;
	style.WindowPadding = ImVec2(20, 20);
	style.ItemSpacing = ImVec2(10, 15);

	// --- Glassy Colour Palette ---
	// Background (Translucent Navy)
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.5f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.14f, 0.00f); // Transparent children
	colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Borders & Separators
	colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.30f, 0.50f);

	// Widgets (Inputs)
	colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.15f, 0.19f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.19f, 0.25f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.23f, 0.30f, 1.00f);

	// Buttons (Vibrant Blue)
	colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.50f, 0.85f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.35f, 0.65f, 1.00f);

	// Plotting Area (Matching the dark theme)
	colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);
}

// Main code
int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) return 1;

	// 1. Setup Window 
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	// Note: Updated title, but kept your settings
	GLFWwindow* window = glfwCreateWindow(1600, 900, "TB Funtime Engine", nullptr, nullptr);
	//glfwMaximizeWindow(window);		// For maximized window at start
	if (window == nullptr) return 1;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync
	glfwSetWindowSizeLimits(window, 1000, 900, GLFW_DONT_CARE, GLFW_DONT_CARE); // Minimum size

	// We store the path here. The UI (in the loop) decides where to put it.
	glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char** paths) {
		if (count > 0) {
			droppedFilePath = paths[0];
			fileJustDropped = true;
		}
		});

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;

	ImFont* mainFont = nullptr;
	ImFont* boldFont = nullptr;
	ImFont* titleFont = nullptr;

	// Using Arial with fallback
	if (std::filesystem::exists("C:\\Windows\\Fonts\\arial.ttf")) {
		mainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f);
		ImFont* titleFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arialbd.ttf", 28.0f);
	}
	else {
		mainFont = io.Fonts->AddFontDefault();
	}

	if (std::filesystem::exists("C:\\Windows\\Fonts\\arial.ttf")) {						//seguisb.ttf
		boldFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f);
	}
	else {
		boldFont = mainFont; // Fallback to normal if bold missing
	}

	SetupSleekTheme(); // YOUR theme function
	ImPlot::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// [Variables for Graph Limits]
	static float xMin = -2.0f, xMax = 2.0f;
	// Added default Y range 
	static float yMin = 1e-9f, yMax = 2.0f;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Run the "slideshow" batch logic every frame
		UpdateBatchLogic();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// [Layout Setup]
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::Begin("MasterCanvas", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		// --- SIDEBAR ---
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.09f, 1.00f));
		ImGui::BeginChild("Sidebar", ImVec2(550, 0), true, ImGuiWindowFlags_NoScrollbar);

		ImGui::Dummy(ImVec2(0, 10));
		ImGui::Indent(15);

		if (titleFont) ImGui::PushFont(titleFont);
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "TB FUNTIME ENGINE"); // Cyan/Blue Brand Color
		if (titleFont) ImGui::PopFont();

		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "v1.0  |  James M F Morris | The University of Liverpool");

		ImGui::Unindent(15);
		ImGui::Dummy(ImVec2(0, 15)); // Space before the line
		ImGui::Separator();          // The line dividing Header from Settings
		ImGui::Dummy(ImVec2(0, 15));
		ImGui::Indent(15);

		// Header
		if (boldFont) ImGui::PushFont(boldFont);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "SYSTEM CONFIGURATION");
		if (boldFont) ImGui::PopFont();

		ImGui::Unindent(15); ImGui::Separator(); ImGui::Indent(15);
		ImGui::Dummy(ImVec2(0, 15));

		// --- INPUT BOX (Target 1) ---
		ImGui::Text("Input Source");
		ImGui::PushItemWidth(470);
		ImGui::InputText("##in", inPath, 512);

		if (ImGui::IsItemHovered() && strlen(inPath) > 0) {
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(inPath);
			ImGui::EndTooltip();
		}

		// Logic: If file dropped AND mouse is over this box -> Paste here
		if (fileJustDropped && ImGui::IsItemHovered()) {
			strncpy(inPath, droppedFilePath.c_str(), 512);
			fileJustDropped = false; // Mark as handled
		}
		ImGui::PopItemWidth();

		ImGui::Dummy(ImVec2(0, 10));

		// --- OUTPUT BOX (Target 2) ---
		ImGui::Text("Output Destination");
		ImGui::PushItemWidth(470);
		ImGui::InputText("##out", outPath, 512);

		if (ImGui::IsItemHovered() && strlen(inPath) > 0) {
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(inPath);
			ImGui::EndTooltip();
		}

		// Logic: If file dropped AND mouse is over THIS box -> Paste here
		if (fileJustDropped && ImGui::IsItemHovered()) {
			strncpy(outPath, droppedFilePath.c_str(), 512);
			fileJustDropped = false; // Mark as handled
		}
		ImGui::PopItemWidth();

		ImGui::Dummy(ImVec2(0, 20));

		// --- AXIS CONTROLS ---
		ImGui::Text("Graph Limits");
		ImGui::PushItemWidth(400);
		ImGui::DragFloatRange2("X-Axis (eV)", &xMin, &xMax, 0.01f, -10.0f, 10.0f);
		// NEW: Y-Axis Control
		ImGui::DragFloatRange2("Y-Axis (T)", &yMin, &yMax, 0.1f, 1e-12f, 100.0f, "%.1e");
		ImGui::PopItemWidth();

		ImGui::Unindent(15);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Indent(15); // Re-indent for Batch section

		// --- BATCH CONTROLS ---
		ImGui::Dummy(ImVec2(0, 10));
		if (boldFont) ImGui::PushFont(boldFont);
		ImGui::Text("BATCH CONTROLS");
		if (boldFont) ImGui::PopFont();

		ImGui::Text("Delay Between Runs (sec)");
		ImGui::PushItemWidth(470);
		ImGui::SliderFloat("##delay", &batch.userDelay, 0.0f, 2.0f);
		ImGui::PopItemWidth();

		ImGui::Dummy(ImVec2(0, 10));

		// Status Text (Yellow)
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", batch.currentFile.c_str());

		// Footer Button
		float footerH = 60.0f;
		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - footerH);

		// Toggle Button Text based on state
		const char* btnText = batch.isRunning ? "STOP BATCH" : "CALCULATE";

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));          // Base
		//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.4f, 1.0f));    // Brighter on hover
		//ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.2f, 1.0f));		// Darker when clicked

		if (ImGui::Button(btnText, ImVec2(470, 50))) {
			if (batch.isRunning) {
				batch.isRunning = false; // User clicked Stop
				batch.currentFile = "Stopped by user.";
			}
			else {
				// User clicked Execute
				if (std::filesystem::is_directory(inPath)) {
					// --- PREPARE BATCH QUEUE ---
					batch.queue.clear();
					for (const auto& entry : std::filesystem::directory_iterator(inPath)) {
						if (entry.path().extension() == ".txt") {
							batch.queue.push_back(entry.path());
						}
					}
					if (!batch.queue.empty()) {
						batch.currentIndex = 0;
						batch.isRunning = true; // This starts UpdateBatchLogic()
					}
					else {
						batch.currentFile = "No .txt files found in folder!";
					}
				}
				else {
					// Single file run
					RunMySimulation(inPath, outPath);
					batch.currentFile = "Single Run Complete.";
				}
			}
		}
		ImGui::PopStyleColor();

		ImGui::Unindent(15);
		ImGui::EndChild();
		ImGui::PopStyleColor();

		ImGui::SameLine();

		// --- VISUALISER ---
		ImGui::BeginGroup();
		ImGui::Dummy(ImVec2(0, 5));
		if (boldFont) ImGui::PushFont(boldFont);
		ImGui::Text("  Transmission Function Viewer");
		if (boldFont) ImGui::PopFont();
		ImGui::Separator();

		// ImPlotAxisFlags_NoHighlight removes the glowing hover effect
		ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

		if (ImPlot::BeginPlot("##Spectrum", ImVec2(-1, 500), ImPlotFlags_NoTitle)) {
			ImPlot::SetupAxis(ImAxis_X1, "Energy (eV)", ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoGridLines);
			ImPlot::SetupAxisLimits(ImAxis_X1, xMin, xMax, ImGuiCond_Always);

			ImPlot::SetupAxis(ImAxis_Y1, "Transmission (T)", ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoGridLines);
			ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
			ImPlot::SetupAxisLimits(ImAxis_Y1, yMin, yMax, ImGuiCond_Always);

			if (!current_plot.x.empty()) {
				ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 4.0f);
				ImPlot::PlotLine("T(E)", current_plot.x.data(), current_plot.y.data(), (int)current_plot.x.size());
				ImPlot::PopStyleVar();
				ImPlot::PopStyleColor();
			}
			ImPlot::EndPlot();
		}
		ImPlot::PopStyleColor(2);
		ImGui::EndGroup();

		ImGui::End(); // End MasterCanvas

		// Cleanup drop flag if it wasn't consumed this frame
		if (fileJustDropped) fileJustDropped = false;

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		// Kept your preferred background colour
		glClearColor(0.03f, 0.03f, 0.04f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
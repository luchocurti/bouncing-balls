/**
 * @file main.cpp
 * @brief Application entry point and SDL event loop.
 *
 * Handles command-line configuration, SDL initialization, user input,
 * simulation timing, rendering, and resource cleanup for the bouncing-ball
 * application.
 */

#include <SDL2/SDL.h>

#include <cstdint>
#include <iostream>
#include <random>
#include <string>

#include "renderer.hpp"
#include "simulation.hpp"

namespace {

// -----------------------------------------------------------------------------
// Window / Simulation constants
// -----------------------------------------------------------------------------

/** Default window width in pixels. */
constexpr int DEFAULT_WINDOW_WIDTH =
    static_cast<int>(simulation::WORLD_RADIUS * 2 + 40);

/** Minimum window width in pixels. */
constexpr int MIN_WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH - 40;

/** Maximum window width in pixels. */
constexpr int MAX_WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH * 2;

/** Default window height in pixels. */
constexpr int DEFAULT_WINDOW_HEIGHT =
    static_cast<int>(simulation::WORLD_RADIUS * 2 + 40);

/** Minimum window height in pixels. */
constexpr int MIN_WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT - 40;
/** Maximum window height in pixels. */

constexpr int MAX_WINDOW_HEIGHT =
    static_cast<int>(DEFAULT_WINDOW_HEIGHT * 1.25);

/**
 * Command-line and runtime configuration for the application.
 */
struct Options {
  int width = DEFAULT_WINDOW_WIDTH;
  int height = DEFAULT_WINDOW_HEIGHT;
  double ballRadius = simulation::DEFAULT_BALL_RADIUS;
  std::uint32_t seed = std::random_device{}();
};

// -----------------------------------------------------------------------------
// Command-line parsing and user-facing text
// -----------------------------------------------------------------------------

/**
 * Parses a string as a complete floating-point value.
 *
 * @param text Input text to convert.
 * @param value Output parameter receiving the parsed value.
 * @return true when the entire string is a valid floating-point number;
 *         otherwise false.
 */
bool parseDouble(const std::string &text, double &value) {
  try {
    std::size_t consumed = 0;
    value = std::stod(text, &consumed);

    // Reject inputs such as "12px": stod() accepts the numeric prefix, while
    // command-line options require the entire argument to be numeric.
    return consumed == text.size();
  } catch (...) {
    return false;
  }
}

/**
 * Parses a string as a complete integer value.
 *
 * @param text Input text to convert.
 * @param value Output parameter receiving the parsed value.
 * @return true when the entire string is a valid integer; otherwise false.
 */
bool parseInt(const std::string &text, int &value) {
  try {
    std::size_t consumed = 0;
    value = std::stoi(text, &consumed);

    // As with parseDouble(), require complete consumption so malformed suffixes
    // do not silently pass validation.
    return consumed == text.size();
  } catch (...) {
    return false;
  }
}

/**
 * Prints the keyboard controls supported by the interactive application.
 */
void printControls() {
  std::cout << "\nControls:\n"
            << "  R             Restart\n"
            << "  Esc / Q       Quit\n"
            << "  Space         Pause / Resume\n";
}

/**
 * Prints command-line usage and the available options.
 *
 * @param program Program name used in the usage line.
 */
void printUsage(const char *program) {
  std::cout << "\nUsage:          " << program << " [options]\n"
            << "\n"
            << "Options:\n"
            << "  --radius R    Ball radius in pixels\n"
            << "                Default: " << simulation::DEFAULT_BALL_RADIUS
            << "\n"
            << "                Range: (" << simulation::MIN_BALL_RADIUS << ", "
            << simulation::MAX_BALL_RADIUS << ")\n"
            << "  --width W     Window width in pixels\n"
            << "                Default: " << DEFAULT_WINDOW_WIDTH << "\n"
            << "                Range: [" << MIN_WINDOW_WIDTH << ", "
            << MAX_WINDOW_WIDTH << "]\n"
            << "  --height H    Window height in pixels\n"
            << "                Default: " << DEFAULT_WINDOW_HEIGHT << "\n"
            << "                Range: [" << MIN_WINDOW_HEIGHT << ", "
            << MAX_WINDOW_HEIGHT << "]\n"
            << "  --seed N      Random seed for reproducible runs\n"
            << "  --help        Show this help\n";

  printControls();
}

/**
 * Parses the application's command-line arguments into an Options structure.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @param options Output configuration populated with parsed values.
 * @return true when parsing succeeds. false for --help or any invalid option.
 */
bool parseArguments(int argc, char **argv, Options &options) {
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];

    if (argument == "--help") {
      printUsage(argv[0]);

      // Treat --help as a normal request to print information; main() can then
      // terminate before initializing SDL.
      return false;
    }

    if ((argument == "--radius" || argument == "--width" ||
         argument == "--height" || argument == "--seed") &&
        i + 1 >= argc) {
      std::cerr << "Missing value for " << argument << '\n';
      return false;
    }

    if (argument == "--radius") {
      // Pre-increment consumes the value belonging to this option in the same
      // pass through the argument loop.
      if (!parseDouble(argv[++i], options.ballRadius) ||
          options.ballRadius <= simulation::MIN_BALL_RADIUS ||
          options.ballRadius >= simulation::MAX_BALL_RADIUS) {
        std::cerr << "Invalid radius.\n";
        return false;
      }
    } else if (argument == "--width") {
      if (!parseInt(argv[++i], options.width) ||
          options.width < MIN_WINDOW_WIDTH ||
          options.width > MAX_WINDOW_WIDTH) {
        std::cerr << "Invalid width.\n";
        return false;
      }
    } else if (argument == "--height") {
      if (!parseInt(argv[++i], options.height) ||
          options.height < MIN_WINDOW_HEIGHT ||
          options.height > MAX_WINDOW_HEIGHT) {
        std::cerr << "Invalid height.\n";
        return false;
      }
    } else if (argument == "--seed") {
      int parsedSeed = 0;
      if (!parseInt(argv[++i], parsedSeed)) {
        std::cerr << "Invalid seed.\n";
        return false;
      }

      // The simulation API uses an unsigned 32-bit seed, so convert the
      // parsed command-line integer to that representation before storing it.
      options.seed = static_cast<std::uint32_t>(parsedSeed);
    } else {
      std::cerr << "Unknown option: " << argument << '\n';
      printUsage(argv[0]);
      return false;
    }
  }

  return true;
}

} // namespace

// -----------------------------------------------------------------------------
// Application entry point
// -----------------------------------------------------------------------------

/**
 * Initializes SDL, runs the simulation/render loop, and releases SDL resources.
 *
 * The main loop is responsible for event handling, frame timing, simulation
 * updates, rendering, and displaying lightweight runtime status in the window
 * title.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 for normal termination or command-line handling; 1 when SDL
 *         initialization or renderer/window creation fails.
 */
int main(int argc, char **argv) {
  Options options;

  // Parse configuration before creating any SDL resources so invalid input
  // fails fast and does not leave a partially initialized application.
  if (!parseArguments(argc, argv, options)) {
    return 0;
  }

  // SDL must be initialized before creating a window or renderer.
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
    return 1;
  }

  // Create the application window using the requested dimensions.
  SDL_Window *window = SDL_CreateWindow(
      "Bouncing Balls - Collision Spawning", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, options.width, options.height, SDL_WINDOW_SHOWN);

  if (window == nullptr) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
    SDL_Quit();
    return 1;
  }

  // Prefer an accelerated, vsync-backed renderer for smooth presentation.
  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  // Some systems may not expose an accelerated renderer. Fall back to
  // software rendering rather than failing completely.
  if (renderer == nullptr) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }

  if (renderer == nullptr) {
    std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // The simulation owns the physics state; the renderer only reads from it.
  simulation::Simulation simulation(options.ballRadius, options.seed);

  bool running = true;
  bool paused = false;

  // SDL's high-resolution performance counter provides frame timing without
  // relying on wall-clock APIs with lower resolution.
  Uint64 previousCounter = SDL_GetPerformanceCounter();
  const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

  printControls();

  // ---------------------------------------------------------------------------
  // Main event / simulation / rendering loop
  // ---------------------------------------------------------------------------

  std::cout << "\n[ RUNNING  ]\n";
  while (running) {
    SDL_Event event;

    // Process every pending event before advancing the simulation so input
    // remains responsive even when several events arrive in one frame.
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        // Ignore key-repeat events so holding a key does not trigger the same
        // action multiple times.
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
        case SDLK_q:
          running = false;
          break;

        case SDLK_SPACE:
          paused = !paused;
          if (paused) {
            std::cout << "[ PAUSING  ]\n";
          } else {
            std::cout << "[ RESUMING ]\n";
          }
          break;

        case SDLK_r:
          // Recreate the simulation with the original options so restarting
          // also resets its random sequence.
          simulation = simulation::Simulation(options.ballRadius, options.seed);
          paused = false;
          std::cout << "[RESTARTING]\n";
          break;

        default:
          break;
        }
      }
    }

    // Convert the performance-counter delta into seconds for the simulation.
    const Uint64 currentCounter = SDL_GetPerformanceCounter();
    const double dt =
        static_cast<double>(currentCounter - previousCounter) / frequency;
    previousCounter = currentCounter;

    if (!paused) {
      simulation.update(dt);
    }

    // Draw the complete frame only after the simulation state has been updated.
    renderer::drawBackground(renderer);
    renderer::renderSimulation(renderer, simulation, options.width,
                               options.height);

    SDL_RenderPresent(renderer);

    // Change the title to expose useful runtime state without requiring
    // any extra GUI/text rendering library.
    std::string title =
        "Bouncing Balls | Balls: " + std::to_string(simulation.ballCount());

    if (paused) {
      title += " | PAUSED";
    } else if (simulation.finished()) {
      title += " | NO SPACE - FINISHED";
    }

    SDL_SetWindowTitle(window, title.c_str());
  }

  // ---------------------------------------------------------------------------
  // Cleanup
  // ---------------------------------------------------------------------------
  //
  // Destroy SDL resources in the reverse order of their creation so dependent
  // objects are released before SDL itself is shut down.

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  std::cout << "[ EXITING  ]\n";

  return 0;
}

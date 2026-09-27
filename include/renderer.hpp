/**
 * @file renderer.hpp
 * @brief Public interface for rendering the simulation.
 *
 * Declares the functions responsible for drawing the simulation background
 * and visualizing the current state of the bouncing-ball simulation using SDL.
 */

#pragma once

#include <SDL2/SDL.h>

#include "simulation.hpp"

namespace renderer {

// -----------------------------------------------------------------------------
// Rendering constants
// -----------------------------------------------------------------------------

namespace Color {
constexpr SDL_Color BLACK = {0, 0, 0, 255};
constexpr SDL_Color WHITE = {255, 255, 255, 255};
constexpr SDL_Color RED = {255, 0, 0, 255};
constexpr SDL_Color GREEN = {0, 255, 0, 255};
constexpr SDL_Color BLUE = {0, 0, 255, 255};
} // namespace Color

/**
 * Clears the SDL renderer with the background color used by the application.
 *
 * @param renderer SDL renderer that receives the clear operation. Must be
 *                 initialized before this function is called.
 */
void drawBackground(SDL_Renderer *renderer);

/**
 * Draws the complete simulation state into the current SDL render target.
 *
 * The simulation's world is centered in the supplied viewport. The world
 * boundary is drawn first, followed by every ball using its current position,
 * radius, and color.
 *
 * @param renderer SDL renderer used for drawing. Must be initialized and
 *                 associated with the application window.
 * @param simulation Read-only simulation state to visualize.
 * @param width Width of the rendering viewport in pixels.
 * @param height Height of the rendering viewport in pixels.
 */
void renderSimulation(SDL_Renderer *renderer,
                      const simulation::Simulation &simulation, int width,
                      int height);

} // namespace renderer

/**
 * @file renderer.cpp
 * @brief SDL-based implementation of the simulation renderer.
 *
 * Implements the rendering operations declared in renderer.hpp, including
 * drawing the world boundary, filled balls, and the application background.
 * Circle rasterization is handled internally using lightweight drawing
 * algorithms tailored to the SDL renderer.
 */

#include "renderer.hpp"

#include <cmath>
#include <cstdint>

namespace renderer {

namespace {

// -----------------------------------------------------------------------------
// Primitive drawing helpers
// -----------------------------------------------------------------------------
//
// These helpers are intentionally kept private to this translation unit.
// They implement the rasterization details needed by the public renderer API
// without exposing them as part of the renderer interface.

void drawFilledCircle(SDL_Renderer *renderer, int cx, int cy, int radius) {
  // A non-positive radius cannot produce a visible circle.
  if (radius <= 0) {
    return;
  }

  // For each horizontal offset, compute the half-width of the circle's
  // intersection with that scanline and draw the complete span at once.
  for (int dy = -radius; dy <= radius; ++dy) {
    const int dx = static_cast<int>(
        std::floor(std::sqrt(static_cast<double>(radius * radius - dy * dy))));

    SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
  }
}

/**
 * Draws the eight points produced by exploiting the circle's symmetry.
 *
 * @param renderer SDL renderer that receives the points.
 * @param cx X coordinate of the circle center.
 * @param cy Y coordinate of the circle center.
 * @param x Horizontal offset from the center for the current octant.
 * @param y Vertical offset from the center for the current octant.
 *
 * @return Nothing. The function emits eight symmetric pixels.
 */
void drawEightPoints(SDL_Renderer *renderer, int cx, int cy, int x, int y) {
  SDL_RenderDrawPoint(renderer, cx + x, cy + y);
  SDL_RenderDrawPoint(renderer, cx + y, cy + x);
  SDL_RenderDrawPoint(renderer, cx - y, cy + x);
  SDL_RenderDrawPoint(renderer, cx - x, cy + y);
  SDL_RenderDrawPoint(renderer, cx - x, cy - y);
  SDL_RenderDrawPoint(renderer, cx - y, cy - x);
  SDL_RenderDrawPoint(renderer, cx + y, cy - x);
  SDL_RenderDrawPoint(renderer, cx + x, cy - y);
}

/**
 * Draws the outline of a circle using the midpoint circle algorithm.
 *
 * @param renderer SDL renderer that receives the circle outline.
 * @param cx X coordinate of the circle center.
 * @param cy Y coordinate of the circle center.
 * @param radius Circle radius in pixels.
 *
 * @return Nothing. The outline is drawn directly into the SDL renderer.
 */
void drawCircleOutline(SDL_Renderer *renderer, int cx, int cy, int radius) {
  // The midpoint algorithm calculates one octant and mirrors it using
  // circle symmetry, avoiding a floating-point angle sweep.
  int x = radius;
  int y = 0;
  int decisionOver2 = 1 - x;

  while (x >= y) {
    drawEightPoints(renderer, cx, cy, x, y);
    ++y;

    if (decisionOver2 <= 0) {
      decisionOver2 += 2 * y + 1;
    } else {
      --x;
      decisionOver2 += 2 * (y - x) + 1;
    }
  }
}

} // namespace

// -----------------------------------------------------------------------------
// Public rendering API
// -----------------------------------------------------------------------------

/**
 * Clears the current frame with the application's light gray background.
 *
 * @param renderer SDL renderer whose color buffer is cleared.
 */
void drawBackground(SDL_Renderer *renderer) {
  SDL_SetRenderDrawColor(renderer, Color::BLACK.r, Color::BLACK.g,
                         Color::BLACK.b, Color::BLACK.a);
  SDL_RenderClear(renderer);
}

/**
 * Renders the circular world boundary and all balls in the simulation.
 *
 * Simulation coordinates are expressed relative to the center of the world,
 * while SDL uses the top-left corner as the viewport origin. The renderer
 * therefore translates each world-space position by the viewport center.
 *
 * @param renderer SDL renderer used for the current frame.
 * @param simulation Read-only simulation state to draw.
 * @param width Width of the viewport in pixels.
 * @param height Height of the viewport in pixels.
 */
void renderSimulation(SDL_Renderer *renderer,
                      const simulation::Simulation &simulation, int width,
                      int height) {
  // Center the simulation's world inside the SDL window.
  const double cx = static_cast<double>(width) * 0.5;
  const double cy = static_cast<double>(height) * 0.5;

  // Draw the fixed world boundary before the balls so the balls remain
  // visually on top of the circle outline.
  SDL_SetRenderDrawColor(renderer, Color::WHITE.r, Color::WHITE.g,
                         Color::WHITE.b, Color::WHITE.a);
  drawCircleOutline(renderer, static_cast<int>(std::lround(cx)),
                    static_cast<int>(std::lround(cy)),
                    static_cast<int>(std::lround(simulation::WORLD_RADIUS)));

  // Convert each ball from world coordinates to window coordinates.
  for (const simulation::Ball &ball : simulation.balls()) {
    SDL_SetRenderDrawColor(renderer, ball.color.r, ball.color.g, ball.color.b,
                           255);

    drawFilledCircle(renderer,
                     static_cast<int>(std::lround(cx + ball.position.x)),
                     static_cast<int>(std::lround(cy + ball.position.y)),
                     static_cast<int>(std::lround(ball.radius)));
  }
}

} // namespace renderer

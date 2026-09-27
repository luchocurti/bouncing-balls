/**
 * @file simulation.hpp
 * @brief Public interface for the bouncing-ball simulation.
 *
 * Defines the data types, configuration constants, and Simulation class used
 * to represent and advance the physics state of the application, including
 * ball movement, boundary handling, collision detection, and ball spawning.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace simulation {

// -----------------------------------------------------------------------------
// World and simulation constants
// -----------------------------------------------------------------------------

constexpr double PI = 3.14159265358979323846;

/** Radius of the circular simulation world in world-space units. */
constexpr double WORLD_RADIUS = 380.0;

/** Default radius assigned to each newly-created ball. */
constexpr double DEFAULT_BALL_RADIUS = 12.0;

/** Minimum radius assigned to each newly-created ball. */
constexpr double MIN_BALL_RADIUS = 0.0;

/** Maximum radius assigned to each newly-created ball. */
constexpr double MAX_BALL_RADIUS = WORLD_RADIUS / 4.0;

/** Minimum initial speed assigned to a ball. */
constexpr double MIN_SPEED = 180.0;

/** Maximum initial speed assigned to a ball. */
constexpr double MAX_SPEED = 360.0;

/** Number of balls created when a simulation is reset. */
constexpr int INITIAL_BALLS = 2;

// When a collision happens, we try this many random locations before
// falling back to a deterministic scan.
constexpr int RANDOM_SPAWN_ATTEMPTS = 2500;

// Numerical tolerances used to avoid unstable calculations around zero.
constexpr double EPSILON = 1e-9;

// -----------------------------------------------------------------------------
// Basic value types
// -----------------------------------------------------------------------------

/**
 * Represents a two-dimensional vector in simulation/world coordinates.
 *
 * The operators provide the small set of vector operations required by the
 * simulation's movement and collision calculations.
 */
struct Vec2 {
  double x{};
  double y{};

  /** @return The component-wise sum of this vector and @p other. */
  Vec2 operator+(const Vec2 &other) const { return {x + other.x, y + other.y}; }

  /** @return The component-wise difference between this vector and @p other. */
  Vec2 operator-(const Vec2 &other) const { return {x - other.x, y - other.y}; }

  /**
   * Scales both components by @p scalar.
   *
   * @param scalar Multiplication factor applied to both components.
   * @return A new scaled vector.
   */
  Vec2 operator*(double scalar) const { return {x * scalar, y * scalar}; }

  /**
   * Adds @p other to this vector in place.
   *
   * @param other Vector whose components are added to this vector.
   * @return Reference to this vector after modification.
   */
  Vec2 &operator+=(const Vec2 &other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  /**
   * Subtracts @p other from this vector in place.
   *
   * @param other Vector whose components are subtracted from this vector.
   * @return Reference to this vector after modification.
   */
  Vec2 &operator-=(const Vec2 &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }
};

/**
 * Represents an RGB color stored in the format expected by SDL.
 */
struct Color {
  std::uint8_t r{};
  std::uint8_t g{};
  std::uint8_t b{};
};

/**
 * Stores the complete state of one simulated ball.
 *
 * Positions are expressed relative to the center of the circular world.
 * Velocity is expressed in world units per second.
 */
struct Ball {
  Vec2 position{};
  Vec2 velocity{};
  double radius{DEFAULT_BALL_RADIUS};
  Color color{};
};

// -----------------------------------------------------------------------------
// Simulation
// -----------------------------------------------------------------------------

/**
 * Owns and advances the state of the bouncing-ball simulation.
 *
 * The simulation is deterministic for a given random seed. Balls move with
 * constant velocity between collisions, remain inside the circular world, and
 * spawn a new ball when they collide while approaching one another.
 */
class Simulation {
public:
  /**
   * Creates a simulation configured with a fixed ball radius and random seed.
   *
   * @param ballRadius Radius assigned to balls created by this simulation.
   *                   It must be positive and smaller than WORLD_RADIUS.
   * @param seed Seed used to initialize the simulation's pseudo-random
   *             generator. Reusing the same seed reproduces the same random
   *             sequence.
   */
  Simulation(double ballRadius, std::uint32_t seed);

  /**
   * Restores the simulation to its initial state.
   *
   * Existing balls are discarded and INITIAL_BALLS new balls are placed at
   * non-overlapping positions whenever sufficient free space exists.
   */
  void reset();

  /**
   * Advances the simulation by the requested wall-clock time.
   *
   * Internally, the simulation consumes the elapsed time using a fixed
   * sub-step so the physics remains less sensitive to rendering frame rate.
   *
   * @param dt Elapsed real time in seconds since the previous update call.
   */
  void update(double dt);

  /**
   * Returns the current number of balls in the simulation.
   *
   * @return Number of active balls.
   */
  std::size_t ballCount() const;

  /**
   * Reports whether the simulation can no longer spawn additional balls.
   *
   * @return true after no valid spawn location could be found; otherwise false.
   */
  bool finished() const;

  /**
   * Provides read-only access to all current balls.
   *
   * @return Constant reference to the simulation's internal ball container.
   *         The reference remains valid until the simulation changes its
   *         container, for example by resetting or spawning balls.
   */
  const std::vector<Ball> &balls() const;

private:
  // Configuration and mutable simulation state.
  double ballRadius_;
  std::mt19937 rng_;
  std::uniform_real_distribution<double> unit_;
  std::vector<Ball> balls_;
  bool finished_{false};
  double accumulator_{0.0};
  double nextHue_{0.0};

  /**
   * Generates a random floating-point value in the inclusive range
   * approximately bounded by @p min and @p max.
   *
   * @param min Lower bound of the requested range.
   * @param max Upper bound of the requested range.
   * @return A pseudo-random value produced from the simulation's RNG.
   */
  double randomRange(double min, double max);

  /**
   * Generates a random velocity with a speed in the configured range.
   *
   * @return A two-dimensional velocity whose direction is uniformly sampled
   *         by angle and whose magnitude lies between MIN_SPEED and MAX_SPEED.
   */
  Vec2 randomVelocity();

  /**
   * Creates a ball at the requested position and assigns its initial state.
   *
   * @param position World-space center position for the new ball.
   * @return A newly constructed ball with a random velocity and generated
   *         display color.
   */
  Ball makeBall(const Vec2 &position);

  /**
   * Tests whether a ball centered at @p position fits entirely inside the
   * circular world.
   *
   * @param position World-space center position to test.
   * @return true when the ball would remain inside the world boundary.
   */
  bool isInsideWorld(const Vec2 &position) const;

  /**
   * Tests whether a ball at @p position would overlap an existing ball.
   *
   * @param position World-space center position to test.
   * @return true when the candidate position overlaps another ball.
   */
  bool overlapsAnyBall(const Vec2 &position) const;

  /**
   * Combines the world-boundary and inter-ball occupancy checks.
   *
   * @param position World-space center position to test.
   * @return true when a ball can be placed safely at @p position.
   */
  bool positionIsFree(const Vec2 &position) const;

  /**
   * Finds a free position suitable for spawning a new ball.
   *
   * Random sampling is attempted first for speed. If that does not find a
   * valid position, a deterministic spiral scan is used as a fallback.
   *
   * @param result Output parameter receiving the selected world-space position.
   * @return true if a free position was found; false if the world appears full.
   */
  bool findSpawnPosition(Vec2 &result);

  /**
   * Advances the simulation by one fixed physics step.
   *
   * @param dt Fixed simulation timestep in seconds.
   */
  void step(double dt);

  /**
   * Keeps a ball inside the circular world and reflects outward motion.
   *
   * @param ball Ball whose position and velocity may be corrected in place.
   */
  void keepInsideCircle(Ball &ball);

  /**
   * Resolves an overlap between two balls and applies their collision response.
   *
   * Positional overlap is corrected first. A collision is considered a
   * spawning collision only when the balls were approaching one another.
   *
   * @param a First ball participating in the potential collision.
   * @param b Second ball participating in the potential collision.
   * @return true when an approaching collision was resolved and should spawn
   *         a new ball; false otherwise.
   */
  bool resolveCollision(Ball &a, Ball &b);
};

} // namespace simulation

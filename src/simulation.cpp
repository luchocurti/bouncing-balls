/**
 * @file simulation.cpp
 * @brief Implementation of the bouncing-ball simulation.
 *
 * Implements the Simulation class and its supporting mathematical and color
 * conversion helpers. The simulation uses fixed-size physics steps, handles
 * collisions and boundary constraints, and generates new balls when collisions
 * occur and sufficient space remains.
 */

#include "simulation.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace simulation {

namespace {

// -----------------------------------------------------------------------------
// Vector and color helpers
// -----------------------------------------------------------------------------

/**
 * Computes the dot product of two vectors.
 *
 * @param a First vector.
 * @param b Second vector.
 * @return Scalar dot product a · b.
 */
double dot(const Vec2 &a, const Vec2 &b) { return a.x * b.x + a.y * b.y; }

/**
 * Returns the squared length of a vector.
 *
 * @param v Vector whose magnitude is requested.
 * @return Squared Euclidean length of @p v.
 *
 * Squared length avoids an unnecessary square root when only distances need
 * to be compared.
 */
double lengthSquared(const Vec2 &v) { return dot(v, v); }

/**
 * Returns the Euclidean length of a vector.
 *
 * @param v Vector whose magnitude is requested.
 * @return Euclidean length of @p v.
 */
double length(const Vec2 &v) { return std::sqrt(lengthSquared(v)); }

/**
 * Converts an HSV color to SDL-compatible 8-bit RGB components.
 *
 * @param h Hue in degrees. Values outside [0, 360) are wrapped around.
 * @param s Saturation in the range [0, 1].
 * @param v Value/brightness in the range [0, 1].
 * @return RGB color with 8-bit channels.
 */
Color hsvToRgb(double h, double s, double v) {
  // Normalize hue so the sector calculation below always operates on one
  // complete color wheel.
  h = std::fmod(h, 360.0);
  if (h < 0.0) {
    h += 360.0;
  }

  // Convert HSV into the intermediate chroma representation used by the
  // standard piecewise HSV-to-RGB conversion.
  const double c = v * s;
  const double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
  const double m = v - c;

  double r1 = 0.0;
  double g1 = 0.0;
  double b1 = 0.0;

  // Select the RGB sector corresponding to the hue. The temporary channels
  // are later shifted by m to restore the requested brightness.
  if (h < 60.0) {
    r1 = c;
    g1 = x;
  } else if (h < 120.0) {
    r1 = x;
    g1 = c;
  } else if (h < 180.0) {
    g1 = c;
    b1 = x;
  } else if (h < 240.0) {
    g1 = x;
    b1 = c;
  } else if (h < 300.0) {
    r1 = x;
    b1 = c;
  } else {
    r1 = c;
    b1 = x;
  }

  return {static_cast<std::uint8_t>((r1 + m) * 255.0),
          static_cast<std::uint8_t>((g1 + m) * 255.0),
          static_cast<std::uint8_t>((b1 + m) * 255.0)};
}

} // namespace

// -----------------------------------------------------------------------------
// Lifecycle and public state
// -----------------------------------------------------------------------------

/**
 * Initializes the simulation with a fixed ball radius and random seed.
 *
 * @param ballRadius Radius assigned to every ball created by this simulation.
 * @param seed Seed used to initialize the pseudo-random number generator.
 */
Simulation::Simulation(double ballRadius, std::uint32_t seed)
    : ballRadius_(ballRadius), rng_(seed), unit_(0.0, 1.0) {
  // Reset through the normal initialization path so construction and manual
  // restarts produce the same initial state.
  reset();
}

void Simulation::reset() {
  // Discard all existing balls and reset state that accumulates during a run.
  balls_.clear();
  finished_ = false;
  accumulator_ = 0.0;
  nextHue_ = 0.0;

  // Place the initial balls at random, non-overlapping positions.
  for (int i = 0; i < INITIAL_BALLS; ++i) {
    Vec2 position{};
    if (!findSpawnPosition(position)) {
      // Starting without the required initial configuration means the world
      // is already unable to satisfy the simulation's spawn constraints.
      finished_ = true;
      return;
    }

    balls_.push_back(makeBall(position));
  }

  // Explicitly generate the initial velocities for the two starting balls.
  // This keeps their motion independent from the spawn-position search.
  if (balls_.size() == 2) {
    balls_[0].velocity = randomVelocity();
    balls_[1].velocity = randomVelocity();
  }
}

void Simulation::update(double dt) {
  // Once no further spawning is possible, or no balls exist, there is nothing
  // left for the physics update to process.
  if (finished_ || balls_.empty()) {
    return;
  }

  // A fixed simulation step makes the physics less dependent on rendering FPS.
  constexpr double fixedDt = 1.0 / 240.0;

  // Clamp unusually large frame gaps so a temporary stall cannot produce a
  // very large physics catch-up that destabilizes the simulation.
  accumulator_ += std::min(dt, 0.1);

  // Consume the accumulated frame time in fixed-size physics steps.
  while (accumulator_ >= fixedDt && !finished_) {
    step(fixedDt);
    accumulator_ -= fixedDt;
  }
}

std::size_t Simulation::ballCount() const { return balls_.size(); }

bool Simulation::finished() const { return finished_; }

const std::vector<Ball> &Simulation::balls() const { return balls_; }

// -----------------------------------------------------------------------------
// Randomized ball creation
// -----------------------------------------------------------------------------

double Simulation::randomRange(double min, double max) {
  // Transform the normalized distribution [0, 1] into the requested range.
  return min + (max - min) * unit_(rng_);
}

Vec2 Simulation::randomVelocity() {
  // Sample direction independently from speed so every orientation is equally
  // likely while the magnitude remains within the configured limits.
  const double angle = randomRange(0.0, 2.0 * PI);
  const double speed = randomRange(MIN_SPEED, MAX_SPEED);

  return {std::cos(angle) * speed, std::sin(angle) * speed};
}

Ball Simulation::makeBall(const Vec2 &position) {
  // Golden-ratio hue stepping provides well-separated colors without storing
  // or maintaining a separate palette.
  const Color color = hsvToRgb(nextHue_, 0.72, 0.95);
  nextHue_ = std::fmod(nextHue_ + 137.50776405003785, 360.0);

  return {position, randomVelocity(), ballRadius_, color};
}

// -----------------------------------------------------------------------------
// Spawn-position validation and search
// -----------------------------------------------------------------------------

bool Simulation::isInsideWorld(const Vec2 &position) const {
  // The ball center must remain far enough from the origin that the entire
  // ball, not just its center, stays inside the circular boundary.
  const double allowed = WORLD_RADIUS - ballRadius_;
  return lengthSquared(position) <= allowed * allowed;
}

bool Simulation::overlapsAnyBall(const Vec2 &position) const {
  const double minDistance = 2.0 * ballRadius_;
  const double minDistanceSquared = minDistance * minDistance;

  // Compare squared distances so each candidate avoids an unnecessary sqrt.
  for (const Ball &ball : balls_) {
    if (lengthSquared(position - ball.position) < minDistanceSquared) {
      return true;
    }
  }

  return false;
}

bool Simulation::positionIsFree(const Vec2 &position) const {
  // A valid spawn position must satisfy both the world boundary and occupancy
  // constraints.
  return isInsideWorld(position) && !overlapsAnyBall(position);
}

bool Simulation::findSpawnPosition(Vec2 &result) {
  const double maxCenterRadius = WORLD_RADIUS - ballRadius_;

  // First try random sampling. It is inexpensive and usually succeeds while
  // a significant amount of free space remains.
  for (int attempt = 0; attempt < RANDOM_SPAWN_ATTEMPTS; ++attempt) {
    const double angle = randomRange(0.0, 2.0 * PI);
    const double r = std::sqrt(unit_(rng_)) * maxCenterRadius;

    // sqrt() compensates for the area represented by radius values so samples
    // are distributed across the disk rather than concentrated at the center.
    const Vec2 candidate{std::cos(angle) * r, std::sin(angle) * r};

    if (positionIsFree(candidate)) {
      result = candidate;
      return true;
    }
  }

  // Fall back to a deterministic spiral scan. This avoids declaring the world
  // full merely because random sampling missed a narrow remaining gap.
  const double goldenAngle = PI * (3.0 - std::sqrt(5.0));
  constexpr int maxSamples = 100000;

  for (int i = 0; i < maxSamples; ++i) {
    const double t = static_cast<double>(i) + 0.5;
    const double fraction = t / static_cast<double>(maxSamples);
    const double r = std::sqrt(fraction) * maxCenterRadius;
    const double angle = static_cast<double>(i) * goldenAngle;

    const Vec2 candidate{std::cos(angle) * r, std::sin(angle) * r};

    if (positionIsFree(candidate)) {
      result = candidate;
      return true;
    }
  }

  // Reaching this point means neither search strategy found a legal position,
  // so the simulation can no longer fulfill a collision-spawn request.
  std::cout << "[ FINISHED ]\n";

  return false;
}

// -----------------------------------------------------------------------------
// Physics update
// -----------------------------------------------------------------------------

void Simulation::step(double dt) {
  // 1) Move all balls and immediately correct any boundary violations.
  for (Ball &ball : balls_) {
    ball.position += ball.velocity * dt;
    keepInsideCircle(ball);
  }

  // 2) Resolve collisions among the balls that existed at the start of this
  // step. Each approaching pair schedules one new ball for spawning.
  //
  // "Approaching" is checked before the velocity response so an already
  // overlapping pair that is separating cannot repeatedly spawn balls.
  int collisionSpawns = 0;

  const std::size_t currentCount = balls_.size();

  for (std::size_t i = 0; i < currentCount; ++i) {
    for (std::size_t j = i + 1; j < currentCount; ++j) {
      if (resolveCollision(balls_[i], balls_[j])) {
        ++collisionSpawns;
      }
    }
  }

  // 3) Spawn after all existing collisions have been processed. This prevents
  // a newly-created ball from participating in the same physics step.
  for (int i = 0; i < collisionSpawns; ++i) {
    Vec2 spawnPosition{};

    if (!findSpawnPosition(spawnPosition)) {
      finished_ = true;
      return;
    }

    balls_.push_back(makeBall(spawnPosition));
  }
}

void Simulation::keepInsideCircle(Ball &ball) {
  const double maxDistance = WORLD_RADIUS - ball.radius;
  const double distance = length(ball.position);

  if (distance <= maxDistance) {
    return;
  }

  // Avoid dividing by a distance that is numerically indistinguishable from
  // zero when constructing the boundary normal.
  if (distance < EPSILON) {
    ball.position = {maxDistance, 0.0};
    return;
  }

  // Project the ball center back onto the allowed circle.
  const Vec2 normal = ball.position * (1.0 / distance);
  ball.position = normal * maxDistance;

  // Reflect only the velocity component pointing out through the boundary.
  // Tangential motion is left unchanged.
  const double outwardSpeed = dot(ball.velocity, normal);
  if (outwardSpeed > 0.0) {
    ball.velocity -= normal * (2.0 * outwardSpeed);
  }
}

bool Simulation::resolveCollision(Ball &a, Ball &b) {
  const Vec2 delta = b.position - a.position;
  const double distanceSquaredValue = lengthSquared(delta);

  // If the center distance is already large enough, the balls are not
  // overlapping and no collision response is needed.
  if (distanceSquaredValue >= (a.radius + b.radius) * (a.radius + b.radius)) {
    return false;
  }

  double distance = std::sqrt(distanceSquaredValue);

  // The exact-center case is extremely unlikely, but a valid normal is still
  // required for both positional separation and velocity response.
  Vec2 normal;
  if (distance < EPSILON) {
    const double angle = randomRange(0.0, 2.0 * PI);
    normal = {std::cos(angle), std::sin(angle)};
    distance = 0.0;
  } else {
    normal = delta * (1.0 / distance);
  }

  const Vec2 relativeVelocity = b.velocity - a.velocity;
  const double velocityAlongNormal = dot(relativeVelocity, normal);

  // Correct overlap even if the balls are already moving apart. The response
  // itself, and therefore spawning, only applies to a genuine approach.
  const bool isApproaching = velocityAlongNormal < 0.0;

  const double penetration = (a.radius + b.radius) - distance;

  if (penetration > 0.0) {
    // Treat the balls as equal-mass objects and split the positional
    // correction evenly between them. The small bias reduces re-overlap caused
    // by floating-point rounding.
    const Vec2 correction = normal * (penetration * 0.5 + 1e-4);
    a.position -= correction;
    b.position += correction;

    // Boundary correction can move a ball outside the world, so reapply the
    // boundary constraint after separating the pair.
    keepInsideCircle(a);
    keepInsideCircle(b);
  }

  if (!isApproaching) {
    return false;
  }

  // For equal masses and a perfectly elastic collision, exchanging the normal
  // velocity components is equivalent to applying the standard impulse.
  a.velocity += normal * velocityAlongNormal;
  b.velocity -= normal * velocityAlongNormal;

  return true;
}

} // namespace simulation

# Bouncing Balls — Collision Spawns a New Ball

A small C++20 + SDL2 physics simulation for Ubuntu.

The simulation starts with two balls moving inside a circular boundary. Whenever two balls collide, a new ball is created and placed at a free random position inside the circle. The process continues until there is no longer enough space to place another complete ball.

## Rules

1. Start with two balls at random, non-overlapping positions inside a circle.
2. Every ball moves with a constant velocity.
3. Balls bounce elastically against the circular boundary.
4. When two balls collide while approaching each other, a new ball is created.
5. The new ball is placed at a random free position inside the circle.
6. If there is no available space for another complete ball, the simulation finishes.

## 1. Install Dependencies

The project requires:

* C++20-compatible compiler
* SDL2 development libraries
* `pkg-config`

On Ubuntu 24.04:

```bash
sudo apt update
sudo apt install build-essential libsdl2-dev pkg-config
```

## 2. Project Structure

```text
.
├── include/
│   ├── renderer.hpp      # Rendering interface
│   └── simulation.hpp    # Public simulation data types and class interface
├── src/
│   ├── main.cpp          # Application entry point, CLI, SDL setup, event loop
│   ├── renderer.cpp      # SDL2 drawing implementation
│   └── simulation.cpp    # Simulation state, physics, collisions, spawning
├── .gitignore            # Files and directories excluded from Git
├── LICENSE               # Project license
├── Makefile              # Make build configuration
└── README.md             # Project documentation
```

### `src/main.cpp`

Contains the application layer:

* Command-line argument handling
* SDL2 initialization and shutdown
* Window and renderer creation
* Keyboard and window-event handling
* Main application loop
* Simulation update/render orchestration

`main.cpp` does not contain the physics implementation.

### `include/simulation.hpp` / `src/simulation.cpp`

Contain the reusable simulation module:

* 2D vector operations
* Ball representation
* Ball movement
* Circular wall collision detection and response
* Ball-to-ball collision detection and response
* New-ball spawning
* Available-space detection
* Fixed-timestep simulation

The simulation module is intentionally independent of SDL2. It can therefore be reused by another front end without coupling the physics code to the renderer.

### `include/renderer.hpp` / `src/renderer.cpp`

Contain the SDL2 rendering layer:

* Background clearing
* Circular boundary rendering
* Filled-ball rendering
* Conversion from simulation coordinates to window coordinates

This keeps graphics-specific code separate from the simulation logic.

### `Makefile`

Provides a simple command-line build interface using `g++` and `sdl2-config`.

It automatically:

* Compiles every `src/*.cpp` file into an object file.
* Generates dependency files for header changes.
* Links all object files into the final executable.

Available targets:

| Command      | Description                    |
| ------------ | ------------------------------ |
| `make`       | Build the application          |
| `make all`   | Build the application          |
| `make run`   | Build and run the application  |
| `make clean` | Remove generated build files   |

### `.gitignore`

Contains files and directories that should not be committed to the repository, such as the compiled executable.

### `LICENSE`

Contains the license under which the project is distributed.

## 3. Build with Make

The simplest way to build the project is:

```bash
make
```

This builds the executable:

```text
bouncing_balls
```

`make` is equivalent to:

```bash
make all
```

### Build and run

```bash
make run
```

This builds the application if necessary and then starts it.

### Run manually

After building:

```bash
./bin/bouncing_balls
```

### Clean the build

To remove the executable and generated object/dependency files:

```bash
make clean
```

## 4. Controls

| Key     | Action                 |
| ------- | ---------------------- |
| `Space` | Pause / Resume         |
| `R`     | Restart the simulation |
| `Esc`   | Quit                   |
| `Q`     | Quit                   |

The number of balls is displayed in the window title.

When the simulation runs out of available space, the window title indicates that the simulation has finished.

## 5. Optional Parameters

The executable accepts several optional command-line parameters.

### Ball radius

```bash
./bin/bouncing_balls --radius 10
```

Changes the radius of every ball.

The default radius is:

```text
12 pixels
```

### Window size

```bash
./bin/bouncing_balls --width 1000 --height 1000
```

Changes the window dimensions.

The default window size is:

```text
800 × 800 pixels
```

### Random seed

```bash
./bin/bouncing_balls --seed 12345
```

Specifies the random seed.

Using a fixed seed is useful when reproducing or debugging a particular simulation run.

For example:

```bash
./bin/bouncing_balls --seed 12345
```

should produce the same initial random conditions each time.

### Show command-line help

```bash
./bin/bouncing_balls --help
```

## 6. Physics & Algorithm Details

### 6.1 Coordinate System

The simulation uses a 2D Cartesian coordinate system.

The center of the circular world is considered:

```text
(0, 0)
```

The SDL window coordinates are only used when rendering the simulation.

The circular world has a radius of:

```text
380 pixels
```

The actual ball center is therefore constrained to:

```text
WORLD_RADIUS - ball_radius
```

from the center of the world.

This ensures that the entire ball remains inside the circle rather than allowing its center to reach the boundary.

### 6.2 Ball Movement

Each ball has:

* Position
* Velocity
* Radius
* Color

At every physics step, the position is updated using:

```text
position += velocity × dt
```

The velocity remains constant unless the ball collides with another ball or with the circular boundary.

### 6.3 Physics Time Step

The simulation uses a fixed physics time step of:

```text
1 / 240 seconds
```

This separates the physics simulation from the rendering frame rate.

A fixed timestep makes collision behaviour more predictable and reduces differences caused by varying rendering performance.

The real elapsed time is accumulated and processed in fixed-size physics steps.

### 6.4 Wall Reflection Mechanics

The circular boundary is treated as a perfectly elastic wall.

When a ball reaches the boundary, its position is corrected so that the entire ball remains inside the circle.

The collision normal is calculated from the center of the world toward the ball:

```text
normal = normalize(ball_position)
```

The velocity is then reflected around that normal.

For a velocity `v` and collision normal `n`, the reflected velocity is:

```text
v' = v - 2(v · n)n
```

Only the component of the velocity pointing outward from the circle is reflected.

This produces a physically consistent bounce while preserving the ball's speed.

### 6.5 Ball-to-Ball Collision Detection

Two balls are considered to be colliding when the distance between their centers is smaller than the sum of their radii:

```text
distance < radiusA + radiusB
```

For two balls with equal mass, the collision normal is the normalized vector between their centers.

The relative velocity along this normal is calculated to determine whether the balls are actually approaching each other.

This distinction is important because two balls can temporarily overlap due to numerical integration while already moving apart. Such an overlap should not create another ball.

### 6.6 Ball-to-Ball Elastic Collision

The balls are treated as having equal mass and the collision is perfectly elastic.

For an equal-mass collision, the velocity components along the collision normal are exchanged between the two balls.

The tangential velocity components remain unchanged.

This preserves kinetic energy and momentum within the limits of floating-point numerical precision.

### 6.7 Collision → New Ball

When two balls are detected to be approaching each other and collide:

1. Their positions are corrected if necessary.
2. Their velocities are updated according to the elastic collision equations.
3. A new ball is created.
4. The program searches for a valid position for the new ball.
5. If a valid position is found, the new ball is inserted into the simulation.
6. If no valid position can be found, the simulation finishes.

Newly created balls do not participate in the current collision-processing pass. They start participating from the following physics step.

This prevents a newly created ball from immediately triggering additional collisions during the same update.

### 6.8 Space Availability Check

A new ball can only be created if its entire area fits inside the circular world and it does not overlap any existing ball.

A candidate position is therefore valid when both conditions are true:

```text
distance(candidate, world_center)
    <= WORLD_RADIUS - ball_radius
```

and:

```text
distance(candidate, existing_ball)
    >= 2 × ball_radius
```

for every existing ball.

### 6.9 Finding a Free Position

The program uses two methods.

#### Random search

While there is plenty of free space, the program generates random candidate positions inside the circle and checks whether they are valid.

This gives the new balls a natural random distribution.

The radius of the random point is generated using:

```text
r = sqrt(random) × maximum_radius
```

rather than simply using a uniformly distributed radius.

The square root is important because it produces a uniform distribution over the **area** of the circle.

#### Deterministic fallback search

As the circle becomes increasingly crowded, random sampling becomes inefficient.

After a configurable number of unsuccessful random attempts, the program performs a deterministic spiral-like scan based on the golden angle.

This greatly reduces the possibility of incorrectly concluding that the circle is full simply because random sampling failed to find a free position.

The simulation only finishes when both approaches fail to find a valid position.

### 6.10 Collision Processing

During each physics step, the program first moves all existing balls.

It then checks every pair of balls:

```text
ball 0 ↔ ball 1
ball 0 ↔ ball 2
ball 1 ↔ ball 2
...
```

Only the balls that existed at the beginning of that physics step are considered during that collision pass.

New balls are added afterwards.

This prevents newly spawned balls from being processed recursively within the same physics step.

## 7. Rendering

SDL2 is used for:

* Creating the application window
* Creating the renderer
* Drawing the circular boundary
* Drawing the balls
* Handling keyboard and window events

The project does not use an external physics engine.

Physics and simulation state are implemented in `src/simulation.cpp`, while SDL2 rendering is implemented in `src/renderer.cpp`.

The balls are rendered as filled circles and the simulation boundary as a circular outline.

## 8. Randomness and Reproducibility

The simulation uses C++'s `<random>` facilities.

By default, the random number generator is initialized using `std::random_device`.

This means that consecutive executions normally produce different simulations.

A fixed seed can be provided with:

```bash
./bin/bouncing_balls --seed 12345
```

This is particularly useful for:

* Debugging
* Comparing changes to the physics
* Reproducing a particular behaviour
* Testing

## 9. Implementation Notes

The project intentionally does not use a physics engine.

The physics required by this simulation are implemented directly using basic vector mathematics.

The main components are:

* 2D vector operations
* Circle-circle intersection tests
* Vector projection
* Elastic collision response
* Reflection around a surface normal
* Random point generation inside a circle
* Spatial availability checks
* Fixed-timestep simulation
* SDL2 rendering

---

## License

See [`LICENSE`](LICENSE) for the license applicable to this project.

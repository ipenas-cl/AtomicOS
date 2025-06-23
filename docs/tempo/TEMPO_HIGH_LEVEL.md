# Tempo: High-Level Language with Low-Level Power

## Write Python, Get Better-Than-Assembly Performance

Tempo combines the **ease of Python** with **performance beyond assembly**, making systems programming accessible to everyone.

## Simple Syntax, Powerful Results

### Hello World - As Easy as Python
```tempo
function main() {
    print("Hello, World!");
}
```

### Web Server - Like Node.js, But Deterministic
```tempo
@wcet(1000)  // Guaranteed response time!
function handle_request(request: HttpRequest) -> HttpResponse {
    match request.path {
        "/" => return HttpResponse::ok("<h1>Welcome to Tempo!</h1>"),
        "/api/data" => return serve_json(get_data()),
        _ => return HttpResponse::not_found()
    }
}

// Automatic async/await with deterministic timing
async function start_server() {
    let server = HttpServer::new("0.0.0.0:8080");
    
    while true {
        let request = await server.accept();  // Non-blocking, deterministic
        spawn handle_request(request);        // Lightweight tasks
    }
}
```

### Game Development - Unity-Simple, AAA Performance
```tempo
struct Player {
    position: Vec3,
    health: int32,
    inventory: Vec<Item>
}

// Automatic SIMD optimization
function update_physics(entities: Vec<Entity>, delta_time: float32) {
    for entity in entities {
        entity.velocity += entity.acceleration * delta_time;
        entity.position += entity.velocity * delta_time;
        
        // Automatic spatial partitioning
        check_collisions(entity);
    }
}

// Real-time guarantees for 144 FPS
@fps(144)
function game_loop() {
    while running {
        handle_input();
        update_physics(entities, 1.0/144.0);
        render_scene();
        
        // Compiler ensures we hit framerate
        wait_for_next_frame();
    }
}
```

### Database - Like PostgreSQL, But Faster
```tempo
// Type-safe SQL with compile-time optimization
function find_users(age: int32) -> Vec<User> {
    // This compiles to optimal assembly with indices
    return query!{
        SELECT * FROM users 
        WHERE age > {age}
        ORDER BY created_at DESC
        LIMIT 100
    };
}

// Automatic transaction handling
@transaction
function transfer_money(from: Account, to: Account, amount: Money) -> Result {
    if from.balance < amount {
        return Error("Insufficient funds");
    }
    
    from.balance -= amount;  // Automatic rollback on error
    to.balance += amount;
    
    log_transaction(from, to, amount);
    return Ok();
}
```

### AI/ML - Like PyTorch, But Real-Time
```tempo
// Automatic GPU acceleration
@gpu
function neural_network(input: Tensor) -> Tensor {
    return input
        |> linear(784, 256)
        |> relu()
        |> linear(256, 128)
        |> relu()
        |> linear(128, 10)
        |> softmax();
}

// Guaranteed inference time for robotics
@wcet(1_000_000)  // 1ms max
function autonomous_drive(camera: Image, lidar: PointCloud) -> Control {
    let objects = detect_objects(camera);      // YOLO, optimized
    let path = plan_route(lidar, objects);    // A*, deterministic
    return compute_control(path);              // MPC, real-time
}
```

### IoT/Embedded - Like Arduino, But Professional
```tempo
// As simple as Arduino
function blink_led() {
    let led = Pin::new(13, Output);
    
    loop {
        led.high();
        delay_ms(1000);
        led.low();
        delay_ms(1000);
    }
}

// But with professional features
@interrupt @wcet(100)
function handle_sensor() {
    let reading = ADC::read(0);
    
    if reading > THRESHOLD {
        send_alert(reading);  // Non-blocking
    }
    
    store_reading(reading);   // Lock-free buffer
}
```

## Features That Make Life Easy

### 1. Automatic Memory Management (No GC!)
```tempo
function process_data() {
    let buffer = Vec::new();  // Stack allocated when possible
    
    for i in 0..1000 {
        buffer.push(compute(i));  // Automatic growth
    }
    
    return buffer;  // Move semantics, no copy
}  // Automatic cleanup, no GC pause
```

### 2. Error Handling Like Rust, But Simpler
```tempo
function read_config() -> Result<Config> {
    let file = File::open("config.json")?;  // ? operator
    let data = file.read_all()?;
    let config = Json::parse(data)?;
    return Ok(config);
}

// Or use exceptions if you prefer
function main() {
    try {
        let config = read_config();
        start_app(config);
    } catch (e: Error) {
        log_error(e);
        use_defaults();
    }
}
```

### 3. Powerful Type System, Optional Typing
```tempo
// Full type inference
function add(a, b) {
    return a + b;  // Works with int, float, vectors...
}

// Or explicit types for clarity
function calculate_tax(income: Money) -> Money {
    return income * 0.25;
}

// Generics made simple
function find_max<T: Comparable>(items: Vec<T>) -> T {
    return items.reduce((a, b) => a > b ? a : b);
}
```

### 4. Built-in Concurrency
```tempo
// Channels like Go
function producer(ch: Channel<int32>) {
    for i in 0..100 {
        ch.send(i);
    }
}

function consumer(ch: Channel<int32>) {
    while let Some(value) = ch.receive() {
        process(value);
    }
}

// Async/await like JavaScript
async function fetch_all(urls: Vec<String>) -> Vec<Response> {
    // Parallel requests, deterministic completion
    return await Promise::all(
        urls.map(url => fetch(url))
    );
}
```

### 5. Package Manager and Ecosystem
```tempo
// tempo.toml
[dependencies]
web = "1.0"
graphics = "2.5"
ai = "0.8"

// main.tempo
import web::{Server, Response};
import graphics::{Window, render};
import ai::{NeuralNet, train};

function main() {
    // Just works!
}
```

## Real-World Applications

### 1. Web Services (Better than Go)
```tempo
@service
struct UserService {
    db: Database,
    cache: Cache,
}

impl UserService {
    @route("GET", "/users/{id}")
    @wcet(10_000)  // 10ms guaranteed response
    async function get_user(id: int32) -> Response {
        // Try cache first
        if let Some(user) = self.cache.get(id).await {
            return Response::json(user);
        }
        
        // Fallback to database
        match self.db.find_user(id).await {
            Ok(user) => {
                self.cache.set(id, user).await;
                Response::json(user)
            },
            Err(_) => Response::not_found()
        }
    }
}
```

### 2. Game Engines (Better than C++)
```tempo
struct GameObject {
    transform: Transform,
    mesh: Mesh,
    scripts: Vec<Component>
}

// ECS pattern, but simple
function update_world(world: World, dt: float32) {
    // Automatic parallelization
    parallel_for system in world.systems {
        system.update(world.entities, dt);
    }
    
    // Physics with guaranteed framerate
    world.physics.step(dt);
    
    // Render with automatic culling
    world.renderer.draw();
}
```

### 3. Operating Systems (Yes, Really!)
```tempo
// Write your own OS in high-level code!
@kernel
function main() {
    init_memory();
    init_interrupts();
    init_scheduler();
    
    // Create init process
    spawn init_process();
    
    // Start scheduling
    scheduler.run();
}

function init_process() {
    mount_filesystems();
    start_services();
    
    // Launch shell
    exec("/bin/shell");
}
```

## Learning Path

### Week 1: Basics
- Variables and functions
- Control flow
- Error handling
- Simple programs

### Week 2: Intermediate
- Structs and methods
- Generics
- Async programming
- Building APIs

### Week 3: Advanced
- WCET annotations
- Security features
- Optimization hints
- System programming

### Week 4: Master
- Writing drivers
- Real-time systems
- Kernel modules
- Your own OS!

## Why Developers Love Tempo

> "I replaced 10,000 lines of C++ with 1,000 lines of Tempo. It's faster, safer, and I can actually understand my own code!" - Game Developer

> "We migrated from Go to Tempo. Our API latency dropped 10x and we guarantee response times!" - Backend Engineer  

> "I'm a Python developer who never thought I could write embedded code. With Tempo, I built a drone autopilot!" - ML Engineer

## Get Started Today

```bash
# Install Tempo
curl -sSf https://tempo-lang.org/install | sh

# Create new project
tempo new my_app
cd my_app

# Run it!
tempo run
```

## Conclusion

Tempo proves you don't need to choose between:
- **Easy to write** vs **Fast to run**
- **High-level** vs **Low-level control**
- **Safe** vs **Performant**
- **Productive** vs **Predictable**

Write code like Python. Get performance better than assembly. That's the Tempo promise.
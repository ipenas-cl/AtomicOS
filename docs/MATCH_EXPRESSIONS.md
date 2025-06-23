# Tempo Match Expressions

## Overview

Match expressions in Tempo provide powerful pattern matching capabilities with deterministic execution time guarantees. Unlike traditional switch statements, match expressions are exhaustive, type-safe, and can destructure complex data types.

## Syntax

```tempo
match expression {
    pattern1 => result1,
    pattern2 => result2,
    pattern3 => {
        // Multi-line block
        statement1;
        statement2;
        result3
    },
    _ => default_result
}
```

## Pattern Types

### Literal Patterns

Match against specific values:

```tempo
@wcet(20)
function process_code(code: int32) -> ptr<char> {
    match code {
        0 => "Success",
        1 => "Error",
        2 => "Warning",
        _ => "Unknown"
    }
}
```

### Range Patterns

Match numeric ranges:

```tempo
@wcet(25)
function categorize_score(score: int32) -> ptr<char> {
    match score {
        0..59 => "F",
        60..69 => "D",
        70..79 => "C",
        80..89 => "B",
        90..100 => "A",
        _ => "Invalid"
    }
}
```

### Variable Patterns

Bind matched values to variables:

```tempo
@wcet(15)
function extract_value(opt: Option<int32>) -> int32 {
    match opt {
        Some(value) => value,
        None => 0
    }
}
```

### Wildcard Pattern

The `_` pattern matches anything:

```tempo
@wcet(20)
function is_ready(state: ProcessState) -> bool {
    match state {
        ProcessState::Ready => true,
        _ => false
    }
}
```

### Guard Patterns

Add conditions to patterns:

```tempo
@wcet(30)
function classify_temp(temp: int32) -> ptr<char> {
    match temp {
        t if t < 0 => "Freezing",
        t if t < 20 => "Cold",
        t if t < 30 => "Comfortable",
        _ => "Hot"
    }
}
```

### Enum Patterns

Match and destructure enum variants:

```tempo
enum Result {
    Ok { value: int32 },
    Error { code: int32, msg: ptr<char> }
}

@wcet(40)
function handle_result(res: Result) -> int32 {
    match res {
        Result::Ok { value } => value,
        Result::Error { code, .. } => -code
    }
}
```

### Struct Patterns

Destructure structs in patterns:

```tempo
struct Point {
    x: int32,
    y: int32
}

@wcet(25)
function quadrant(p: Point) -> int32 {
    match p {
        Point { x: 0, y: 0 } => 0,
        Point { x, y } if x > 0 && y > 0 => 1,
        Point { x, y } if x < 0 && y > 0 => 2,
        Point { x, y } if x < 0 && y < 0 => 3,
        Point { x, y } if x > 0 && y < 0 => 4,
        _ => -1
    }
}
```

## WCET Analysis

### Pattern Matching Overhead

Each pattern type has different WCET costs:

- **Literal**: 2 cycles (comparison + jump)
- **Range**: 6 cycles (2 comparisons + 2 jumps)
- **Enum**: 4 + 2n cycles (n = field count)
- **Guard**: Pattern cost + guard expression cost
- **Wildcard**: 0 cycles (always matches)

### Jump Table Optimization

When matching on dense integer or enum values, the compiler generates a jump table:

```tempo
// This generates a jump table
@wcet(15)
function state_to_int(state: ProcessState) -> int32 {
    match state {
        ProcessState::Ready => 1,
        ProcessState::Running => 2,
        ProcessState::Blocked => 3,
        ProcessState::Suspended => 4,
        ProcessState::Zombie => 5
    }
}
```

Jump table WCET: O(1) - constant time lookup

## Exhaustiveness Checking

The compiler verifies that all possible values are covered:

```tempo
// Compiler error: non-exhaustive match
function bad_match(state: ProcessState) -> int32 {
    match state {
        ProcessState::Ready => 1,
        ProcessState::Running => 2
        // Missing: Blocked, Suspended, Zombie
    }
}

// OK: exhaustive with wildcard
function good_match(state: ProcessState) -> int32 {
    match state {
        ProcessState::Ready => 1,
        ProcessState::Running => 2,
        _ => 0
    }
}
```

## Code Generation

### Sequential Matching

For complex patterns, the compiler generates sequential checks:

```asm
; Match expression on [ebp-4]
; Total WCET: 25 cycles
mov eax, [ebp-4]

.match_arm_0:
    cmp eax, 0
    jne .match_arm_1
    ; Arm 0 body (WCET: 5)
    mov eax, 1
    jmp .match_end

.match_arm_1:
    cmp eax, 1
    jne .match_arm_2
    ; Arm 1 body (WCET: 5)
    mov eax, 10
    jmp .match_end

.match_arm_2:
    ; Wildcard - always matches
    ; Arm 2 body (WCET: 5)
    mov eax, 0

.match_end:
```

### Jump Table Generation

For simple literal patterns:

```asm
; Jump table match
sub eax, 0         ; Normalize to 0-based
cmp eax, 4         ; Check upper bound
ja .match_default
jmp [.jump_table + eax * 4]

.jump_table:
    dd .match_body_0
    dd .match_body_1
    dd .match_body_2
    dd .match_body_3
    dd .match_body_4

.match_body_0:
    mov eax, 1
    jmp .match_end

; ... other bodies ...

.match_default:
    ud2    ; Unreachable

.match_end:
```

## Best Practices

### 1. Order Patterns by Frequency

Place most common patterns first for better average-case performance:

```tempo
match msg_type {
    // Common cases first
    MsgType::Data => process_data(),
    MsgType::Ack => process_ack(),
    // Rare cases last
    MsgType::Error => handle_error(),
    _ => ignore()
}
```

### 2. Use Jump Tables When Possible

Structure enums to have contiguous values:

```tempo
enum Command {
    Read = 0,
    Write = 1,
    Seek = 2,
    Close = 3
}
```

### 3. Minimize Guard Complexity

Keep guard expressions simple for predictable WCET:

```tempo
// Good: simple guard
match value {
    v if v > 0 => positive_case(),
    _ => other_case()
}

// Avoid: complex guard
match value {
    v if complex_check(v) && another_check(v) => case1(),
    _ => case2()
}
```

### 4. Prefer Exhaustive Matches

Avoid wildcard when all cases are known:

```tempo
// Better: explicitly handle all cases
match state {
    State::A => handle_a(),
    State::B => handle_b(),
    State::C => handle_c()
}

// Worse: wildcard hides missing cases
match state {
    State::A => handle_a(),
    _ => default_handle()
}
```

## Integration with Tempo Features

### Constant-Time Matching

For security-critical code:

```tempo
@wcet(50)
@constant_time
function secure_dispatch(cmd: int32) -> int32 {
    // Compiler ensures constant-time execution
    match cmd {
        0 => operation_a(),
        1 => operation_b(),
        2 => operation_c(),
        _ => noop()
    }
}
```

### Real-Time Constraints

Match expressions respect function WCET bounds:

```tempo
@wcet(100)
@realtime(deadline: 100)
function rt_handler(event: Event) -> void {
    match event {
        Event::Timer => handle_timer(),      // WCET: 20
        Event::IO => handle_io(),            // WCET: 30
        Event::Network => handle_network(),  // WCET: 40
        _ => {}                              // WCET: 0
    }
    // Total WCET: 40 + overhead < 100
}
```

## Limitations

1. **Pattern Complexity**: Deep pattern matching increases WCET
2. **Memory Usage**: Jump tables require memory proportional to value range
3. **Type Restrictions**: Only matchable types (enums, integers, etc.)
4. **No Side Effects**: Patterns cannot have side effects

## Future Enhancements

- **Active Patterns**: User-defined pattern functions
- **View Patterns**: Pattern aliases and transformations
- **Or-patterns**: Multiple patterns in one arm
- **Pattern Synonyms**: Reusable pattern definitions
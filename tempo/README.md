# Tempo v5.0.0 - Pure Tempo

The deterministic programming language - 100% written in Tempo!

## 🚀 Installation

### Get the binary
```bash
# Download pre-compiled Tempo
wget https://github.com/ipenas-cl/AtomicOS/releases/download/v5.0.0/tempo
chmod +x tempo
sudo mv tempo /usr/local/bin/
```

### Or use the installed version
```bash
/usr/local/bin/tempo --version
```

## 📁 Pure Tempo Structure

```
tempo/
├── src/                    # Tempo compiler written in Tempo!
│   ├── compiler.tempo      # Main compiler
│   ├── lexer.tempo         # Tokenizer
│   ├── parser.tempo        # Parser
│   ├── ast.tempo           # Abstract syntax tree
│   └── codegen_macos.tempo # Code generator
├── docs/                   # Documentation
│   └── *.md                # All docs
├── examples/               # Example programs
│   ├── beginner/          # Hello world, etc
│   ├── intermediate/      # Structs, arrays
│   ├── advanced/          # Real-time, security
│   └── projects/          # Complete apps
└── tests/                 # Test suite
```

## 🎯 Philosophy

**NO C, NO C++, NO DEPENDENCIES!**

Tempo is self-hosted - the Tempo compiler is written in Tempo itself. This means:
- 🚫 No C files
- 🚫 No header files
- 🚫 No external dependencies
- ✅ 100% Pure Tempo

## 💻 Usage

```bash
# Compile a Tempo program
tempo hello.tempo hello.s

# Assemble and link
as hello.s -o hello.o
ld hello.o -o hello

# Run
./hello
```

## 🔥 Self-Hosting

Tempo compiles itself! Check out `src/compiler.tempo` to see how:

```tempo
// From src/compiler.tempo
fn main() -> i32 {
    let args = get_args();
    if (args.len < 3) {
        print("Usage: tempo <input.tempo> <output.s>\n");
        return 1;
    }
    
    let tokens = lex_file(args[1]);
    let ast = parse(tokens);
    let code = generate_code(ast);
    write_file(args[2], code);
    
    return 0;
}
```

## 📚 Learn Tempo

1. **Quick Start**: See `examples/beginner/01_hello_world.tempo`
2. **Tutorial**: Read `docs/LEARN_TEMPO.md`
3. **Examples**: Browse `examples/` directory

## 🌟 Why Tempo?

- **Deterministic**: Same timing, every run
- **Secure**: Built-in security levels
- **Fast**: No garbage collector, no runtime
- **Simple**: Easy syntax, hard to misuse
- **Self-Hosted**: Tempo is written in Tempo!

## 🤝 Contributing

Want to improve Tempo? Edit the Tempo source files in `src/`!

1. Modify `src/*.tempo` files
2. Compile with existing tempo binary
3. Test your changes
4. Submit a pull request

## 📜 License

MIT License - Copyright (c) 2025 Ignacio Peña Sepúlveda

---

**Tempo™** - The language that compiles itself!
Email: ignacio@tempo-lang.org
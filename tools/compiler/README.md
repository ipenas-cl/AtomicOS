# Tempo Compiler Assembly Components

This directory contains assembly implementations of various Tempo compiler components.

## Files

- `tempo_compiler_full.s` - Full compiler implementation in assembly
- `tempo_compiler_macho.s` - macOS Mach-O format compiler
- `tempo_parser_native.s` - Native assembly parser implementation
- `tempo_pure.s` - Pure assembly compiler core
- `tempo_pure_final.s` - Final optimized pure assembly version

## Purpose

These files represent:
- Assembly implementations of the Tempo compiler
- Parser components written in assembly
- Platform-specific compiler variants
- Research into self-hosting capabilities

## Status

These are experimental assembly versions. The primary compiler is `tempo_compiler.c` in the parent directory.
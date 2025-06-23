# Tempo Linker Components

This directory contains various versions of the Tempo linker implemented in assembly.

## Files

- `tempo_linker.s` - Base linker implementation
- `tempo_linker_v2.s` - Version 2 with enhanced features
- `tempo_linker_100.s` - Optimized version
- `tempo_linker_final.s` - Final production version
- `tempo_linker_pure.s` - Pure assembly implementation
- `tempo_linker_real.s` - Real mode linker

## Purpose

These linkers are responsible for:
- Linking Tempo object files
- Resolving symbols and relocations
- Generating final executables
- Supporting the self-hosted Tempo compiler

## Status

These are experimental/research versions of the linker. The active linker functionality is integrated into the main Tempo compiler.
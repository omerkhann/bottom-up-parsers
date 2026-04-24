# Bottom-Up Parser Suite (SLR(1) + LR(1)) — C++17

This workspace contains a modular C++ implementation of:

- SLR(1) parsing: LR(0) item sets + FOLLOW-based reductions
- LR(1) parsing: LR(1) item sets + lookahead-based reductions
- Stack-based shift-reduce driver with step trace and parse-tree construction

## Team

- **Member 1**: Muhammad Omer Khan — 23I-0650
- **Member 2**: Muhammad Sameer    — 23I-0634

## Language

- **Programming language**: C++17

## Build

Using `g++`:

```bash
make
```

## Run

```bash
./parser --grammar input/grammar_with_conflict.txt --tokens "id = id"
```

Outputs are written into `output/`:

- `augmented_grammar.txt`
- `slr_items.txt`, `slr_parsing_table.txt`, `slr_trace.txt`
- `lr1_items.txt`, `lr1_parsing_table.txt`, `lr1_trace.txt`
- `parse_trees.txt`
- `comparison.txt` (SLR(1) vs LR(1) performance comparison)

## Grammar input format

One production per line:

```
NonTerminal -> sym1 sym2 | sym3
```

- Non-terminals must start with uppercase (multi-character names supported)
- Terminals can be operators / keywords / lowercase strings (e.g. `id`, `*`, `=`)
- Epsilon may be written as `@` (internally normalized to `@`)

## Input file format specification

- **Grammar file**: plain text, one production per line (see above).
- **Input string**: provided via command line `--tokens`, tokens separated by whitespace.

## Sample commands

- **Run SLR(1)** (table + trace produced):

```bash
./parser --grammar input/expr_grammar.txt --tokens "id + id * id"
```

- **Run LR(1)**:

The same command produces LR(1) outputs too (`output/lr1_*`).

```bash
./parser --grammar input/grammar_with_conflict.txt --tokens "id = id"
```

## Known limitations

- <Add any known limitations here, if any.>

## Make targets (quick testing)

```bash
make run-conflict
make run-expr
make run-epsilon
make run-ambiguous
make run-rr
make run-dangling
```

Or run any grammar/tokens:

```bash
make run GRAMMAR=input/dangling_else.txt TOKENS="if true then other else other"
```


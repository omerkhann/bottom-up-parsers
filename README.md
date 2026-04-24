# Bottom-Up Parser Suite (SLR(1) + LR(1)) — C++17

This workspace contains a modular C++ implementation of:

- SLR(1) parsing: LR(0) item sets + FOLLOW-based reductions
- LR(1) parsing: LR(1) item sets + lookahead-based reductions
- Stack-based shift-reduce driver with step trace and parse-tree construction

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

## Grammar input format

One production per line:

```
NonTerminal -> sym1 sym2 | sym3
```

- Non-terminals must start with uppercase (multi-character names supported)
- Terminals can be operators / keywords / lowercase strings (e.g. `id`, `*`, `=`)
- Epsilon may be written as `@` (internally normalized to `@`)

## Make targets (quick testing)

```bash
make run-conflict
make run-expr
make run-epsilon
make run-ambiguous
make run-rr
```

Or run any grammar/tokens:

```bash
make run GRAMMAR=input/dangling_else.txt TOKENS="if true then other else other"
```


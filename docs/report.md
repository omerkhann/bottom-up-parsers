## Bottom-Up Parser Design & Implementation (SLR(1) and LR(1))

### 1. Introduction
Bottom-up parsing constructs a parse for an input string by starting from the input tokens and repeatedly applying **shift** and **reduce** operations until the start symbol is produced. LR-family parsers (LR(0), SLR(1), LR(1), LALR(1)) are among the most widely used bottom-up parsers because they support deterministic parsing for a large class of grammars and provide strong error detection.

This project implements a **Bottom-Up Parser Suite** supporting:
- **SLR(1)**: uses LR(0) item sets with FOLLOW-based reduction placement.
- **LR(1)**: uses LR(1) item sets with explicit lookahead terminals, often resolving conflicts that SLR(1) cannot.

### 2. Approach

#### 2.1 Data Structures
- **Grammar**
  - Stores productions in normalized form \(A \to \alpha\).
  - Supports grammar augmentation with a fresh start symbol \(S' \to S\).
  - Computes FIRST and FOLLOW sets.
  - Supports epsilon productions (written as `@` or `epsilon` in input).
- **Items and Item Sets**
  - LR(0) items: \(A \to \alpha \cdot \beta\)
  - LR(1) items: \([A \to \alpha \cdot \beta, a]\) where \(a\) is the lookahead terminal.
  - ItemSet (state): collection of items plus transitions computed by GOTO.
- **Parsing Table**
  - ACTION entries: shift/reduce/accept on terminals.
  - GOTO entries: transitions on nonterminals.
  - Tracks and reports conflicts (shift/reduce, reduce/reduce).
- **Shift-Reduce Driver**
  - Maintains parallel stacks of **states**, **symbols**, and **parse tree nodes**.
  - Emits step-by-step trace: step number, stack snapshot, remaining input, action.
- **Parse Tree**
  - Constructed from reduction sequence.
  - Each reduction creates a parent node with children corresponding to RHS symbols.

#### 2.2 Algorithms (CLOSURE and GOTO)

##### LR(0) CLOSURE
Given item set \(I\):
1. Initialize closure with \(I\).
2. For each item \(A \to \alpha \cdot B \beta\) in the set where \(B\) is a nonterminal:
   - Add \(B \to \cdot \gamma\) for every production \(B \to \gamma\).
3. Repeat until no new items are added.

##### LR(0) GOTO
For grammar symbol \(X\):
1. For each item \(A \to \alpha \cdot X \beta\), move dot: \(A \to \alpha X \cdot \beta\).
2. Return CLOSURE of the resulting set.

##### LR(1) CLOSURE (lookahead propagation)
For each LR(1) item \([A \to \alpha \cdot B \beta, a]\):
1. For each production \(B \to \gamma\):
2. Compute \(FIRST(\beta a)\)
3. For each terminal \(b\) in that FIRST set (excluding epsilon unless needed):
   - Add \([B \to \cdot \gamma, b]\)
4. Repeat until no new items are added.

##### LR(1) GOTO
Same dot-move logic as LR(0), but items carry lookaheads; return LR(1) CLOSURE of the moved set.

#### 2.3 Design Decisions and Trade-offs
- **Generic file-based grammar input**: makes the suite reusable across multiple grammars and test cases.
- **Readable output**: tables and traces are formatted into aligned columns to match assignment expectations.
- **Memory usage reporting**: exact STL map allocator overhead is platform-specific; we report table entry counts and a conservative approximate bytes estimate (plus the exact table text files for inspection).

#### 2.4 Handling Lookaheads in LR(1)
Lookaheads are propagated using \(FIRST(\beta a)\) during CLOSURE:
- This ensures reductions only occur when the current input token matches the item’s lookahead.
- This precision often removes shift/reduce conflicts that SLR(1) introduces by using FOLLOW(\(A\)) broadly.

### 3. Challenges
- **Lookahead propagation correctness**: a common bug is mishandling epsilon in FIRST sets, which can cause missing or incorrect LR(1) items. We addressed this by implementing FIRST-of-sequence carefully and ensuring epsilon is handled consistently.
- **Conflict reporting and table formatting**: representing ACTION/GOTO tables clearly required computing stable column sets and printing aligned grids so conflicts could be understood quickly.
- **Testing across grammars**: some grammars require tokenization conventions (space-separated tokens) to match terminals such as `id`, `if`, `then`, `else`.

### 4. Test Cases

#### 4.1 Grammars Tested (at least 3)
- Grammar A: `input/grammar_with_conflict.txt` (LR(1) but not SLR(1))
- Grammar B: `input/expr_grammar.txt`
- Grammar C: `input/epsilon_grammar.txt`
- Grammar D: `input/ambiguous_expr.txt`
- Grammar E: `input/reduce_reduce_conflict.txt`

#### 4.2 Input Strings (at least 5 per grammar)
Add the following (valid + invalid) strings you tested:

- **Grammar A**:
  - 1) `<fill>`
  - 2) `<fill>`
  - 3) `<fill>`
  - 4) `<fill>`
  - 5) `<fill>`
- **Grammar B**:
  - 1) `<fill>`
  - 2) `<fill>`
  - 3) `<fill>`
  - 4) `<fill>`
  - 5) `<fill>`
- **Grammar C**:
  - 1) `<fill>`
  - 2) `<fill>`
  - 3) `<fill>`
  - 4) `<fill>`
  - 5) `<fill>`

#### 4.3 Conflict Demonstration (SLR(1) conflict resolved by LR(1))
Using `input/grammar_with_conflict.txt`:
- **SLR(1)** produces a shift/reduce conflict in the parsing table.
- **LR(1)** produces **0 conflicts** due to lookahead-specific reductions.

(Paste the relevant ACTION cell conflict line(s) here.)

### 5. Comparison Analysis
The program produces `output/comparison.txt` for each run containing:
- Number of states generated (SLR vs LR(1))
- Parsing table size (entry counts + approximate bytes)
- Time to construct parsing tables
- Parsing speed (parse throughput over multiple iterations)

#### 5.1 SLR(1) vs LR(1) Parsing Power
- **SLR(1)** may report conflicts due to FOLLOW-based reductions being too broad.
- **LR(1)** often resolves these conflicts because reductions are restricted by explicit lookaheads.

### 6. Sample Outputs
Add screenshots and paste/attach outputs here:
- **Screenshots of program execution**: `<add here>`
- **Item sets and parsing tables**: `<add here>`
- **Parse trees for accepted strings**: `<add here>`
- **Conflict examples and resolutions**: `<add here>`

### 7. Conclusion
This assignment demonstrates how augmenting grammars, constructing canonical LR item collections, and building ACTION/GOTO tables leads to practical shift-reduce parsers. The LR(1) parser’s lookahead precision provides greater power than SLR(1), typically at the cost of more states and a larger canonical collection.


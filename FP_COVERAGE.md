# FP operation coverage

This library aims to cover the **standard functional-programming operation set**
— the functions you reach for in Haskell's `Prelude`/`Data.List`, Lisp's
sequence library, and ML/OCaml's `List` — with a C API and SIMD-accelerated hot
paths.

**Scope, stated honestly.** "Coverage" here means *operations*, not language
semantics. A linked C library provides functions and data types; it does **not**
provide a type system, lazy-by-default evaluation, syntactic pattern matching,
enforced purity, macros, or higher-kinded types. Those are properties of a
*language* and its compiler (see the last section). Within that scope, the goal
is genuinely full coverage of the operation set — this file tracks how close we
are.

## ✅ Covered today

**Higher-order core** — `map`, `filter`, `foldl`, `foldr`, `foldl1`, `foldr1`,
`zipWith`, `zipWith3`, `mapAccumL`, `mapAccumR`, `zip`, `unzip`
**Scans** — `scanl`, `scanr`, `scanl1`, `scanr1`, prefix-sum kernels (`fp_scan_add_*`)
**Ordering & grouping** — `sortBy` / `sortOn` (stable), `groupBy`, `nubBy`, `intersperse`,
`transpose`
**Reductions / Foldable** — `sum`, `product`, `min`, `max`, `all`, `any`,
`countIf`, `find` (→ `Maybe`), `elem`/`contains`
**Slicing & ordering** — `take`, `drop`, `takeWhile`, `dropWhile`, `span`,
`slice`, `reverse`, `concat`, `replicate`
**Selection** — `partition` (predicate and SIMD `>`-threshold), `unique` (`nub`),
`union`, `intersect`
**List-monad bind** — `concatMap` / `flatMap`
**Generators (anamorphisms)** — `iterate`, `unfoldr`, `range`
**Composition** — `compose`, `pipe`, `flip`, `curry`/partial application, `const`
**Optional / error** — `Maybe` and `Either` with `fmap`, `bind`, `ap`,
`sequence`, `traverse` (Maybe & Either, short-circuiting), `mapMaybe`,
`safeDivide`, `safeAt`, `safeHead`
**Monoidal** — `foldMap` (fold with an explicit empty + combine)
**Lazy** — lazy sequences (`map`/`filter`/`take`/`range`/`iterate`)
**Transducers** — `mapping`, `filtering`, `taking`, composition

## 🔜 Planned — remaining operations toward full coverage

- `zip3` / `zipWith3` over >2 inputs returning pair/tuple types
- `intercalate`, `subsequences` / `permutations`
- `Foldable`/`Functor`/`Applicative`/`Traversable` as explicit dictionaries
  (the C stand-in for type classes)
- `mapM` / `forM` and applicative `traverse` over containers beyond arrays
- More monads via explicit context: `State`, `Reader`, `Writer`, and the list
  monad as a first-class type

## ⛔ Out of scope for a C library (language-level, not operations)

These require a compiler/runtime and are **not** a coverage gap this library can
close — they belong to the language, not the operation set:

- lazy-by-default evaluation and infinite data structures
- Hindley–Milner type inference and enforced purity
- syntactic pattern matching and algebraic data types
- macros / homoiconicity, a REPL, first-class continuations
- higher-kinded types and type classes as language features

When we say "toward full coverage," we mean the operation set above — not
re-implementing Haskell, Lisp, or ML as languages.

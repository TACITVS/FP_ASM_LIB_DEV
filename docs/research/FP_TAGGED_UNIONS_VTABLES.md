# FP Patterns in C: Tagged Unions + Vtables

## Overview

This document explores how **tagged unions** (algebraic data types) and **vtables** (type classes) enable powerful functional programming patterns in C.

---

## 1. Tagged Unions (Algebraic Data Types)

### What We Already Have: Maybe and Either

```c
// Maybe: Optional values
typedef enum { FP_NOTHING, FP_JUST } fp_maybe_tag_t;

typedef struct {
    fp_maybe_tag_t tag;
    union {
        double value_f64;
        int64_t value_i64;
        void* value_ptr;
    };
} Maybe;

// Either: Error handling
typedef enum { FP_LEFT, FP_RIGHT } fp_either_tag_t;

typedef struct {
    fp_either_tag_t tag;
    union {
        struct {
            const char* error_msg;
            int error_code;
        } left;
        struct {
            double value_f64;
            int64_t value_i64;
            void* value_ptr;
        } right;
    };
} Either;
```

### Benefits:
- ✅ Type-safe sum types
- ✅ Exhaustive pattern matching (via switch)
- ✅ Zero runtime overhead (just a tag + union)
- ✅ Compiler can optimize aggressively

---

## 2. More Tagged Unions for FP

### List (Cons Cell)

```c
typedef enum { FP_NIL, FP_CONS } fp_list_tag_t;

typedef struct FPList {
    fp_list_tag_t tag;
    union {
        struct {
            double head;
            struct FPList* tail;
        } cons;
    };
} FPList;

// Example: [1, 2, 3]
FPList* list_123 = &(FPList){
    .tag = FP_CONS,
    .cons = {
        .head = 1.0,
        .tail = &(FPList){
            .tag = FP_CONS,
            .cons = {
                .head = 2.0,
                .tail = &(FPList){
                    .tag = FP_CONS,
                    .cons = {
                        .head = 3.0,
                        .tail = &(FPList){ .tag = FP_NIL }
                    }
                }
            }
        }
    }
};

// Pattern matching
double sum_list(FPList* list) {
    switch (list->tag) {
        case FP_NIL:
            return 0.0;
        case FP_CONS:
            return list->cons.head + sum_list(list->cons.tail);
    }
}
```

### Tree (Binary)

```c
typedef enum { FP_LEAF, FP_NODE } fp_tree_tag_t;

typedef struct FPTree {
    fp_tree_tag_t tag;
    union {
        double leaf_value;
        struct {
            double node_value;
            struct FPTree* left;
            struct FPTree* right;
        } node;
    };
} FPTree;

// Pattern matching
double sum_tree(FPTree* tree) {
    switch (tree->tag) {
        case FP_LEAF:
            return tree->leaf_value;
        case FP_NODE:
            return tree->node.node_value +
                   sum_tree(tree->node.left) +
                   sum_tree(tree->node.right);
    }
}
```

### Validation (Accumulating Errors)

```c
typedef enum { FP_VALID, FP_INVALID } fp_validation_tag_t;

typedef struct {
    fp_validation_tag_t tag;
    union {
        double valid_value;
        struct {
            const char** errors;  // Array of error messages
            size_t error_count;
        } invalid;
    };
} Validation;

// Accumulate all validation errors, not just first one!
Validation validate_user_input(const char* name, int age) {
    const char* errors[10];
    size_t count = 0;

    if (!name || strlen(name) == 0) {
        errors[count++] = "Name cannot be empty";
    }
    if (age < 0 || age > 150) {
        errors[count++] = "Age must be between 0 and 150";
    }

    if (count > 0) {
        return (Validation){
            .tag = FP_INVALID,
            .invalid = { .errors = errors, .error_count = count }
        };
    } else {
        return (Validation){
            .tag = FP_VALID,
            .valid_value = (double)age
        };
    }
}
```

---

## 3. Vtables (Type Classes)

Vtables enable polymorphism - different types implementing the same interface.

### Functor Type Class

```c
// Functor interface
typedef struct {
    void* (*fmap)(void* container, void* (*fn)(void*), void* ctx);
} FunctorVTable;

// Generic functor operations
typedef struct {
    void* data;
    FunctorVTable* vtable;
} Functor;

// Implement Functor for Maybe
void* maybe_fmap(void* container, void* (*fn)(void*), void* ctx) {
    Maybe* m = (Maybe*)container;
    if (m->tag == FP_NOTHING) return m;

    Maybe result = {
        .tag = FP_JUST,
        .value_ptr = fn(m->value_ptr)
    };
    return malloc_copy(&result, sizeof(Maybe));
}

FunctorVTable maybe_functor_vtable = {
    .fmap = maybe_fmap
};

// Implement Functor for List
void* list_fmap(void* container, void* (*fn)(void*), void* ctx) {
    FPList* list = (FPList*)container;
    switch (list->tag) {
        case FP_NIL:
            return &(FPList){ .tag = FP_NIL };
        case FP_CONS:
            return &(FPList){
                .tag = FP_CONS,
                .cons = {
                    .head = *(double*)fn(&list->cons.head),
                    .tail = list_fmap(list->cons.tail, fn, ctx)
                }
            };
    }
}

FunctorVTable list_functor_vtable = {
    .fmap = list_fmap
};

// Now we can write generic code!
void* generic_fmap(Functor f, void* (*fn)(void*)) {
    return f.vtable->fmap(f.data, fn, NULL);
}
```

### Foldable Type Class

```c
// Foldable interface
typedef struct {
    void* (*fold_left)(void* container, void* init,
                       void* (*fn)(void*, void*));
    void* (*fold_right)(void* container, void* init,
                        void* (*fn)(void*, void*));
    size_t (*length)(void* container);
} FoldableVTable;

typedef struct {
    void* data;
    FoldableVTable* vtable;
} Foldable;

// Implement Foldable for arrays
void* array_fold_left(void* container, void* init,
                      void* (*fn)(void*, void*)) {
    // ArrayContainer has: double* data, size_t length
    ArrayContainer* arr = (ArrayContainer*)container;
    void* acc = init;
    for (size_t i = 0; i < arr->length; i++) {
        acc = fn(acc, &arr->data[i]);
    }
    return acc;
}

size_t array_length(void* container) {
    return ((ArrayContainer*)container)->length;
}

FoldableVTable array_foldable_vtable = {
    .fold_left = array_fold_left,
    .fold_right = array_fold_right,
    .length = array_length
};

// Implement Foldable for lists
void* list_fold_left(void* container, void* init,
                     void* (*fn)(void*, void*)) {
    FPList* list = (FPList*)container;
    void* acc = init;
    while (list->tag == FP_CONS) {
        acc = fn(acc, &list->cons.head);
        list = list->cons.tail;
    }
    return acc;
}

FoldableVTable list_foldable_vtable = {
    .fold_left = list_fold_left,
    .fold_right = list_fold_right,
    .length = list_length
};

// Generic fold that works on ANY Foldable!
void* generic_fold(Foldable f, void* init, void* (*fn)(void*, void*)) {
    return f.vtable->fold_left(f.data, init, fn);
}
```

### Monad Type Class

```c
// Monad interface
typedef struct {
    void* (*pure)(void* value);
    void* (*bind)(void* container, void* (*fn)(void*));
} MonadVTable;

typedef struct {
    void* data;
    MonadVTable* vtable;
} Monad;

// Implement Monad for Maybe
void* maybe_pure(void* value) {
    return &(Maybe){ .tag = FP_JUST, .value_ptr = value };
}

void* maybe_bind(void* container, void* (*fn)(void*)) {
    Maybe* m = (Maybe*)container;
    if (m->tag == FP_NOTHING) return m;
    return fn(m->value_ptr);  // fn returns Maybe*
}

MonadVTable maybe_monad_vtable = {
    .pure = maybe_pure,
    .bind = maybe_bind
};

// Implement Monad for Either
void* either_pure(void* value) {
    return &(Either){ .tag = FP_RIGHT, .right = { .value_ptr = value } };
}

void* either_bind(void* container, void* (*fn)(void*)) {
    Either* e = (Either*)container;
    if (e->tag == FP_LEFT) return e;
    return fn(e->right.value_ptr);
}

MonadVTable either_monad_vtable = {
    .pure = either_pure,
    .bind = either_bind
};

// Generic monadic operations
void* generic_bind(Monad m, void* (*fn)(void*)) {
    return m.vtable->bind(m.data, fn);
}
```

---

## 4. Combining Tagged Unions + Vtables

### Generic Container with Type Classes

```c
// A value that can be any type with Functor/Monad/Foldable capabilities
typedef struct {
    void* data;                  // The actual data
    FunctorVTable* functor;      // Functor operations (optional)
    MonadVTable* monad;          // Monad operations (optional)
    FoldableVTable* foldable;    // Foldable operations (optional)
} GenericContainer;

// Example: Maybe with all type classes
GenericContainer make_maybe_container(Maybe m) {
    return (GenericContainer){
        .data = malloc_copy(&m, sizeof(Maybe)),
        .functor = &maybe_functor_vtable,
        .monad = &maybe_monad_vtable,
        .foldable = NULL  // Maybe is not Foldable
    };
}

// Example: List with all type classes
GenericContainer make_list_container(FPList* list) {
    return (GenericContainer){
        .data = list,
        .functor = &list_functor_vtable,
        .monad = &list_monad_vtable,
        .foldable = &list_foldable_vtable
    };
}

// Generic algorithm that works on ANY Monad
void* generic_sequence(GenericContainer* containers, size_t count,
                       MonadVTable* result_monad) {
    // sequence :: [m a] -> m [a]
    // Turns list of monads into monad of list

    void* result = result_monad->pure(NULL);
    for (size_t i = 0; i < count; i++) {
        if (!containers[i].monad) continue;
        // Bind each monad in sequence
        result = result_monad->bind(result, /* ... */);
    }
    return result;
}
```

---

## 5. Performance Considerations

### Tagged Unions:
- ✅ **Zero runtime overhead** - Just a tag (1 byte) + union
- ✅ **Compiler optimizations** - Switch can be optimized to jump table
- ✅ **Cache-friendly** - Small, contiguous memory

### Vtables:
- ⚠️ **Indirection overhead** - Function pointer call (not inlined)
- ⚠️ **Cache miss potential** - Jumping to different code locations
- ✅ **Amortized** - Cost is per-operation, not per-element
- ✅ **Flexibility** - Runtime polymorphism

**Recommendation:** Use vtables for **container-level** operations, not element-level.

### Good:
```c
// Vtable for container-level fold (called once)
double sum = container->vtable->fold(container->data, 0.0, add);
```

### Bad:
```c
// Vtable for every element access (called n times!)
for (size_t i = 0; i < n; i++) {
    element = container->vtable->get(i);  // ❌ Too slow!
}
```

---

## 6. Practical Applications

### 1. Generic Validation Pipeline

```c
typedef struct {
    GenericContainer data;  // Could be Maybe, Either, Validation
    void* (*validate)(void* value, const char* rule);
} Validator;

// Works with ANY monad!
void* validate_user(User user, Validator validator) {
    return validator.data.monad->bind(validator.data.data,
                                      validate_name)
           ->bind(validate_age)
           ->bind(validate_email);
}
```

### 2. Generic Data Transformations

```c
// Transform ANY Foldable container
double* transform_and_collect(Foldable container,
                               double (*transform)(double)) {
    // Map then fold
    size_t len = container.vtable->length(container.data);
    double* result = malloc(len * sizeof(double));

    // Use vtable to iterate
    container.vtable->fold_left(container.data, result, collect);
    return result;
}
```

### 3. Type-Safe State Machines

```c
typedef enum {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_ERROR
} StateMachineTag;

typedef struct {
    StateMachineTag tag;
    union {
        struct { /* idle data */ } idle;
        struct { double progress; } running;
        struct { double saved_progress; } paused;
        struct { const char* error_msg; } error;
    };
    StateMachineVTable* vtable;  // State-specific operations
} StateMachine;

// Each state has different operations!
```

---

## 7. Conclusion

### Tagged Unions Enable:
- ✅ Algebraic data types (Maybe, Either, List, Tree)
- ✅ Type-safe sum types
- ✅ Pattern matching
- ✅ Zero-overhead abstractions

### Vtables Enable:
- ✅ Polymorphism (type classes)
- ✅ Generic algorithms (works on ANY type)
- ✅ Runtime flexibility
- ✅ Interface-based design

### Combined Power:
- ✅ **True FP in C** - Monads, Functors, Foldables
- ✅ **Type safety** - Compiler-checked invariants
- ✅ **Performance** - Optimize where it matters
- ✅ **Flexibility** - Runtime polymorphism when needed

**This is how we bring Haskell/ML-style FP to C!** 🎯

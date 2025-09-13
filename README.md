# BitSmith

**A Domain-Specific Language for Bit Manipulation Tasks**

---

## Description
BitSmith is a high-level **Domain-Specific Language (DSL)** designed to make bit-level operations **simple, readable, and maintainable**.  
It allows developers to perform **masking, slicing, concatenation, and binary operations** without verbose and error-prone low-level code. BitSmith is ideal for **embedded systems, cryptography, hardware simulation, and testing**.

---

## Features
- **High-level abstractions** for bit manipulation  
- **Concise syntax** for masks, slices, and concatenation (`::`)  
- Supports **bitwise operators**: AND (`&`), OR (`|`), XOR (`^`), NOT (`~`)  
- **Functions** and **patterns** for reusable logic  
- Optimized **bit mapping** for fast evaluation  
- **Simulates low-level hardware operations** in a readable way  


## Getting Started

### Example Code

    mask Block32 {
        L: 16;
        R: 16;
    };

    function feistel_round {
        L = feistel_round[Block32.L];   
        R = feistel_round[Block32.R];   
        
        expanded = R[:8] :: R[8:16] :: R[:8];
        mixed = expanded ^ 0x3D;  
    
        out = mixed[:4] :: mixed[4:8] :: mixed[8:12] :: mixed[12:16]; 
    
        newL = R;
        newR = L ^ out[:16];   
        newR = newR :: newL;
        return newR;
    }
    
    function main : 32 {        
        block = 0x12345678;    
        key   = 0x0F0F0F;      
    
        r1 = feistel_round(block);
        r2 = feistel_round(r1);
    
        return r2;
    }

### Requirements
- C++17 or later  
- Standard C++ compiler (g++, clang++)  

### Build
```bash
g++ -std=c++17 main.cpp preprocessor.cpp lexer.cpp parser.cpp semantics.cpp -o dslc

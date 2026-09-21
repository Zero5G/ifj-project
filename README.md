Clang: https://apt.llvm.org/ 
```bash
sudo bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
```

Dokumentace: doxygen graphviz
```bash
sudo apt install doxygen graphviz
```

Unit testing: https://github.com/Snaipe/Criterion
```bash
sudo apt install libcriterion-dev
```

Opravit rozbitý autocomplete:
```bash
sudo apt install bear
rm compile_commands.json
bear -- make
```

Clang Tidy: https://clang.llvm.org/extra/clang-tidy/

Clang Format: https://clang.llvm.org/docs/ClangFormat.html

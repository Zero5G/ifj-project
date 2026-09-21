# IFJ Projekt
Pro kompilaci budeme zatim používat Clang, protože je nejlehčí nainstalovat
nejnovější verzi, na merlinovy mužeme používat `gcc-15`, pokud ho chcete používat
taky, tak si ho musíte kompilovat sami:
[tenhle návod](https://medium.com/@xersendo/moving-to-c-26-how-to-build-and-set-up-gcc-15-1-on-ubuntu-f52cc9173fa0)
mi fungoval na Linux Mint, ale na Ubuntu 24.04 - WSL selhal. Ubuntu 26.04 - WSL by mněla mít `gcc-15`
jako default při stažení `gcc`, takže můžete použít i to.

Clang: https://apt.llvm.org/ 
```bash
sudo bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
```

Dokumentace: doxygen graphviz
```bash
sudo apt install doxygen graphviz
```

Unit testing: [Criterion](https://github.com/Snaipe/Criterion), docs: https://criterion.readthedocs.io/en/master/starter.html
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

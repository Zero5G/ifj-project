# IFJ Projekt
## Kompilace
Pro kompilaci budeme zatim používat Clang, protože je nejlehčí nainstalovat
nejnovější verzi, na merlinovy mužeme používat `gcc-15`, pokud ho chcete používat
taky, tak si ho musíte kompilovat sami:
[tenhle návod](https://medium.com/@xersendo/moving-to-c-26-how-to-build-and-set-up-gcc-15-1-on-ubuntu-f52cc9173fa0)
mi fungoval na Linux Mint, ale na Ubuntu 24.04 - WSL selhal. Ubuntu 26.04 - WSL by mněla mít `gcc-15`
jako default při stažení `gcc`, takže můžete použít i to.

Clang: https://apt.llvm.org/ 
```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 23
rm llvm.sh
```

## Make příkazy
- `all` (bez příkazu) - spustí kompilaci, více [zde](#kompilace)
- `test` - spustí kompilaci & testy, více [zde](#testování)
- `doc` - generace dokumentace z kometářů, více [zde](#dokumentace)
- `clean` - odstraní vše ze složek `build`, `bin` a `doc`
- `run` - spustí hlavní soubor bez parametrů
- `check` - spustí `check-format` a `check-tidy`, více [zde](#formátování)
- `check-format` - kontrola formátování, více [zde](#formátování)
- `check-tidy` - kontrola základních chyb, více [zde](#formátování)
- `format` - formátovaní **VŠECH** souborů `.c` a `.h`, spouštějte jen když víte že formátujete jen svůj kód, více [zde](#formátování)

## Dokumentace
Dokumentace: [docs](https://www.doxygen.nl/manual/docblocks.html)
```bash
sudo apt install doxygen graphviz
```

## Testování
Unit testing: [Criterion](https://github.com/Snaipe/Criterion), docs: https://criterion.readthedocs.io/en/master/starter.html
```bash
sudo apt install libcriterion-dev
```
## Autocomplete
Opravit rozbitý autocomplete:
```bash
sudo apt install bear
rm compile_commands.json
bear -- make
```

## Formátování
Clang Format: https://clang.llvm.org/docs/ClangFormat.html
```bash
sudo apt-get install clang-format
```

Clang Tidy: https://clang.llvm.org/extra/clang-tidy/
```bash
sudo apt-get install clang-tidy
```

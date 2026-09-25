# IFJ Projekt
## Kompilace
Pro kompilaci budeme zatím používat Clang, protože je nejlehčí nainstalovat
nejnovější verzi, na Merlinovi můžeme používat `gcc-15`,
pokud ho chcete používat taky, tak si ho musíte kompilovat sami:
[tenhle návod](https://medium.com/@xersendo/moving-to-c-26-how-to-build-and-set-up-gcc-15-1-on-ubuntu-f52cc9173fa0)
mi fungoval na Linux Mint, ale na Ubuntu 24.04 - WSL selhal. Ubuntu 26.04 - WSL by měla mít `gcc-15`
jako default při stažení `gcc`, takže můžete použít i to.

Clang: https://apt.llvm.org/ 
```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 23
rm llvm.sh
```

## Make příkazy
- `all` (bez příkazu) - spustí `clean` a kompilaci, více [zde](#kompilace)
- `build` (bez příkazu) - spustí kompilaci, více [zde](#kompilace)
- `debug` (bez příkazu) - spustí kompilaci pro debug, více [zde](#kompilace)
- `clean` - odstraní vše ze složek `build`, `bin` a `doc`
- `test` - spustí kompilaci & testy, více [zde](#testování)
- `doc` - generace dokumentace z kometářů, více [zde](#dokumentace)
- `run` - spustí hlavní soubor bez parametrů
- `check` - spustí `check-format` a `check-tidy`, více [zde](#formátování)
- `check-format` - kontrola formátování, více [zde](#formátování)
- `check-tidy` - kontrola základních chyb, více [zde](#formátování)
- `format` - formátovaní **VŠECH** souborů `.c` a `.h`, spouštějte jen když víte že formátujete jen svůj kód, více [zde](#formátování)

> [!NOTE]
> Pokud přecházíte z `build` do `debug` a naopak musíte spustic `clean`, e.g. `make clean debug`

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
1. Instalace programu Build EAR
  ```bash
  sudo apt install bear
  ```
2. Generace/Regenerace `compile_commands.json`
  ```bash
  rm compile_commands.json
  bear -- make clean build
  ```

## Formátování
Clang Format: https://clang.llvm.org/docs/ClangFormat.html
```bash
sudo apt-get install clang-format-23
```

Clang Tidy: https://clang.llvm.org/extra/clang-tidy/
```bash
sudo apt-get install clang-tidy-23
```

## Nastavit GitHub s Gitem
1. Instalace Gitu and GitHub CLI
  ```bash
  sudo apt-get install git gh
  ```
2. Spojení s GitHubem
  ```bash
  gh auth login
  ```
  Pak budete pracovat s interactivním terminálem:
  1. ? What account do you want to log into? `GitHub.com`
  2. ? What is your preferred protocol for Git operations on this host? `HTTPS`
  3. ? How would you like to authenticate GitHub CLI? `Login with a web browser`
  > [!NOTE]
  > Pokud se nepovede otevřít prohlížeč klikňete na odkaz, nebo ho zkopírujte do prohlížeče 
  4. Authenticate Git with your GitHub credentials? (Y/n) `Y`
  > [!NOTE]
  > Pokud na posledním kroku dáte `n`, tak můžete spustit `git auth setup-git`

## Instalace všeho
```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 23
rm llvm.sh

sudo apt install build-essential git gh clang-tidy-23 clang-format-23 clang-tools-23 libcriterion-dev bear doxygen graphviz
```

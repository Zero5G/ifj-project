---
title: "IFJ Projekt 2026/27"
author: "Tým xmanouj00"
date: "November 2026"
geometry: margin=1.8cm
fontsize: 11pt
---

![VUT FIT logo](FIT_barevne_RGB_CZ.png)

# Implementace překladače imperativního jazyka IFJ26. {.unlisted .unnumbered}

Jméno | xLogin | Body
- | - | -
**Jakub Manoušek** | `xmanouj00` | 25%
Jakub Machač | `xmachaj00` | 25%
Matěj Krňák | `xkrnakm00` | 25%
Vojta Buchmaier | `x00` | 25%

- **Zvolená varianta:** `TRP-izp`

- Git repositář: [odkaz](https://github.com/Zero5G/ifj-project) (`https://github.com/Zero5G/ifj-project`)

- Seznam rozšíření: `BOOLTHEN`, `FUNEXP`

- Rozdělení práce:
  - `xmanouj00`: FSM, AST, semantická analýza
  - `xmachaj00`: dynamický řeťezec, generátor kódu
  - `xkrnakm00`: gramatika, 
  - `x00`: tabulka s rozptýlenými položkami, lexikální analýza

\newpage
\renewcommand{\contentsname}{Obsah}
\tableofcontents
\newpage

# Úvod
Projekt byl dělán v jazuku C, standardu `C23`, požívaly jsme kompilátor `clang`, protože byl nejjedušší na instalaci a pracuje s nástroji `clang-tidy` and `clang-format`. Projekt je kompilován za použití programu `Make`.

Pro programovaní nebyl nucený editor, ale byl poskytnut soubor `.editorconfig`.

Styl programovaní jsme zvolili `LLVM` s malou úpravou.

Projekt dokáže generovat referenční manuál pomocí programu `Doxygen`.

Návod na kompilaci projektu (na operačním systémem Ubuntu) je obsáhnu v souboru `README.md` v kořenovém adresáři.

# Návrh a implementace
Cílem projektu je implementovat překladač jazuka `IFJ26`, který je podmnožinou jazyku `Scala`.
Překladač má být implementován v jazyce `C`.
## Lexikální analýza
## Parser
### AST
## Semantická analýza
## Generátor kódu

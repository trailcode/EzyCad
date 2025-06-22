---
trigger: manual
---

# Windsurf C++ Code Formatting Rules

## Class Naming
- Class names use snake_case with an initial capital letter and underscores as needed.
  - Example: `class My_class`

## Braces
- The opening curly brace `{` for classes, functions, and methods appears on the next line (Allman style).
  - Example:
    ```cpp
    class My_class
    {
      // ...
    };
    ```

## Functions and Methods
- Function and method names use snake_case.
  - Example: `void do_something();`

## Variables
- Variable names use snake_case.
  - Example: `int my_variable;`

## Inheritance
- Use a space before the colon in inheritance.
  - Example:
    ```cpp
    class My_child_class : public My_base_class
    {
      // ...
    };
    ```

## Indentation and Spacing
- Use consistent indentation of **two spaces per level**.
- Place a blank line between method definitions for readability.

## Single-Statement Loops and Conditionals
- For loops and conditionals with a single statement, omit the braces and place the statement on the next indented line.
  - Example:
    ```cpp
    for (const auto& shape : shapes_)
      sum += shape->area();

    if (is_ready)
      do_something();
    ```

---

## .clang-format Settings

Use the following [.clang-format](cci:7://file:///c:/src/EzyCad/.clang-format:0:0-0:0) settings to automatically enforce the Windsurf C++ style:

```yaml
---
Language: Cpp
BasedOnStyle: Google
ColumnLimit: 0
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: AcrossComments
AlignConsecutiveDeclarations: AcrossComments
AlignConsecutiveMacros: AcrossComments
AlignEscapedNewlines: Right
AlignOperands: true
AlignTrailingComments: true
AllowAllArgumentsOnNextLine: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: Empty
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AlwaysBreakAfterReturnType: None
AlwaysBreakTemplateDeclarations: No
BinPackArguments: true
BinPackParameters: true
BreakBeforeBraces: Allman
IndentWidth: 2
TabWidth: 2
UseTab: Never
PointerAlignment: Left

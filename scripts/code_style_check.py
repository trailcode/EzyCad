#!/usr/bin/env python3
"""Check EzyCad C++ sources against docs/ezycad_code_style.md.

Start with Vertical rhythm (blank lines). More rule groups can be added later.

Usage:
  python scripts/code_style_check.py [paths...]
  python scripts/code_style_check.py --rule vertical-rhythm src/gui.cpp

Default paths: <repo>/src
Exit 1 if any finding is reported.
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path

CPP_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"}

SKIP_DIR_NAMES = {
    ".git",
    "third_party",
    "node_modules",
    "_build",
    "_deps",
    "build",
    "out",
    ".venv",
    "venv",
}
SKIP_DIR_PREFIXES = ("build-", "cmake-build")

IDENT_START = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_")
IDENT_CONT = IDENT_START | set("0123456789")

TYPE_DEF_STARTS = frozenset({"class", "struct", "enum", "union", "namespace"})
CONTROL_STARTS = frozenset({"if", "for", "while", "switch", "do", "try"})
ACCESS_LABELS = frozenset({"public", "private", "protected", "default"})

SAME_ROW_UI_PREFIXES = ("ImGui::SameLine", "GUI_DOC_HELP_")
IMGUI_TEXT_BEATS = ("ImGui::TextWrapped", "ImGui::TextDisabled")
IMGUI_SPACING = "ImGui::Spacing"
IMGUI_BUTTON = "ImGui::Button"


@dataclass(frozen=True)
class Finding:
    path: Path
    line: int
    rule: str
    message: str

    def format(self, repo: Path) -> str:
        try:
            rel = self.path.resolve().relative_to(repo.resolve())
        except ValueError:
            rel = self.path
        return f"{rel.as_posix()}:{self.line}: [{self.rule}] {self.message}"


@dataclass
class Stmt:
    kind: str
    start: int
    end: int
    has_else: bool = False
    bodies: tuple[tuple[int, int], ...] = ()


class Source:
    def __init__(self, path: Path, text: str):
        self.path = path
        self.text = text
        self.masked = mask_comments_and_strings(text)
        self.lines = text.splitlines()
        self._line_at = build_line_index(text)
        self.off_ranges = clang_format_off_ranges(self.lines)

    def line_of(self, index: int) -> int:
        if index < 0:
            return 1
        if index >= len(self._line_at):
            return self._line_at[-1] if self._line_at else 1
        return self._line_at[index]

    def in_clang_format_off(self, index: int) -> bool:
        line = self.line_of(index)
        return any(a <= line <= b for a, b in self.off_ranges)

    def slice(self, start: int, end: int) -> str:
        return self.masked[start:end]

    def leading_code(self, start: int, end: int) -> str:
        i = skip_ws(self.masked, start, end)
        j = i
        n = min(end, len(self.masked))
        while j < n and self.masked[j] not in " \t\n":
            j += 1
        # Include qualified calls: ImGui::SameLine(
        while j < n and self.masked[j] in ":":
            j += 1
            while j < n and self.masked[j] not in " \t\n(":
                j += 1
        return self.masked[i:j]

    def has_blank_line_between(self, a_end: int, b_start: int) -> bool:
        first = self.line_of(max(0, a_end - 1)) + 1
        last = self.line_of(b_start) - 1
        for ln in range(first, last + 1):
            if 1 <= ln <= len(self.lines) and not self.lines[ln - 1].strip():
                return True
        return False

    def line_is_closer_only(self, stmt_end: int) -> bool:
        """True when the statement ends on a line that is only `};` (multi-line close)."""
        i = stmt_end
        while i > 0 and self.masked[i - 1] in " \t\n":
            i -= 1
        if i < 2 or self.masked[i - 2 : i] != "};":
            return False
        line = self.line_of(i - 1)
        raw = self.lines[line - 1] if 1 <= line <= len(self.lines) else ""
        return raw.strip() == "};"


def build_line_index(text: str) -> list[int]:
    line = 1
    out = [1] * len(text)
    for i, ch in enumerate(text):
        out[i] = line
        if ch == "\n":
            line += 1
    return out


def clang_format_off_ranges(lines: list[str]) -> list[tuple[int, int]]:
    ranges: list[tuple[int, int]] = []
    off_at: int | None = None
    for i, line in enumerate(lines, start=1):
        stripped = line.strip()
        if stripped == "// clang-format off":
            off_at = i
        elif stripped == "// clang-format on" and off_at is not None:
            ranges.append((off_at, i))
            off_at = None
    if off_at is not None:
        ranges.append((off_at, len(lines)))
    return ranges


def mask_comments_and_strings(text: str) -> str:
    """Replace comments and string/char contents with spaces; keep newlines and code."""
    out: list[str] = []
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        if ch == "/" and nxt == "/":
            while i < n and text[i] != "\n":
                out.append(" ")
                i += 1
            continue
        if ch == "/" and nxt == "*":
            out.append("  ")
            i += 2
            while i < n - 1 and not (text[i] == "*" and text[i + 1] == "/"):
                out.append("\n" if text[i] == "\n" else " ")
                i += 1
            if i < n - 1:
                out.append("  ")
                i += 2
            elif i < n:
                out.append(" ")
                i += 1
            continue

        if ch == "R" and nxt == '"':
            i = _mask_raw_string(text, i, out)
            continue

        if ch in "\"'":
            quote = ch
            out.append(" ")
            i += 1
            while i < n and text[i] != quote:
                if text[i] == "\\":
                    out.append("\n" if text[i] == "\n" else " ")
                    i += 1
                    if i < n:
                        out.append("\n" if text[i] == "\n" else " ")
                        i += 1
                    continue
                out.append("\n" if text[i] == "\n" else " ")
                i += 1
            if i < n:
                out.append(" ")
                i += 1
            continue

        out.append(ch)
        i += 1
    return "".join(out)


def _mask_raw_string(text: str, i: int, out: list[str]) -> int:
    # R"delim( ... )delim"
    n = len(text)
    out.append(" ")  # R
    i += 1
    out.append(" ")  # "
    i += 1
    delim: list[str] = []
    while i < n and text[i] != "(":
        delim.append(text[i])
        out.append(" ")
        i += 1
    if i < n:
        out.append(" ")
        i += 1
    close = ")" + "".join(delim) + '"'
    while i < n:
        if text.startswith(close, i):
            for _ in close:
                out.append(" ")
                i += 1
            return i
        out.append("\n" if text[i] == "\n" else " ")
        i += 1
    return i


def skip_ws(masked: str, i: int, end: int) -> int:
    n = min(end, len(masked))
    while i < n and masked[i] in " \t\n\r\f\v":
        i += 1
    return i


def skip_ws_and_pp(masked: str, i: int, end: int) -> int:
    n = min(end, len(masked))
    while i < n:
        i = skip_ws(masked, i, end)
        if i >= n:
            break
        if masked[i] == "#" and _at_line_start(masked, i):
            while i < n and masked[i] != "\n":
                i += 1
            continue
        break
    return i


def _at_line_start(masked: str, i: int) -> bool:
    j = i - 1
    while j >= 0 and masked[j] in " \t":
        j -= 1
    return j < 0 or masked[j] == "\n"


def peek_ident(masked: str, i: int, end: int) -> str:
    n = min(end, len(masked))
    if i >= n or masked[i] not in IDENT_START:
        return ""
    j = i + 1
    while j < n and masked[j] in IDENT_CONT:
        j += 1
    return masked[i:j]


def skip_ident(masked: str, i: int, end: int) -> int:
    ident = peek_ident(masked, i, end)
    return i + len(ident)


def skip_balanced_angles(masked: str, i: int, end: int) -> int:
    n = min(end, len(masked))
    if i >= n or masked[i] != "<":
        return i
    depth = 0
    paren = 0
    while i < n:
        ch = masked[i]
        if ch == "(":
            paren += 1
        elif ch == ")":
            paren = max(0, paren - 1)
        elif ch == "<" and paren == 0:
            depth += 1
        elif ch == ">" and paren == 0:
            depth -= 1
            i += 1
            if depth == 0:
                return i
            continue
        i += 1
    return i


def skip_decl_prefix(masked: str, i: int, end: int) -> int:
    """Skip `template <...>` / `template <>` and `[[attribute]]` prefixes."""
    while True:
        i = skip_ws_and_pp(masked, i, end)
        ident = peek_ident(masked, i, end)
        if ident == "template":
            i = skip_ident(masked, i, end)
            i = skip_ws(masked, i, end)
            if i < end and masked[i] == "<":
                i = skip_balanced_angles(masked, i, end)
            continue
        if i < end and masked[i] == "[" and i + 1 < end and masked[i + 1] == "[":
            depth = 0
            while i < end:
                if masked[i] == "[" and i + 1 < end and masked[i + 1] == "[":
                    depth += 1
                    i += 2
                    continue
                if masked[i] == "]" and i + 1 < end and masked[i + 1] == "]":
                    depth -= 1
                    i += 2
                    if depth == 0:
                        break
                    continue
                i += 1
            continue
        return i


def skip_balanced(masked: str, i: int, end: int, open_ch: str, close_ch: str) -> int:
    n = min(end, len(masked))
    if i >= n or masked[i] != open_ch:
        return i
    depth = 0
    while i < n:
        ch = masked[i]
        if ch == open_ch:
            depth += 1
        elif ch == close_ch:
            depth -= 1
            i += 1
            if depth == 0:
                return i
            continue
        i += 1
    return i


def skip_labels(masked: str, i: int, end: int) -> int:
    while True:
        j = skip_ws_and_pp(masked, i, end)
        ident = peek_ident(masked, j, end)
        if not ident:
            return j
        k = skip_ident(masked, j, end)
        k = skip_ws(masked, k, end)
        if ident == "case":
            paren = 0
            while k < end:
                ch = masked[k]
                if ch == "(":
                    paren += 1
                elif ch == ")":
                    paren -= 1
                elif ch == ":" and paren == 0 and (k + 1 >= end or masked[k + 1] != ":"):
                    i = k + 1
                    break
                k += 1
            else:
                return j
            continue
        if ident in ACCESS_LABELS and k < end and masked[k] == ":" and (k + 1 >= end or masked[k + 1] != ":"):
            i = k + 1
            continue
        return j


def parse_if(src: Source, i: int, end: int) -> tuple[Stmt, int]:
    start = i
    i = skip_ident(src.masked, i, end)
    i = skip_ws(src.masked, i, end)
    if i < end and src.masked[i] == "(":
        i = skip_balanced(src.masked, i, end, "(", ")")
    i = skip_ws_and_pp(src.masked, i, end)
    then_start = i
    i = skip_statement(src, i, end)
    then_end = i
    bodies = [(then_start, then_end)]
    has_else = False

    j = skip_ws_and_pp(src.masked, i, end)
    if peek_ident(src.masked, j, end) == "else":
        has_else = True
        j = skip_ident(src.masked, j, end)
        j = skip_ws_and_pp(src.masked, j, end)
        else_start = j
        if peek_ident(src.masked, j, end) == "if":
            _, j = parse_if(src, j, end)
        else:
            j = skip_statement(src, j, end)
        i = j
        bodies.append((else_start, i))

    return Stmt("if", start, i, has_else=has_else, bodies=tuple(bodies)), i


def skip_statement(src: Source, i: int, end: int) -> int:
    i = skip_ws_and_pp(src.masked, i, end)
    if i >= end:
        return i

    ident = peek_ident(src.masked, i, end)
    if ident == "if":
        _, i = parse_if(src, i, end)
        return i
    if ident in ("for", "while", "switch"):
        i = skip_ident(src.masked, i, end)
        i = skip_ws(src.masked, i, end)
        if i < end and src.masked[i] == "(":
            i = skip_balanced(src.masked, i, end, "(", ")")
        i = skip_ws_and_pp(src.masked, i, end)
        return skip_statement(src, i, end)
    if ident == "do":
        i = skip_ident(src.masked, i, end)
        i = skip_ws_and_pp(src.masked, i, end)
        i = skip_statement(src, i, end)
        i = skip_ws_and_pp(src.masked, i, end)
        if peek_ident(src.masked, i, end) == "while":
            i = skip_ident(src.masked, i, end)
            i = skip_ws(src.masked, i, end)
            if i < end and src.masked[i] == "(":
                i = skip_balanced(src.masked, i, end, "(", ")")
            i = skip_ws(src.masked, i, end)
            if i < end and src.masked[i] == ";":
                i += 1
        return i
    if ident == "try":
        i = skip_ident(src.masked, i, end)
        i = skip_ws_and_pp(src.masked, i, end)
        i = skip_statement(src, i, end)
        while True:
            j = skip_ws_and_pp(src.masked, i, end)
            if peek_ident(src.masked, j, end) != "catch":
                break
            j = skip_ident(src.masked, j, end)
            j = skip_ws(src.masked, j, end)
            if j < end and src.masked[j] == "(":
                j = skip_balanced(src.masked, j, end, "(", ")")
            j = skip_ws_and_pp(src.masked, j, end)
            i = skip_statement(src, j, end)
        return i
    if ident == "else":
        # Orphan else (should be consumed by parse_if). Skip its body so we do not stall.
        i = skip_ident(src.masked, i, end)
        i = skip_ws_and_pp(src.masked, i, end)
        return skip_statement(src, i, end)

    if src.masked[i] == "{":
        return skip_balanced(src.masked, i, end, "{", "}")

    return skip_generic_statement(src, i, end)


def skip_generic_statement(src: Source, i: int, end: int) -> int:
    head = skip_decl_prefix(src.masked, i, end)
    ident = peek_ident(src.masked, head, end)
    is_type_def = ident in TYPE_DEF_STARTS
    paren = 0
    bracket = 0
    saw_eq = False

    n = min(end, len(src.masked))
    while i < n:
        ch = src.masked[i]
        if ch in " \t\r\f\v":
            i += 1
            continue
        if ch == "\n":
            i += 1
            i = skip_ws_and_pp(src.masked, i, end)
            continue
        if ch == "#" and _at_line_start(src.masked, i):
            while i < n and src.masked[i] != "\n":
                i += 1
            continue
        if ch == "=" and paren == 0 and bracket == 0:
            i, is_assign = skip_equals_token(src.masked, i, n)
            if is_assign:
                saw_eq = True
            continue
        if ch == "(":
            paren += 1
            i += 1
            continue
        if ch == ")":
            paren = max(0, paren - 1)
            i += 1
            continue
        if ch == "[":
            bracket += 1
            i += 1
            continue
        if ch == "]":
            bracket = max(0, bracket - 1)
            i += 1
            continue
        if ch == "{":
            if paren == 0 and bracket == 0:
                close_as_fn = is_type_def or (not saw_eq and _opens_function_body(src.masked, i))
                i = skip_balanced(src.masked, i, end, "{", "}")
                if close_as_fn:
                    i = skip_ws(src.masked, i, end)
                    if i < n and src.masked[i] == ";":
                        i += 1
                    return i
                continue
            i = skip_balanced(src.masked, i, end, "{", "}")
            continue
        if ch == ";" and paren == 0 and bracket == 0:
            return i + 1
        if ch == "}" and paren == 0 and bracket == 0:
            return i
        i += 1
    return i


def _prev_non_ws(masked: str, i: int, end: int) -> str:
    j = i - 1
    while j >= 0 and j < end and masked[j] in " \t\n\r":
        j -= 1
    if j < 0:
        return ""
    return masked[j]


def parse_statements(src: Source, start: int, end: int) -> list[Stmt]:
    stmts: list[Stmt] = []
    i = start
    guard = 0
    n = min(end, len(src.masked))
    while True:
        guard += 1
        if guard > n + 8:
            break
        i = skip_labels(src.masked, i, end)
        if i >= end:
            break
        if src.masked[i] == "}":
            break
        stmt_start = i
        head = skip_decl_prefix(src.masked, i, end)
        ident = peek_ident(src.masked, head, end)
        if ident == "if":
            stmt, i = parse_if(src, head, end)
            stmts.append(Stmt(stmt.kind, stmt_start, stmt.end, has_else=stmt.has_else, bodies=stmt.bodies))
            if i <= stmt_start:
                i += 1
            continue
        if ident in CONTROL_STARTS:
            body_hint = head
            i = skip_statement(src, head, end)
            stmts.append(Stmt(ident, stmt_start, i, bodies=_control_bodies(src, body_hint, i)))
            if i <= stmt_start:
                i += 1
            continue
        if src.masked[i] == "{":
            close = skip_balanced(src.masked, i, end, "{", "}")
            stmts.append(Stmt("compound", i, close, bodies=((i + 1, close - 1 if close > i else close),)))
            i = close
            continue
        i = skip_generic_statement(src, i, end)
        if i <= stmt_start:
            i += 1
            continue
        kind = "typedef" if ident in TYPE_DEF_STARTS else "stmt"
        inner: tuple[tuple[int, int], ...] = ()
        if kind == "typedef" or _looks_like_function_body(src, stmt_start, i):
            inner = _outer_brace_span(src, stmt_start, i)
        stmts.append(Stmt(kind, stmt_start, i, bodies=inner))
    return stmts


def _control_bodies(src: Source, start: int, end: int) -> tuple[tuple[int, int], ...]:
    i = skip_ident(src.masked, start, end)
    i = skip_ws(src.masked, i, end)
    if i < end and src.masked[i] == "(":
        i = skip_balanced(src.masked, i, end, "(", ")")
    i = skip_ws_and_pp(src.masked, i, end)
    if i < end and src.masked[i] == "{":
        close = skip_balanced(src.masked, i, end, "{", "}")
        return ((i + 1, close - 1),)
    return ((i, end),)


def skip_equals_token(masked: str, i: int, n: int) -> tuple[int, bool]:
    """Advance past `=` / `==` / `!=` / `<=` / `>=` / `<=>`. True if assignment `=`."""
    nxt = masked[i + 1] if i + 1 < n else ""
    prev = masked[i - 1] if i > 0 else ""
    if nxt == ">":
        return i + 2, False
    if nxt == "=":
        return i + 2, False
    if prev in "!<>=":
        return i + 1, False
    return i + 1, True


FUNC_TRAIL_IDENTS = frozenset({"const", "noexcept", "override", "final", "volatile", "mutable"})


def _opens_function_body(masked: str, brace_at: int) -> bool:
    """True if `{` starts a function/method body (not a brace-init)."""
    prev = _prev_non_ws(masked, brace_at, len(masked))
    if prev == ")":
        return True
    ident = _prev_ident(masked, brace_at)
    return ident in FUNC_TRAIL_IDENTS


def _prev_ident(masked: str, i: int) -> str:
    j = i - 1
    while j >= 0 and masked[j] in " \t\n\r":
        j -= 1
    if j < 0 or masked[j] not in IDENT_CONT:
        return ""
    end = j + 1
    while j >= 0 and masked[j] in IDENT_CONT:
        j -= 1
    return masked[j + 1 : end]


def _prefix_has_assignment(masked: str, start: int, brace_at: int) -> bool:
    i = start
    paren = 0
    bracket = 0
    n = brace_at
    while i < n:
        ch = masked[i]
        if ch == "(":
            paren += 1
        elif ch == ")":
            paren = max(0, paren - 1)
        elif ch == "[":
            bracket += 1
        elif ch == "]":
            bracket = max(0, bracket - 1)
        elif ch == "=" and paren == 0 and bracket == 0:
            i, is_assign = skip_equals_token(masked, i, n)
            if is_assign:
                return True
            continue
        i += 1
    return False


def _looks_like_function_body(src: Source, start: int, end: int) -> bool:
    brace_at = src.masked.find("{", start, end)
    if brace_at < 0:
        return False
    if _prefix_has_assignment(src.masked, start, brace_at):
        return False
    i = start
    last_rparen = -1
    paren = 0
    while i < end:
        ch = src.masked[i]
        if ch == "(":
            paren += 1
        elif ch == ")":
            paren -= 1
            if paren == 0:
                last_rparen = i
        elif ch == "{":
            if last_rparen >= 0 and paren == 0:
                return True
            return False
        i += 1
    return False


def _outer_brace_span(src: Source, start: int, end: int) -> tuple[tuple[int, int], ...]:
    i = start
    paren = 0
    while i < end:
        ch = src.masked[i]
        if ch == "(":
            paren += 1
        elif ch == ")":
            paren -= 1
        elif ch == "{" and paren == 0:
            close = skip_balanced(src.masked, i, end, "{", "}")
            if close > i + 1:
                return ((i + 1, close - 1),)
            return ()
        i += 1
    return ()


def starts_with_any(code: str, prefixes: tuple[str, ...]) -> bool:
    return any(code.startswith(p) for p in prefixes)


def check_vertical_rhythm(src: Source) -> list[Finding]:
    findings: list[Finding] = []
    _check_span(src, 0, len(src.masked), findings)
    return findings


def _check_span(src: Source, start: int, end: int, findings: list[Finding]) -> None:
    if start >= end:
        return
    stmts = parse_statements(src, start, end)
    for stmt in stmts:
        for b0, b1 in stmt.bodies:
            _check_span(src, b0, b1, findings)

    for a, b in zip(stmts, stmts[1:]):
        if src.in_clang_format_off(a.start) or src.in_clang_format_off(b.start):
            continue
        if src.has_blank_line_between(a.end, b.start):
            continue

        b_lead = src.leading_code(b.start, b.end)
        a_lead = src.leading_code(a.start, a.end)
        line = src.line_of(b.start)

        if a.kind == "if" and not a.has_else and b.kind == "if":
            findings.append(
                Finding(
                    src.path,
                    line,
                    "vertical-rhythm",
                    "blank line required between consecutive if statements that have no else",
                )
            )
            continue

        if a.kind == "if" and starts_with_any(b_lead, SAME_ROW_UI_PREFIXES):
            findings.append(
                Finding(
                    src.path,
                    line,
                    "vertical-rhythm",
                    "blank line required after if before same-row UI (ImGui::SameLine / GUI_DOC_HELP_)",
                )
            )
            continue

        if src.line_is_closer_only(a.end) and a.kind not in TYPE_DEF_STARTS and a.kind != "typedef":
            findings.append(
                Finding(
                    src.path,
                    line,
                    "vertical-rhythm",
                    "blank line required after local lambda / helper / lookup table close (};)",
                )
            )
            continue

        if starts_with_any(a_lead, IMGUI_TEXT_BEATS) and b_lead.startswith(IMGUI_SPACING):
            findings.append(
                Finding(
                    src.path,
                    line,
                    "vertical-rhythm",
                    "blank line required between ImGui text beat (TextWrapped / TextDisabled) and Spacing",
                )
            )
            continue

        if a_lead.startswith(IMGUI_BUTTON) and (
            b_lead.startswith(IMGUI_BUTTON) or b_lead.startswith("ImGui::SameLine")
        ):
            findings.append(
                Finding(
                    src.path,
                    line,
                    "vertical-rhythm",
                    "blank line required between sequential ImGui Button / SameLine actions",
                )
            )


def iter_cpp_files(paths: list[Path]) -> list[Path]:
    files: list[Path] = []
    for path in paths:
        path = path.resolve()
        if path.is_file():
            if path.suffix.lower() in CPP_SUFFIXES:
                files.append(path)
            continue
        if not path.is_dir():
            continue
        for child in path.rglob("*"):
            if not child.is_file() or child.suffix.lower() not in CPP_SUFFIXES:
                continue
            if any(p.name in SKIP_DIR_NAMES or p.name.startswith(SKIP_DIR_PREFIXES) for p in child.parents):
                continue
            files.append(child)
    return sorted(set(files))


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


RULES = {
    "vertical-rhythm": check_vertical_rhythm,
}


def collect_findings(files: list[Path], rule_names: list[str] | None = None) -> list[Finding]:
    names = rule_names or list(RULES)
    findings: list[Finding] = []
    for path in files:
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            text = path.read_text(encoding="utf-8", errors="replace")
        src = Source(path, text)
        for name in names:
            findings.extend(RULES[name](src))
    findings.sort(key=lambda f: (str(f.path).lower(), f.line, f.message))
    return findings


def main() -> int:
    root = repo_root()
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "paths",
        nargs="*",
        type=Path,
        default=[root / "src"],
        help="Files or directories (default: src/)",
    )
    ap.add_argument(
        "--rule",
        action="append",
        choices=sorted(RULES),
        dest="rules",
        help="Rule group to run (repeatable). Default: all implemented groups",
    )
    args = ap.parse_args()
    files = iter_cpp_files(args.paths)
    if not files:
        print("No C/C++ files found.", file=sys.stderr)
        return 2

    findings = collect_findings(files, args.rules)
    for f in findings:
        print(f.format(root))

    if findings:
        print(f"{len(findings)} finding(s) in {len({f.path for f in findings})} file(s).")
        return 1
    print("No code-style findings.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Generate PIMPL forwarding stubs from a C++ interface header.

Reads public method declarations from a class/struct and writes an .inl file
with out-of-line definitions that delegate to m_impl->method(...).

Example:
  python tools/pimpl_gen.py src/sketch_op_recorder.h --class Sketch_op_recorder
  -> sketch_op_recorder_impl_000.inl

The output is a starting point: review, rename, and merge into your .cpp.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path


def definition_suffix(suffix: str) -> str:
    """Suffix tokens valid on a declaration but not on an out-of-line definition."""
    suffix = re.sub(r"\boverride\b", "", suffix)
    suffix = re.sub(r"\bfinal\b", "", suffix)
    suffix = re.sub(r"=\s*0\b", "", suffix)
    return normalize_ws(suffix)


@dataclass
class MethodDecl:
    return_type: str
    name: str
    params: str
    suffix: str  # const, override, noexcept, etc. after the closing paren

    @property
    def definition_suffix(self) -> str:
        return definition_suffix(self.suffix)

    @property
    def qualified_signature_tail(self) -> str:
        """Everything after ClassName:: in a definition."""
        parts = [f"({self.params})"]
        if self.definition_suffix:
            parts.append(self.definition_suffix)
        return " ".join(parts)

    @property
    def forward_args(self) -> str:
        return ", ".join(extract_param_names(self.params))


def strip_comments(text: str) -> str:
    # Block comments
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    # Line comments
    text = re.sub(r"//[^\n]*", "", text)
    return text


def find_matching_brace(text: str, open_index: int) -> int:
    depth = 0
    i = open_index
    while i < len(text):
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("unbalanced braces in class body")


def split_top_level(text: str, sep: str) -> list[str]:
    parts: list[str] = []
    depth_angle = depth_paren = depth_bracket = 0
    start = 0
    for i, ch in enumerate(text):
        if ch == "<":
            depth_angle += 1
        elif ch == ">":
            depth_angle = max(0, depth_angle - 1)
        elif ch == "(":
            depth_paren += 1
        elif ch == ")":
            depth_paren = max(0, depth_paren - 1)
        elif ch == "[":
            depth_bracket += 1
        elif ch == "]":
            depth_bracket = max(0, depth_bracket - 1)
        elif ch == sep and depth_angle == depth_paren == depth_bracket == 0:
            parts.append(text[start:i].strip())
            start = i + 1
    parts.append(text[start:].strip())
    return [p for p in parts if p]


def extract_param_names(params: str) -> list[str]:
    if not params.strip():
        return []
    names: list[str] = []
    for part in split_top_level(params, ","):
        part = part.strip()
        if not part or part == "...":
            continue
        # Drop default value
        if "=" in part:
            part = part[: part.index("=")].strip()
        # Name is the last token (handles Type& name, Type* name, Type name)
        tokens = part.split()
        if not tokens:
            continue
        name = tokens[-1]
        # Strip *, &, && from the name side
        name = name.lstrip("*&")
        if name:
            names.append(name)
    return names


def normalize_ws(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def is_skipped_member(name: str, class_name: str, decl: str) -> bool:
    if name == class_name:
        return True  # constructor
    if name.startswith("~"):
        return True  # destructor
    if "= delete" in decl or "= default" in decl:
        return True
    if decl.strip().endswith("{") or "{" in decl:
        return True  # inline body in header
    if name.startswith("operator"):
        return True
    return False


def parse_method_decl(decl: str, class_name: str) -> MethodDecl | None:
    decl = normalize_ws(decl)
    if not decl.endswith(";"):
        return None
    decl = decl[:-1].strip()

    # friend, using, enum, nested type, static data member
    skip_prefixes = (
        "friend ",
        "using ",
        "enum ",
        "struct ",
        "class ",
        "typedef ",
        "static_assert",
    )
    if decl.startswith(skip_prefixes):
        return None

    paren = decl.find("(")
    if paren < 0:
        return None

    before = decl[:paren].strip()
    after = decl[paren + 1 :]
    close = find_matching_paren(after, 0, already_open=True)
    if close < 0:
        return None
    params = after[:close].strip()
    suffix = after[close + 1 :].strip()

    # static member function
    if before.startswith("static "):
        before = before[7:].strip()

    name_match = re.search(r"([~]?[\w:]+)\s*$", before)
    if not name_match:
        return None
    name = name_match.group(1)
    return_type = before[: name_match.start()].strip()

    if is_skipped_member(name, class_name, decl + ";"):
        return None
    if not return_type and name != class_name:
        # Might be a ctor we missed, or malformed
        if name == class_name:
            return None

    return MethodDecl(return_type=return_type, name=name, params=params, suffix=suffix)


def find_matching_paren(text: str, start: int, *, already_open: bool = False) -> int:
    depth = 1 if already_open else 0
    depth_angle = 0
    i = start
    while i < len(text):
        ch = text[i]
        if ch == "<":
            depth_angle += 1
        elif ch == ">":
            depth_angle = max(0, depth_angle - 1)
        elif ch == "(" and depth_angle == 0:
            depth += 1
        elif ch == ")" and depth_angle == 0:
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def class_body_span(text: str, match: re.Match[str]) -> tuple[int, int, str] | None:
    """Return (open_brace, close_brace, kind) or None for forward declarations."""
    kind = match.group(1)
    tail = text[match.end() :]
    semi = tail.find(";")
    brace = tail.find("{")
    if brace < 0 or (semi >= 0 and semi < brace):
        return None
    open_brace = match.end() + brace
    close_brace = find_matching_brace(text, open_brace)
    return open_brace, close_brace, kind


def extract_class_body(text: str, class_name: str) -> tuple[str, str]:
    pattern = re.compile(rf"\b(class|struct)\s+{re.escape(class_name)}\b")
    match = pattern.search(text)
    if not match:
        raise ValueError(f"class/struct '{class_name}' not found")

    span = class_body_span(text, match)
    if not span:
        raise ValueError(f"no body for '{class_name}' (forward declaration?)")
    open_brace, close_brace, kind = span
    return text[open_brace + 1 : close_brace], kind


def strip_leading_access(chunk: str) -> tuple[str, str | None]:
    m = re.match(r"^(public|protected|private)\s*:\s*", chunk)
    if not m:
        return chunk, None
    return chunk[m.end() :].strip(), m.group(1)


def collect_public_methods(class_body: str, class_name: str, kind: str) -> list[MethodDecl]:
    default_public = kind == "struct"
    access = "public" if default_public else "private"
    methods: list[MethodDecl] = []

    chunks = re.split(r";\s*", class_body)
    for chunk in chunks:
        chunk = normalize_ws(chunk)
        if not chunk:
            continue

        chunk, maybe_access = strip_leading_access(chunk)
        if maybe_access:
            access = maybe_access
        if not chunk:
            continue

        access_only = re.fullmatch(r"(public|protected|private)", chunk)
        if access_only:
            access = access_only.group(1)
            continue

        if access != "public":
            continue

        # Nested type forward declarations / definitions
        if re.match(r"(class|struct)\s+\w+\s*$", chunk):
            continue
        if "{" in chunk:
            continue

        method = parse_method_decl(chunk + ";", class_name)
        if method:
            methods.append(method)

    return methods


def render_method(class_name: str, method: MethodDecl, impl_var: str) -> str:
    sig = f"{method.return_type} {class_name}::{method.name}{method.qualified_signature_tail}".strip()
    args = method.forward_args
    call = f"{impl_var}->{method.name}({args})"

    if not method.return_type or method.return_type == "void":
        return f"{sig}\n{{\n  {call};\n}}"

    return f"{sig}\n{{\n  return {call};\n}}"


def default_out_path(header: Path, index: int = 0) -> Path:
    stem = header.stem
    return header.parent / f"{stem}_impl_{index:03d}.inl"


def generate_inl(
    header_path: Path,
    class_name: str,
    impl_var: str = "m_impl",
    out_path: Path | None = None,
) -> str:
    raw = header_path.read_text(encoding="utf-8")
    text = strip_comments(raw)
    body, kind = extract_class_body(text, class_name)
    methods = collect_public_methods(body, class_name, kind)

    if not methods:
        raise ValueError(f"no public methods found for '{class_name}' in {header_path}")

    out_path = out_path or default_out_path(header_path)
    lines = [
        f"// Generated by tools/pimpl_gen.py from {header_path.name}",
        f"// Class: {class_name}  impl: {impl_var}",
        "// Review and merge into your .cpp; not included automatically.",
        "",
    ]

    for method in methods:
        lines.append(render_method(class_name, method, impl_var))
        lines.append("")

    content = "\n".join(lines).rstrip() + "\n"
    out_path.write_text(content, encoding="utf-8", newline="\n")
    return str(out_path)


def list_pimpl_classes(text: str) -> list[str]:
    text = strip_comments(text)
    names: list[str] = []
    for match in re.finditer(r"\b(class|struct)\s+(\w+)", text):
        name = match.group(2)
        span = class_body_span(text, match)
        if not span:
            continue
        open_brace, close_brace, _ = span
        body = text[open_brace + 1 : close_brace]
        if re.search(rf"\bstd::unique_ptr\s*<\s*Impl\s*>\s+m_impl\b", body):
            names.append(name)
    return names


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate PIMPL forwarding .inl stubs from a C++ header."
    )
    parser.add_argument("header", type=Path, help="Interface .h file")
    parser.add_argument(
        "--class",
        dest="class_name",
        help="Class/struct name (default: first type with unique_ptr<Impl> m_impl)",
    )
    parser.add_argument(
        "--impl-var",
        default="m_impl",
        help="Implementation pointer member name (default: m_impl)",
    )
    parser.add_argument(
        "--out",
        type=Path,
        help="Output .inl path (default: <header_stem>_impl_000.inl next to header)",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List classes in the header that declare unique_ptr<Impl> m_impl",
    )
    args = parser.parse_args()

    if not args.header.is_file():
        print(f"error: not a file: {args.header}", file=sys.stderr)
        return 1

    raw = args.header.read_text(encoding="utf-8")
    text = strip_comments(raw)

    if args.list:
        for name in list_pimpl_classes(text):
            print(name)
        return 0

    class_name = args.class_name
    if not class_name:
        candidates = list_pimpl_classes(text)
        if not candidates:
            print(
                "error: no class with 'unique_ptr<Impl> m_impl' found; pass --class",
                file=sys.stderr,
            )
            return 1
        if len(candidates) > 1:
            print(
                "error: multiple PIMPL classes found; pass --class:\n  "
                + "\n  ".join(candidates),
                file=sys.stderr,
            )
            return 1
        class_name = candidates[0]

    try:
        out = generate_inl(args.header, class_name, args.impl_var, args.out)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

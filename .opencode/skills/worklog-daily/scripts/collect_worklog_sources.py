import argparse
import json
import os
import re
import subprocess
from datetime import datetime
from pathlib import Path

DATE_RE = re.compile(r"^\s*#{1,6}\s*(?P<m>\d{1,2})\.(?P<d>\d{1,2})\b")


def iter_md_files(root: Path):
    for dirpath, dirnames, filenames in os.walk(root):
        # skip .git
        dirnames[:] = [d for d in dirnames if d != ".git"]
        for name in filenames:
            if name.lower().endswith(".md"):
                yield Path(dirpath) / name


def split_by_date(md_text: str):
    lines = md_text.splitlines()
    chunks = []
    current_date = None
    current_lines = []

    def flush():
        if current_date and current_lines:
            chunks.append((current_date, "\n".join(current_lines).strip()))

    for line in lines:
        m = DATE_RE.match(line)
        if m:
            flush()
            current_date = (int(m.group("m")), int(m.group("d")))
            current_lines = [line]
        else:
            if current_date:
                current_lines.append(line)
    flush()
    return chunks


def collect_git(root: Path):
    try:
        out = subprocess.check_output(
            ["git", "log", "--date=short", "--pretty=format:%ad|%h|%s"],
            cwd=str(root),
            text=True,
            stderr=subprocess.DEVNULL,
        )
    except Exception:
        return {}
    commits = {}
    for line in out.splitlines():
        if not line.strip():
            continue
        date, h, msg = line.split("|", 2)
        commits.setdefault(date, []).append({"hash": h, "subject": msg})
    return commits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=".")
    ap.add_argument("--year", type=int, default=datetime.now().year)
    args = ap.parse_args()

    root = Path(args.repo).resolve()
    by_date = {}

    for md_path in iter_md_files(root):
        try:
            text = md_path.read_text(encoding="utf-8")
        except Exception:
            continue
        for (m, d), content in split_by_date(text):
            date = f"{args.year:04d}-{m:02d}-{d:02d}"
            by_date.setdefault(date, []).append({
                "source": str(md_path.relative_to(root)),
                "content": content,
            })

    data = {
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "year_assumed": args.year,
        "dates": by_date,
        "git": collect_git(root),
    }

    out_path = root / "worklog_sources.json"
    out_path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()

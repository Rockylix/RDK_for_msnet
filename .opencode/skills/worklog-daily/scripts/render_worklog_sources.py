import json
from pathlib import Path


def main():
    root = Path(".").resolve()
    src_path = root / "worklog_sources.json"
    if not src_path.exists():
        raise SystemExit("worklog_sources.json not found. Run collect_worklog_sources.py first.")

    data = json.loads(src_path.read_text(encoding="utf-8"))
    dates = data.get("dates", {})
    git = data.get("git", {})

    daily_dir = root / "daily"
    daily_dir.mkdir(parents=True, exist_ok=True)

    for date, sections in sorted(dates.items()):
        lines = [f"# {date} 素材", "", "## Git", ""]
        commits = git.get(date, [])
        if commits:
            for c in commits:
                lines.append(f"- `{c['hash']}` {c['subject']}")
        else:
            lines.append("- 无提交记录")
        lines.append("")
        lines.append("## 记录摘录")
        lines.append("")
        for s in sections:
            lines.append(f"### 来源：{s['source']}")
            lines.append("")
            lines.append(s["content"].strip())
            lines.append("")
        out_path = daily_dir / f"{date}.sources.md"
        out_path.write_text("\n".join(lines).strip() + "\n", encoding="utf-8")
        print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()

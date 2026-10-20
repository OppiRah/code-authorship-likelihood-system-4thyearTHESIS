"""Local checks for the CALSS information website (standard library only).

    python -I website/check_site.py            # structure + links
    python -I website/check_site.py --release  # also fail on draft markers

Checks every page for: one <h1>, <html lang>, viewport meta, <title>,
stylesheet present, skip link target, nav links to all three pages with
exactly one aria-current, every relative link/anchor resolving, no
<script>/<form>/<iframe>, and (with --release) no confirm markers or
draft banners left.
"""
import sys
from html.parser import HTMLParser
from pathlib import Path

SITE = Path(__file__).resolve().parent
PAGES = ["index.html", "privacy.html", "terms.html"]


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids, self.links, self.tags = set(), [], []
        self.h1 = 0
        self.lang = self.viewport = self.title = False
        self.current = 0
        self.confirms = self.banners = 0

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        self.tags.append(tag)
        if "id" in a:
            self.ids.add(a["id"])
        cls = (a.get("class") or "").split()
        self.confirms += "confirm" in cls
        self.banners += "draft-banner" in cls
        if tag == "html" and a.get("lang"):
            self.lang = True
        if tag == "meta" and a.get("name") == "viewport":
            self.viewport = True
        if tag == "title":
            self.title = True
        if tag == "h1":
            self.h1 += 1
        if tag == "a" and a.get("aria-current") == "page":
            self.current += 1
        for attr in ("href", "src"):
            if a.get(attr):
                self.links.append(a[attr])


def main():
    release = "--release" in sys.argv
    errors, parsed = [], {}
    for name in PAGES:
        p = Page()
        p.feed((SITE / name).read_text(encoding="utf-8"))
        parsed[name] = p

    for name, p in parsed.items():
        e = lambda msg: errors.append(f"{name}: {msg}")
        if p.h1 != 1: e(f"{p.h1} <h1> elements (want 1)")
        if not p.lang: e("missing <html lang>")
        if not p.viewport: e("missing viewport meta")
        if not p.title: e("missing <title>")
        if "main" not in p.ids: e("skip link target #main missing")
        if p.current != 1: e(f"{p.current} aria-current links (want 1)")
        for bad in ("script", "form", "iframe"):
            if bad in p.tags: e(f"contains <{bad}>")
        for other in PAGES:
            if other not in p.links: e(f"no link to {other}")
        for link in p.links:
            if link.startswith(("http://", "https://", "mailto:")):
                if link.startswith("http://"): e(f"insecure link {link}")
                continue
            path, _, frag = link.partition("#")
            target = path or name
            if not (SITE / target).exists():
                e(f"broken link {link}")
            elif frag and frag not in parsed.get(target, Page()).ids:
                e(f"broken anchor {link}")
        if release and (p.confirms or p.banners):
            e(f"{p.confirms} confirm marker(s), {p.banners} draft banner(s) remain")

    if not (SITE / "assets" / "style.css").exists():
        errors.append("assets/style.css missing")

    for err in errors:
        print("FAIL", err)
    total = {n: p.confirms for n, p in parsed.items()}
    print(f"{len(PAGES)} pages checked; confirm markers per page: {total}")
    print("OK" if not errors else f"{len(errors)} problem(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())

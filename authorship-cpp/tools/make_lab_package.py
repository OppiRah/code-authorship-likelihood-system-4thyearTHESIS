"""Build the CALSS laboratory deployment package (run by make_lab_package.bat).

    python tools/make_lab_package.py

Reads the version from include/version.h, takes the already-built
release/CALSS_Setup_<version>.exe (make_installer.bat), and writes
release/CALSS_<version>_Lab_Deployment/ plus a .zip of it:

    CALSS_Setup_<version>.exe        copied byte for byte
    CALSS_SampleData_SYNTHETIC.zip   copied byte for byte
    README.txt, RELEASE_NOTES.txt    from release_docs/lab/ (placeholders filled)
    INSTALLATION_GUIDE.pdf           rendered from release_docs/lab/INSTALLATION_GUIDE.md
    SHA256SUMS.txt                   computed last, from the files above

The installer's checksum written into the docs is computed from the
installer file itself, and is cross-checked against make_installer.bat's
CALSS_Setup_<version>_SHA256.txt. Needs reportlab (pip install reportlab).
Nothing is signed and nothing outside release/ is written.
"""
import datetime
import hashlib
import os
import re
import shutil
import sys
import zipfile

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import (KeepTogether, ListFlowable, ListItem, Paragraph,
                                SimpleDocTemplate, Spacer, Table, TableStyle)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DOCS = os.path.join(ROOT, "release_docs", "lab")
RELEASE = os.path.join(ROOT, "release")
SAMPLE = "CALSS_SampleData_SYNTHETIC.zip"

MAROON = colors.HexColor("#800000")
GOLD = colors.HexColor("#C9A227")
GRAY = colors.HexColor("#404040")
LIGHT = colors.HexColor("#F3F1EC")


def fail(msg):
    print("ERROR: " + msg)
    sys.exit(1)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def read_version():
    text = open(os.path.join(ROOT, "include", "version.h"), encoding="utf-8").read()
    parts = [re.search(r"#define CALSS_VERSION_%s (\d+)" % k, text) for k in ("MAJOR", "MINOR", "PATCH")]
    if not all(parts):
        fail("could not read the version from include/version.h")
    return ".".join(p.group(1) for p in parts)


# ── Markdown subset -> PDF ──────────────────────────────────────
# Supports exactly what INSTALLATION_GUIDE.md uses: "# " title, "## "
# headings, paragraphs, "- " bullets, "1. " numbered lists, pipe tables,
# **bold** and `code`.

def inline(s):
    s = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    s = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", s)
    s = re.sub(r"`(.+?)`", r'<font face="Courier">\1</font>', s)
    return s


def styles():
    base = ParagraphStyle("body", fontName="Helvetica", fontSize=9.5, leading=13,
                          textColor=colors.black, spaceAfter=5, alignment=TA_LEFT)
    return {
        "title": ParagraphStyle("title", parent=base, fontName="Helvetica-Bold", fontSize=18,
                                leading=22, textColor=MAROON, spaceAfter=4),
        "h2": ParagraphStyle("h2", parent=base, fontName="Helvetica-Bold", fontSize=12.5,
                             leading=16, textColor=MAROON, spaceBefore=10, spaceAfter=5),
        "body": base,
        "cell": ParagraphStyle("cell", parent=base, fontSize=8.5, leading=11, spaceAfter=0),
        "cellh": ParagraphStyle("cellh", parent=base, fontName="Helvetica-Bold", fontSize=8.5,
                                leading=11, spaceAfter=0, textColor=colors.white),
    }


def table_flowable(rows, st, width):
    header, body = rows[0], rows[1:]
    data = [[Paragraph(inline(c), st["cellh"]) for c in header]]
    data += [[Paragraph(inline(c), st["cell"]) for c in r] for r in body]
    n = len(header)
    if n == 3 and header[0] == "#":
        widths = [0.06, 0.74, 0.20]
    elif n == 3:
        widths = [0.30, 0.48, 0.22]
    else:
        widths = [0.34, 0.66]
    t = Table(data, colWidths=[w * width for w in widths], repeatRows=1)
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), MAROON),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, LIGHT]),
        ("GRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#BBBBBB")),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("TOPPADDING", (0, 0), (-1, -1), 3),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
    ]))
    return t


def md_to_flowables(md, st, width):
    out, para, lst, lst_kind, rows = [], [], [], None, []

    def flush():
        nonlocal para, lst, lst_kind, rows
        if para:
            out.append(Paragraph(inline(" ".join(para)), st["body"]))
            para = []
        if lst:
            items = [ListItem(Paragraph(inline(x), st["body"]), leftIndent=14) for x in lst]
            kw = {"bulletType": "1", "bulletFormat": "%s."} if lst_kind == "ol" else {"bulletType": "bullet", "start": "•"}
            out.append(ListFlowable(items, leftIndent=14, bulletFontSize=9, **kw))
            lst, lst_kind = [], None
        if rows:
            out.append(table_flowable(rows, st, width))
            out.append(Spacer(1, 6))
            rows = []

    for raw in md.splitlines():
        line = raw.rstrip()
        if not line:
            flush()
            continue
        if line.startswith("# "):
            flush()
            out.append(Paragraph(inline(line[2:]), st["title"]))
            out.append(Table([[""]], colWidths=[width], rowHeights=[2],
                             style=[("BACKGROUND", (0, 0), (-1, -1), GOLD)]))
            out.append(Spacer(1, 6))
        elif line.startswith("## "):
            flush()
            out.append(KeepTogether([Paragraph(inline(line[3:]), st["h2"])]))
        elif line.startswith("|"):
            if para or lst:
                flush()
            cells = [c.strip() for c in line.strip("|").split("|")]
            if all(re.fullmatch(r"-+", c) for c in cells):
                continue
            rows.append(cells)
        elif line.startswith("- "):
            if lst_kind == "ol" or para or rows:
                flush()
            lst_kind = "ul"
            lst.append(line[2:])
        elif re.match(r"\d+\. ", line):
            if lst_kind == "ul" or para or rows:
                flush()
            lst_kind = "ol"
            lst.append(line.split(". ", 1)[1])
        else:
            if lst or rows:
                flush()
            para.append(line.strip())
    flush()
    return out


def render_pdf(md, path, version):
    st = styles()
    doc = SimpleDocTemplate(path, pagesize=A4, leftMargin=20 * mm, rightMargin=20 * mm,
                            topMargin=18 * mm, bottomMargin=18 * mm,
                            title="CALSS %s Laboratory Installation Guide" % version,
                            author="CALSS thesis team", subject="CALSS installation guide")

    def footer(canvas, d):
        canvas.saveState()
        canvas.setFont("Helvetica", 7.5)
        canvas.setFillColor(GRAY)
        canvas.drawString(20 * mm, 10 * mm, "CALSS %s - Laboratory Installation Guide" % version)
        canvas.drawRightString(A4[0] - 20 * mm, 10 * mm, "Page %d" % d.page)
        canvas.restoreState()

    doc.build(md_to_flowables(md, st, doc.width), onFirstPage=footer, onLaterPages=footer)


def main():
    version = read_version()
    installer = "CALSS_Setup_%s.exe" % version
    src_installer = os.path.join(RELEASE, installer)
    src_sample = os.path.join(RELEASE, SAMPLE)
    for p in (src_installer, src_sample):
        if not os.path.isfile(p):
            fail("missing " + p + " (run compile.bat and make_installer.bat first)")

    inst_hash = sha256(src_installer)
    inst_size = os.path.getsize(src_installer)
    sidecar = os.path.join(RELEASE, "CALSS_Setup_%s_SHA256.txt" % version)
    if not os.path.isfile(sidecar) or inst_hash not in open(sidecar, encoding="ascii").read():
        fail("installer hash does not match " + sidecar + " - rebuild with make_installer.bat")

    out = os.path.join(RELEASE, "CALSS_%s_Lab_Deployment" % version)
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)

    values = {
        "{{VERSION}}": version,
        "{{INSTALLER}}": installer,
        "{{SIZE}}": "{:,}".format(inst_size),
        "{{SHA256}}": inst_hash,
        "{{SAMPLE}}": SAMPLE,
        "{{DATE}}": datetime.date.today().isoformat(),
    }

    def fill(text):
        for k, v in values.items():
            text = text.replace(k, v)
        left = re.findall(r"\{\{\w+\}\}", text)
        if left:
            fail("unfilled placeholders: " + ", ".join(sorted(set(left))))
        return text

    shutil.copyfile(src_installer, os.path.join(out, installer))
    shutil.copyfile(src_sample, os.path.join(out, SAMPLE))
    for name in ("README.txt", "RELEASE_NOTES.txt"):
        text = fill(open(os.path.join(DOCS, name), encoding="utf-8").read())
        with open(os.path.join(out, name), "w", encoding="ascii", newline="\r\n") as f:
            f.write(text)
    render_pdf(fill(open(os.path.join(DOCS, "INSTALLATION_GUIDE.md"), encoding="utf-8").read()),
               os.path.join(out, "INSTALLATION_GUIDE.pdf"), version)

    # Checksums last, once every other file is final.
    names = [installer, "INSTALLATION_GUIDE.pdf", "README.txt", "RELEASE_NOTES.txt", SAMPLE]
    with open(os.path.join(out, "SHA256SUMS.txt"), "w", encoding="ascii", newline="\r\n") as f:
        for n in names:
            f.write("%s  %s\n" % (sha256(os.path.join(out, n)), n))

    # Verify: exact file list, installer bytes unchanged, sums re-check.
    expected = sorted(names + ["SHA256SUMS.txt"])
    actual = sorted(os.listdir(out))
    if actual != expected:
        fail("package file list differs: %s" % actual)
    if sha256(os.path.join(out, installer)) != inst_hash:
        fail("installer copy differs from the built installer")
    for line in open(os.path.join(out, "SHA256SUMS.txt"), encoding="ascii"):
        h, n = line.split()
        if sha256(os.path.join(out, n)) != h:
            fail("checksum mismatch for " + n)

    zpath = out + ".zip"
    if os.path.exists(zpath):
        os.remove(zpath)
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        for n in expected:
            z.write(os.path.join(out, n), "CALSS_%s_Lab_Deployment/%s" % (version, n))

    print("CALSS %s laboratory package: %s" % (version, out))
    for n in expected:
        print("  %-36s %12s bytes" % (n, "{:,}".format(os.path.getsize(os.path.join(out, n)))))
    print("Installer SHA-256: " + inst_hash)
    print("Package ZIP:       %s" % zpath)
    print("ZIP SHA-256:       %s" % sha256(zpath))


if __name__ == "__main__":
    main()

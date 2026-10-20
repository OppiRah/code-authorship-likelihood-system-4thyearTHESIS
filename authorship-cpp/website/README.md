# CALSS public information website

Static pages that give the CALSS Google OAuth consent screen its
**Application home page**, **Privacy policy** and **Terms of service**
links. Not part of the desktop app: nothing here is compiled, packaged by
`make_release.bat`/`make_lab_package.bat`, or installed by `calss.iss`.

| File | Purpose |
|---|---|
| `index.html` | Home page (app name, purpose, audience, Classroom/Gemini, human-review statement, contact) |
| `privacy.html` | Privacy policy (grounded in the 1.3.1 source audit) |
| `terms.html` | Terms of service |
| `assets/style.css` | The only stylesheet; CALSS maroon/gold tokens, light + dark |
| `.nojekyll` | Tells GitHub Pages to serve the files as-is |
| `check_site.py` | Local checks: links, anchors, required page elements, leftover placeholders |

No JavaScript, no web fonts, no cookies, no analytics, no forms.

## Status: DRAFT — do not publish yet

Every `<span class="confirm">` and every `.draft-banner` marks text that
the project owner / university must confirm. Before deploying:

1. Fill in contact e-mail, adviser/DPO contacts, effective date and
   hosting provider.
2. Resolve the Gemini / Limited Use issue (privacy.html §8; see
   `docs/GOOGLE_OAUTH_PRODUCTION_READINESS.md`) and update §5 and §8.
3. Have the university's legal office / DPO review privacy + terms.
4. Remove the confirm markers and draft banners.
5. `python -I website/check_site.py --release` must pass (it fails while
   any confirm marker or draft banner remains).

## Preview locally

```
python -m http.server 8000 --directory website
```

then open http://127.0.0.1:8000/ . (Opening `index.html` directly also
works; all links are relative.)

## Deploy

See `docs/GOOGLE_OAUTH_PRODUCTION_READINESS.md` §6 (hosting/domain) —
the site can be served from any static host as-is.

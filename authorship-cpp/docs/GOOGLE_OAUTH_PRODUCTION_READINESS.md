# Google OAuth production readiness — CALSS 1.3.1

Prepared 2026-10-20 on branch `optimization-pass`. Read-only audit of the
1.3.1 source plus research of Google's documentation. **No desktop-app
source, installer or build file was changed.** New files: this document
and `website/` (see `website/README.md`). Nothing was published, deployed
or changed in Google Cloud Console.

Legend: **[V]** verified in source (file:line), **[G]** stated in Google
documentation (linked), **[U]** unverified / needs owner or Google
confirmation.

---

## 1. Bottom line

1. **CALSS requests a restricted scope.** `drive.readonly` is classified
   *restricted* by Google [G]. The four Classroom scopes are not on
   Google's restricted list [G]; Google's Classroom page does not label
   them, so their *sensitive* classification must be confirmed in the
   Cloud Console scope picker [U].
2. **Publishing to Production is not the same as being verified.**
   Branding links unblock the *Publish app* button; after that,
   sensitive/restricted scopes still need Google's data-access
   verification. Until verified, users see the "Google hasn't verified
   this app" screen and the app is capped at 100 new users [G].
3. **Restricted-scope verification is heavy**: demo video, scope
   justification, Limited Use compliance, and — if restricted data is
   stored on or transmitted through servers — an annual third-party
   CASA security assessment, paid by the developer [G]. CALSS keeps the
   Drive file content on the instructor's PC and never sends it to a
   server [V], but whether Google waives the assessment for this case is
   **Google's decision** [U].
4. **A Limited Use problem exists today.** Student names obtained through
   `classroom.rosters.readonly` and class names/sections from Classroom
   can be sent to Gemini (`src/gemini.cpp:665`, `:830`) [V]. With a
   free-tier key Google may use that content to improve its products and
   have humans read it [G]; the Workspace API policy forbids transferring
   user data to train/improve AI models [G]. This must be fixed (code
   change) or prevented (policy/configuration) **before** applying for
   verification. Not changed here — development is frozen.
5. **Recommendation:** for the thesis defense, stay in External Testing
   (Option C) with professors added as test users; in parallel ask the
   university IT office about an Internal app in the university's
   Workspace (Option B), which needs no Google verification for any
   scope. External Production with verification (Option A) is only worth
   it if CALSS outlives the thesis with a maintainer. See §5.

---

## 2. Source-code audit (CALSS 1.3.1)

### 2.1 OAuth flow and client

| Item | Finding |
|---|---|
| Flow | Authorization-code, installed-app. Browser opened with `ShellExecuteA` on `accounts.google.com/o/oauth2/auth`; code captured by a loopback listener; exchanged at `oauth2.googleapis.com/token`. [V] `src/google_classroom.cpp:569-652`, `:387-413` |
| Redirect URI | `http://localhost:8080` (fixed port, hard-coded twice). [V] `:392`, `:591` |
| Listener | Binds `127.0.0.1:8080` only (`INADDR_LOOPBACK`), 3-minute `select` timeout, single connection. [V] `:478-567` |
| `state` | 16 random bytes (`CryptGenRandom`), checked on callback. [V] `:449-462`, `:533-537` |
| PKCE | **Not used.** No `code_challenge`/`code_verifier`. [V] `:588-596` |
| `access_type=offline`, `prompt=consent` | Yes — refresh token requested, consent shown every sign-in. [V] `:594-595` |
| Client config | `calss_gc_embedded_credentials.json` next to the exe (gitignored, shipped by the installer, `installer/calss.iss:104`); cached to `%LOCALAPPDATA%\CALSS\calss_gc_settings.json` including the client secret. [V] `:314-384` |
| Client type | "installed" JSON → Desktop OAuth client [V, by the loader expecting `installed`]. Cloud Console type not checked [U]. |

Assessment against Google's desktop-app guide [G]
(<https://developers.google.com/identity/protocols/oauth2/native-app>):
loopback redirect is the supported method for desktop apps; Google
recommends `127.0.0.1`/`[::1]` on a **random available port** and notes
`localhost` "may cause issues with client firewalls"; it says to "always
use PKCE"; desktop clients "cannot keep secrets", so shipping the client
secret is expected. CALSS is acceptable but has three improvements
(future version, not done): PKCE, `127.0.0.1` + ephemeral port (fixes the
known "port 8080 in use" issue too), and a real revoke on sign-out.

### 2.2 Scopes requested — exact strings

`src/google_classroom.cpp:570-576` [V]:

| Scope | Google classification | Used for (verified) |
|---|---|---|
| `https://www.googleapis.com/auth/classroom.courses.readonly` | Not restricted [G]; expected **sensitive** [U — confirm in Console] | `GET /v1/courses?courseStates=ACTIVE&pageSize=30` (`:666`) — id, name, section, enrollmentCode (enrollmentCode parsed into memory, never used elsewhere) |
| `https://www.googleapis.com/auth/classroom.coursework.students.readonly` | Not restricted; expected **sensitive** [U] | `GET /v1/courses/{id}/courseWork?pageSize=30` (`:688`) — id, title, description |
| `https://www.googleapis.com/auth/classroom.student-submissions.students.readonly` | Not restricted; expected **sensitive** [U] | `GET .../courseWork/{id}/studentSubmissions?pageSize=100` (`:862-865`) — state, userId, attachments.driveFile {id, title, mimeType} |
| `https://www.googleapis.com/auth/classroom.rosters.readonly` | Not restricted; expected **sensitive** [U] | `GET /v1/userProfiles/{userId}` (`:902`) — givenName, familyName / name |
| `https://www.googleapis.com/auth/drive.readonly` | **Restricted** [G] | `GET /drive/v3/files/{id}?alt=media` (`:709`) — content of `.c` attachments only |

Observations:
- `classroom.courses.readonly` + `coursework.students.readonly` return
  data CALSS does not use (enrollment code, description). Harmless, but a
  verification reviewer asks why each scope is needed — narrow if possible.
- No pagination: max 30 courses, 30 assignments, 100 submissions per
  assignment [V]. Functional limitation, not a privacy issue.
- Is `drive.readonly` avoidable? `drive.file` (non-sensitive) only covers
  files the user opened with/created by the app (e.g. via Google Picker),
  so it does not cover arbitrary student attachments [G]. Google's own
  Classroom guidance for reading attachment *content* uses Drive scopes.
  No narrower scope that works without a UI change is known [U].

### 2.3 Data retrieved and where it goes

| Data | Stored | Leaves the PC? |
|---|---|---|
| `.c` file content (Drive) | `%LOCALAPPDATA%\CALSS\Classroom\<category>__<StudentName>_ASGN<6>.c` (`:1004-1009`, `gui_sync_sheet.cpp:636-639`) | No [V] |
| Assignment title, course name + section | `calss_sync_manifest.jsonl` in that folder (`:727-742`) | **Yes, to Gemini** (class label) if AI on [V] |
| Student given/family name | Embedded in the file name; in `workspace.json` (last student/pair viewed); in `audit.log` on report generation (`gui.cpp:2264`); in per-student HTML reports | **Yes, to Gemini** if AI on [V] |
| Student Google userId | Last 8 chars used only as fallback file name `Student_xxxxxxxx` (`:898-900`) | Only as part of that fallback name |
| Tokens | Credential Manager `CALSS/GoogleClassroomToken`, `CRED_TYPE_GENERIC`, `CRED_PERSIST_LOCAL_MACHINE` (`secret_store.cpp:39-44`) — per Windows user, not roamed | Only to Google endpoints [V] |

### 2.4 Gemini (optional)

- Off unless a key is saved **and** "Include AI summary" is ticked [V].
- Two requests per run to `generativelanguage.googleapis.com` (+1 retry
  each on model-unavailable/503) [V]. Key in `x-goog-api-key` header [V].
- Content: for up to 50 top flagged pairs — student names, class label,
  combined %, authorship %, reliability, profile size, deviating feature
  names and numeric profile/submission values (`gemini.cpp:661-684`,
  `:692-741`, `:759-858`) [V].
- **No source-code text** is in either prompt [V]. Note:
  `installer/before_you_install.rtf:25` and `RELEASE_READINESS.md` §11.3
  say "code from up to 50 flagged pairs per run is sent" — this
  **overstates** what 1.3.1 sends. Conservative, so not harmful, but
  inconsistent with the website; flag for the team (not changed: frozen).
- Gemini API terms [G] (<https://ai.google.dev/gemini-api/terms>): unpaid
  services — Google uses content "to provide, improve, and develop" its
  products, "human reviewers may read, annotate, and process" it, and
  "Do not submit sensitive, confidential, or personal information to the
  Unpaid Services". Paid — not used to improve products.

### 2.5 Telemetry / other network

Only network hosts in the source: `oauth2.googleapis.com`,
`classroom.googleapis.com`, `www.googleapis.com` (Drive),
`generativelanguage.googleapis.com`; browser opened to
`accounts.google.com` and `aistudio.google.com/apikey` [V, grep of
`src/`]. No analytics, crash reporting or CALSS server. Audit log is
local (`%PROGRAMDATA%\CALSS\audit.log`) [V].

### 2.6 Disconnect, revocation, deletion

- **No user-facing sign-out/disconnect.** `gc_signOut()` (local delete of
  the Credential Manager entry) is only called automatically when a
  roster call returns 401/403 (`google_classroom.cpp:924-935`) [V].
- **No token revocation** call to `oauth2.googleapis.com/revoke` anywhere [V].
- Uninstaller deletes only `{localappdata}\CALSS\Classroom` of the
  account running it; keeps Credential Manager entries, `%LOCALAPPDATA%\CALSS`
  (settings, `workspace.json`), `Documents\CALSS Reports/Imports`,
  `audit.log` (`installer/calss.iss:114-119`, `:157-172`) [V].
- Downloaded submissions have no retention limit [V].

### 2.7 Shared Windows accounts

Credential Manager and `%LOCALAPPDATA%` are per Windows account, not per
person. On a shared lab login, any user can sync with the previous
instructor's Google sign-in and read their downloads [V, documented in
the installer page]. The ProgramData audit log is readable by all Users
(`calss.iss:122-136`) and can contain student names [V].

---

## 3. Google requirements (research, 2026-10-20)

| Topic | What Google says | Source |
|---|---|---|
| `drive.readonly` is restricted | "Restricted: … `drive.readonly` — View and download all your Drive files" | [Drive API scopes](https://developers.google.com/workspace/drive/api/guides/api-specific-auth) |
| Restricted scope list | Classroom scopes are not on it (Gmail, Drive, Fit, Chat, … are) | [Restricted scopes](https://support.google.com/cloud/answer/13464325) |
| Classroom scopes | Listed with descriptions, no classification on the page | [Classroom auth guide](https://developers.google.com/workspace/classroom/guides/auth) |
| Verification for sensitive/restricted | Brand requirements + appropriate use case, demo video, Limited Use, narrowest scopes; restricted adds "an annual security assessment from a Google empanelled group of security assessors" | [Verification requirements](https://support.google.com/cloud/answer/13464321) |
| When the assessment applies | "If you store or transmit restricted scope data on servers, then you need to complete a security assessment"; reverify at least every 12 months; exceptions listed: personal use, dev/testing, service-owned data, **internal use only**, domain-wide install | [Restricted scope verification](https://developers.google.com/identity/protocols/oauth2/production-readiness/restricted-scope-verification) |
| Assessment cost | Google charges nothing; cost agreed with the assessor | [Verification FAQ](https://support.google.com/cloud/answer/13463817) |
| Unverified apps | "unverified app" screen; cap of 100 new users total; exempt: dev/test builds not launched publicly, internal apps of a Cloud Organization | [Unverified apps](https://support.google.com/cloud/answer/7454865) |
| Testing vs Production | Testing: up to 100 test users; "Authorizations by a test user will expire seven days from the time of consent". Production: any Google account, verification may still be required | [Audience / user type](https://support.google.com/cloud/answer/15549945) |
| 7-day refresh token | Issued for External + Testing (unless only name/email/profile scopes). The doc ties it only to Testing; moving to Production should end it, but Google does not state that explicitly [U] | [OAuth 2.0 overview — refresh token expiration](https://developers.google.com/identity/protocols/oauth2#expiration) |
| Internal user type | Only for projects in a Google Cloud Organization; limits sign-in to org members (`org_internal` error otherwise) | [Audience](https://support.google.com/cloud/answer/15549945) |
| Home page | On a verified domain you own, describes the app, "can not be only a login page", links the privacy policy | [Branding](https://support.google.com/cloud/answer/15549049) |
| Privacy policy | HTML on a dedicated page (no PDF/iframe), on the same/verified domain, linked from home page and consent screen, URL ≠ home page URL; required for external production apps | [Branding](https://support.google.com/cloud/answer/15549049), [Privacy policy reqs](https://support.google.com/cloud/answer/13806988) |
| Authorized domains | Top private domains used by home/privacy/ToS URLs; subdomains covered; verify ownership in Search Console | [Branding](https://support.google.com/cloud/answer/15549049) |
| Limited Use | Use only for user-facing features; no transfer except listed cases; no use "to create, train, or improve a machine learning or artificial intelligence model"; privacy policy must fully document; Limited Use disclosure on site or in app | [Workspace API user data policy](https://developers.google.com/workspace/workspace-api-user-data-developer-policy), [Google API Services User Data Policy](https://developers.google.com/terms/api-services-user-data-policy) |
| Desktop clients | Loopback redirect; prefer `127.0.0.1` + random port; always PKCE; client secret not confidential; revoke via `POST https://oauth2.googleapis.com/revoke` | [OAuth for desktop apps](https://developers.google.com/identity/protocols/oauth2/native-app) |
| Workspace admin control | Admins mark apps Trusted / Specific data / Limited / Blocked; can restrict Drive and other services, block unconfigured apps, and set stricter rules for under-18 users | [Control which apps access Workspace data](https://knowledge.workspace.google.com/admin/apps/control-which-apps-access-google-workspace-data) |

**Documented requirement vs recommendation:** branding links, domain
verification, privacy policy, demo video, scope justification, Limited
Use and (for server-handled restricted data) CASA are *requirements*. A
Terms-of-service link is optional ("recommends"). `127.0.0.1` + random
port and PKCE are *recommendations* in the desktop guide.

**Free subdomains (`*.github.io`, `*.pages.dev`)** [U]: Google requires a
domain "you own" verified in Search Console. These hosts are on the
Public Suffix List, so `yourname.github.io` is its own top private domain
and can be verified as a URL-prefix property with an HTML file; reports of
this being accepted exist, but Google does not document it either way.
Search Console's *Domain* (DNS) property is not available for them.

---

## 4. Workspace administrator caveat

Even after verification, a professor whose account is in a Google
Workspace for Education domain can be blocked if the domain admin
restricts Classroom/Drive API access or blocks unconfigured third-party
apps [G]. Before rollout, ask the university Workspace admin to mark
CALSS's OAuth client ID as **Trusted** (Admin console → Security → API
controls → App access control). For a verified or unverified External
app this also avoids per-user surprises. Status of UL's policy: [U].

---

## 5. Deployment options

| | A. External + Production + verification | B. Internal (university Workspace) | C. External Testing |
|---|---|---|---|
| Who can sign in | Any Google account (after verification) | Only `@<university domain>` accounts | Up to 100 listed test users |
| Google verification | Required: brand + sensitive + **restricted** (`drive.readonly`) | **Not required** [G] | Not required |
| CASA assessment | Possibly (Google decides; annual; paid to assessor) | No | No |
| Unverified screen / cap | Until verified: warning + 100-user cap | None | Warning shown to test users |
| Token life | Normal refresh tokens [U, see §3] | Normal | **7 days** — re-sign-in weekly |
| Cost | Domain (~USD 10-15/yr) + possible assessment; weeks-months of review | None if the university hosts the project | None |
| Needs | Website + domain, Limited Use fix, demo video, maintainer for annual re-verification | University IT to create/own the Cloud project (or move it) in its Organization, and allow the app | Add each professor's e-mail |
| Fit for CALSS | Poor for a thesis: restricted scope + Gemini issue + annual upkeep | **Best long-term**, if all professors use UL Workspace accounts | **Best for defense/demo** |

**Recommendation.** C now (works today; known 7-day re-sign-in). Ask UL IT
for B — it removes verification entirely and matches "authorized UL lab
instructors". Choose A only if CALSS must serve accounts outside UL's
Workspace and someone will own annual re-verification. The website is
useful for all three (B and C do not require it, but Branding still
benefits from accurate links).

Open question for B [U]: professors must sign in with a UL Workspace
account that is a teacher in the Classroom classes; personal Gmail
Classroom classes would not work.

---

## 6. Hosting and domain

**Recommendation: buy a custom domain and host on GitHub Pages or
Cloudflare Pages.** A domain you own can be verified as a DNS *Domain*
property in Search Console, which is what Google's docs describe and is
the least ambiguous for reviewers. `*.github.io` may work (§3) but is
unconfirmed; acceptable for a trial, not something to stake verification
on. A university subdomain (e.g. `calss.<ul-domain>`) is ideal if UL IT
will host it and verify the domain — then the authorized domain is UL's
domain and UL must be the Search Console owner.

### Deploy (GitHub Pages, custom domain) — manual, not done
1. Create a **separate public repo** (e.g. `calss-site`) containing only
   the contents of `website/` (do not publish this app repo; it holds
   internal docs).
2. Settings → Pages → Deploy from branch `main` / root.
3. Buy the domain; in DNS add the records GitHub Pages documents for
   apex or a `CNAME` for `www`; set the custom domain in Pages settings;
   enable *Enforce HTTPS*.
4. Search Console → Add property → **Domain** → add the TXT record → Verify,
   using the **same Google account that is Owner/Editor of the Cloud
   project**.

Cloudflare Pages is equivalent (connect repo or upload `website/`, add
custom domain, then Search Console TXT record in Cloudflare DNS).

### URLs to enter (Google Auth Platform → Branding) — placeholders
| Field | Value |
|---|---|
| Application home page | `https://<your-domain>/` |
| Application privacy policy link | `https://<your-domain>/privacy.html` |
| Application terms of service link | `https://<your-domain>/terms.html` |
| Authorized domains | `<your-domain>` (top private domain only, e.g. `calss-ul.org`) |

No URL is live yet; nothing has been verified.

---

## 7. Google Cloud Console — manual steps

Done by the project owner. Nothing here was performed.

1. **Before anything:** resolve §8 blockers 1-2 (Gemini/Limited Use,
   contact details) and get legal/DPO sign-off on privacy + terms.
2. Deploy the website; verify the domain in Search Console (§6).
3. Google Auth Platform → **Branding**: app name "CALSS", support e-mail
   (a team address, not personal), logo (**only if you have rights** —
   the UL seal needs university permission; uploading a logo also
   triggers brand verification), home/privacy/ToS URLs, authorized
   domain, developer contact e-mail.
4. **Data Access**: add exactly the five scopes in §2.2; confirm which
   the console lists as sensitive vs restricted and record it in §2.2.
5. **Clients**: confirm the client is type *Desktop app*. (Desktop clients
   have no redirect-URI list; loopback is allowed automatically.)
6. Decide A/B/C (§5).
   - **C:** Audience → Test users → add professors. Stop here.
   - **B:** needs the project under UL's Cloud Organization; Audience →
     User type → Internal. No verification.
   - **A:** Audience → Publish app → In production. Then Verification
     Center → *Verify branding* → publish branding → request data-access
     verification with: scope justifications (§2.2 "used for"), demo
     video, and answers about data handling (local only; Gemini as
     resolved). Expect follow-up e-mails; respond promptly or access can
     be lost [G].
7. Ask the UL Workspace admin to trust the client ID (§4).

### Demo video (Option A) — what to show
Unlisted YouTube video, English, showing: the app name and the consent
screen with all five scopes and the client ID visible in the browser URL;
the sync flow (choose class → assignments → download); where each scope's
data appears in the UI (class list, assignment list, files labelled with
student names, analysis of downloaded files); where data is stored; how
to revoke. **Use a test Classroom with synthetic students and the
committed synthetic `.c` datasets — never real submissions.**

### Testing with a professor not on the test-user list
- Option C: expected `403 access_denied` — confirms Testing restriction.
- Option B: sign in with a UL account → should work; a non-UL account →
  `org_internal` error.
- Option A before verification: user sees "Google hasn't verified this
  app" → Advanced → Go to CALSS (unsafe); counts toward the 100-user cap.
- After any change: sync one synthetic class end-to-end; after 8 days,
  confirm sync still works without re-sign-in (checks refresh-token
  lifetime); revoke at myaccount.google.com/connections and confirm CALSS
  asks to sign in again.

---

## 8. Blockers and items needing approval

### Blockers (before applying for verification)
1. **Gemini receives Classroom-derived student names/class labels**
   (§2.4). Pick one (code change in a future version, team decision):
   (a) pseudonymize — send "Student A/B", "Class 1" and map back locally
   (best; keeps feature working); (b) require/document paid-tier keys;
   (c) disable Gemini when the loaded folder is the Classroom folder.
   Until then, the Limited Use sentence in `privacy.html` §5 must not be
   published.
2. **Contact details, effective date, data controller, DPO, applicable
   law** — owner/university to supply (marked in the website).
3. **Restricted scope `drive.readonly`** — only Option A is affected;
   Google may require CASA.
4. **No in-app disconnect/revoke** — documented as manual steps in the
   privacy policy; a future version should add "Disconnect Google"
   (revoke + delete credential).

### Needs university approval
- Publishing the privacy policy and terms (legal office / DPO).
- Using "University of Luzon" and any UL logo on the site/consent screen.
- Option B (Cloud project in UL's organization) and admin trust (§4).
- Whether Gemini may process student names at all.
- Retention rule for downloaded submissions.

### Known limitations (documented on the site, not fixed)
No PKCE; fixed port 8080 / `localhost`; tokens kept after uninstall;
shared Windows accounts share sign-ins; audit log readable by all users
and kept after uninstall; plain-file storage of downloads; no pagination;
installer page overstates what is sent to Gemini.

---

## 9. Production-readiness checklist

- [ ] Team decides deployment option (A / B / C).
- [ ] Gemini Limited Use issue resolved and privacy §5/§8 updated.
- [ ] Contact e-mail, adviser/DPO, data controller, effective date filled in.
- [ ] Legal office / DPO approved privacy policy and terms.
- [ ] Permission to use UL name/logo confirmed.
- [ ] Domain chosen and bought (or UL subdomain agreed).
- [ ] `website/` deployed over HTTPS; `python -I website/check_site.py --release` passes first.
- [ ] Domain verified in Search Console by a Cloud project owner.
- [ ] Branding fields filled (§6 table); authorized domain added.
- [ ] Data Access scopes match §2.2 exactly; classifications recorded.
- [ ] Client type confirmed Desktop.
- [ ] (A) Branding verified and published; data-access verification submitted with demo video.
- [ ] (A) CASA requirement confirmed or waived by Google.
- [ ] (B) Project in UL organization, user type Internal.
- [ ] UL Workspace admin trusts the client ID.
- [ ] End-to-end test with a non-test-user professor account (§7).
- [ ] 8-day refresh-token check passed.
- [ ] Future version: PKCE, `127.0.0.1` + random port, Disconnect/revoke button, pseudonymized Gemini prompts.

---

## 10. Verification of this work

- `python -I website/check_site.py` → OK (3 pages: one `h1`, `lang`,
  viewport, skip link, nav with one `aria-current`, every relative link
  and anchor resolves, no `script`/`form`/`iframe`, no `http://` links).
- `--release` mode correctly fails while confirm markers remain
  (index 4, privacy 10, terms 6).
- Headless Chromium (Brave), dark scheme: 1280 px desktop and 375 px
  phone (iframe harness — Chromium's minimum window width hides real
  phone widths). No page-level horizontal scroll; the privacy tables
  scroll inside a focusable box (min-width 40rem) on phones.
- Not tested: light scheme rendering, real mobile devices, screen reader.
- Desktop app: no `src/`, `include/`, `installer/` or build file touched,
  so the golden test and build were not re-run.

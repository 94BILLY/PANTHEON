# NOVA README — header snippet (no `git am` / no `raw` URL)

Use this if **`curl` / `raw.githubusercontent.com` returns 404** (private Pantheon repo) or **`git am` says “Patch is empty”** because the downloaded file is actually a tiny **404 HTML** page (~14 bytes).

## Replace the **first 3 lines** of `NOVA/README.md` with this block

Leave everything from line 4 (`Nova is a native Win32...`) unchanged.

```markdown
<h1 align="center" style="margin-top:0;margin-bottom:0;">
  <img src="https://94billy.com/wp-content/uploads/2026/03/Nova-300x300.png" alt="Nova" width="280" /><br />
  <span style="display:inline-block;margin-top:2px;">NOVA</span>
</h1>

<p align="center">Native Windows executive assistant</p>

<p align="center">v1.5 · Win32 C++17 · Local-first · No telemetry</p>

<p align="center"><em>&ldquo;The performance & durability of the past with the intelligence of the future.&rdquo;</em></p>
```

## Then commit and push

```powershell
cd C:\Users\marvi\NOVA\NOVA-repo
git add README.md
git commit -m "docs(README): minimalist header with Nova logo"
git push origin main
```

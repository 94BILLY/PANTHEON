# Apply NOVA README header on Windows (PowerShell)

## Why **`git am` says “Patch is empty”**

Usually **`curl.exe` did not download the patch**. For a **private** GitHub repo,  
`https://raw.githubusercontent.com/94BILLY/PANTHEON/...` returns **404** without auth.  
`curl` then saves a tiny **error page** (~**14 bytes**). `git am` on that file looks “empty”.

**Check the file size (must be ~1–2 KB, not 14 bytes):**

```powershell
(Get-Item nova-readme-header.patch).Length
```

If it’s **under 100 bytes**, the download failed — do **not** run `git am` on it.

---

## Fastest fix: **paste the header** (no patch file)

Open **`docs/NOVA_README_HEADER_SNIPPET.md`** in the Pantheon repo (or on GitHub when logged in) and follow it: replace the **first 3 lines** of NOVA’s `README.md`, then `git add` / `commit` / `push`.

---

## If you still want **`git am`** (good patch file)

### A — Copy **`nova-readme-header.patch`** from your Pantheon clone (always works; no `raw` URL)

The patch is committed in Pantheon at **`docs/nova-readme-header.patch`** (regenerated from the same README change).

```powershell
copy C:\path\to\Pantheon\docs\nova-readme-header.patch C:\Users\marvi\NOVA\NOVA-repo\
cd C:\Users\marvi\NOVA\NOVA-repo
git am --abort
git am nova-readme-header.patch
git push origin main
```

Confirm file size after copy: **`(Get-Item nova-readme-header.patch).Length`** should be **~1.3 KB**, not 14 bytes.

### B — Download with **GitHub CLI** (logged in)

```powershell
cd C:\Users\marvi\NOVA\NOVA-repo
gh api repos/94BILLY/PANTHEON/contents/docs/nova-readme-header.patch -H "Accept: application/vnd.github.raw" -o nova-readme-header.patch
git am --abort
git am nova-readme-header.patch
git push origin main
```

### C — Browser (logged into GitHub)

1. Open Pantheon on GitHub → `docs/nova-readme-header.patch`  
2. **Raw** → Save As → `nova-readme-header.patch` into `NOVA-repo`  
3. `git am --abort` then `git am nova-readme-header.patch`

---

## Git identity (once)

```powershell
git config --global user.name "94BILLY"
git config --global user.email "contact@94billy.com"
```

---

## If `git am` is stuck

```powershell
git am --abort
```

---

## PowerShell note

`curl` alone is **`Invoke-WebRequest`**. Use **`curl.exe`** for curl flags, or **`Invoke-WebRequest -OutFile`**.

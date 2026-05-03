# Apply NOVA README header patch on Windows (PowerShell)

PowerShell’s `curl` is **`Invoke-WebRequest`**, not real curl — it does **not** support `-sL`.

## Option A — Real curl (Windows 10+)

```powershell
cd C:\Users\marvi\NOVA
curl.exe -sL "https://raw.githubusercontent.com/94BILLY/PANTHEON/main/docs/nova-readme-header.patch" -o nova-readme-header.patch
git am nova-readme-header.patch
git push origin main
```

## Option B — PowerShell only

```powershell
cd C:\Users\marvi\NOVA
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/94BILLY/PANTHEON/main/docs/nova-readme-header.patch" -OutFile "nova-readme-header.patch"
git am nova-readme-header.patch
git push origin main
```

## If `git am` fails (“patch does not apply”)

Your `README.md` may have diverged from the patch base. Either:

1. **Apply by hand** — open the patch in a browser or editor and copy the new header into `README.md` from the `diff` section, or  
2. **Three-way** (if you use a recent Git): `git am --3way nova-readme-header.patch`

## Push rejected?

Use HTTPS with a **Personal Access Token** or **GitHub CLI** (`gh auth login`); Windows Credential Manager may need updating for `github.com`.

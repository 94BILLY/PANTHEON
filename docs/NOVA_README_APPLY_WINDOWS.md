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

## If `git am` says **“Patch is empty”**

Git thinks there is nothing to change — often because **`git am` is stuck halfway** or **`README.md` was already edited** to match the patch.

1. **Clean up any half-finished apply:**
   ```powershell
   cd C:\Users\marvi\NOVA\NOVA-repo
   git am --abort
   ```
2. **Confirm the patch file is not zero bytes** (should be a few KB):
   ```powershell
   dir nova-readme-header.patch
   ```
3. **Re-download the patch** (overwrites the file):
   ```powershell
   curl.exe -sL "https://raw.githubusercontent.com/94BILLY/PANTHEON/main/docs/nova-readme-header.patch" -o nova-readme-header.patch
   ```
4. **Check your README still has the *old* first lines** (if it already shows `<h1` and NOVA, the patch is empty on purpose — skip `git am` and just `git push` if you already committed):
   ```powershell
   type README.md | more
   ```
   Expected **before** patch: first line is `# NOVA v1.5 🌎`

5. **Try `git am` again:**
   ```powershell
   git am nova-readme-header.patch
   ```

**Still stuck?** Paste the new header into `README.md` by hand (from the patch `+` lines in [nova-readme-header.patch](https://raw.githubusercontent.com/94BILLY/PANTHEON/main/docs/nova-readme-header.patch)), then:

```powershell
git add README.md
git commit -m "docs(README): minimalist header with Nova logo"
git push origin main
```

## If `git am` fails (“patch does not apply”)

Your `README.md` may have diverged from the patch base. Either:

1. **Apply by hand** — open the patch in a browser or editor and copy the new header into `README.md` from the `diff` section, or  
2. **Three-way** (if you use a recent Git): `git am --3way nova-readme-header.patch`

## Push rejected?

Use HTTPS with a **Personal Access Token** or **GitHub CLI** (`gh auth login`); Windows Credential Manager may need updating for `github.com`.

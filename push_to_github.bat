@echo off
echo ================================================================
echo   FP-ASM GitHub Push Script
echo   Repository: https://github.com/TACITVS/FP_ASM_LIB_DEV
echo ================================================================
echo.

echo This script will push your local FP-ASM library to GitHub.
echo.
echo IMPORTANT: This will REPLACE the existing README.md on GitHub
echo            with your new comprehensive version.
echo.
pause

echo.
echo === Step 1: Checking Git status ===
git status
if %ERRORLEVEL% NEQ 0 (
    echo Initializing git repository...
    git init
)

echo.
echo === Step 2: Adding remote repository ===
git remote remove origin 2>nul
git remote add origin https://github.com/TACITVS/FP_ASM_LIB_DEV.git
git remote -v

echo.
echo === Step 3: Switching to main branch ===
git checkout -b main 2>nul
git checkout main

echo.
echo === Step 4: Staging all files ===
git add .
echo Files staged successfully!

echo.
echo === Step 5: Creating commit ===
git commit -m "FP-ASM v1.0.0: Complete library with 100%% FP equivalence

- Implemented 4 general HOFs (foldl, map, filter, zipWith)
- 100%% Haskell/Lisp/ML language equivalence achieved
- 87%% code reduction through functional composition
- Comprehensive test coverage (25+ test suites)
- Full documentation and GitHub-ready files
- Phase 4 refactoring complete (outlier detection)
- Production-ready with MIT license"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo No changes to commit (already up to date)
)

echo.
echo === Step 6: Pulling existing content from GitHub ===
echo Note: This will merge with existing README.md
git pull origin main --allow-unrelated-histories -X ours

echo.
echo === Step 7: Pushing to GitHub ===
echo.
echo Choose push method:
echo   1. Normal push (recommended if no conflicts)
echo   2. Force push (replaces everything on GitHub)
echo.
set /p PUSH_METHOD="Enter choice (1 or 2): "

if "%PUSH_METHOD%"=="2" (
    echo.
    echo WARNING: Force push will overwrite ALL content on GitHub!
    echo.
    pause
    git push -u origin main --force
) else (
    git push -u origin main
)

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Push failed! This might be due to:
    echo   - Authentication issues (use Personal Access Token)
    echo   - Network problems
    echo   - Conflicting changes on GitHub
    echo.
    echo Try running: git push -u origin main --force
    pause
    exit /b 1
)

echo.
echo === Step 8: Creating and pushing release tag ===
git tag -a v1.0.0 -m "FP-ASM v1.0.0 - Initial Public Release" 2>nul
git push origin v1.0.0

echo.
echo ================================================================
echo   SUCCESS! FP-ASM pushed to GitHub!
echo ================================================================
echo.
echo Your repository is now live at:
echo   https://github.com/TACITVS/FP_ASM_LIB_DEV
echo.
echo Next steps:
echo   1. Visit the repo and verify everything looks correct
echo   2. Go to Releases and create v1.0.0 release
echo   3. Copy content from GITHUB_RELEASE.md for release notes
echo   4. Enable Issues and Discussions in repo settings
echo   5. Add topics for discoverability
echo   6. Star your own repo!
echo   7. Share with the world!
echo.
echo ================================================================
pause

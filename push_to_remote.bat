@echo off
REM ================================================================================
REM Push Pattern 1 Work to Remote Repository
REM Branch name: Claude_pattern1_pure_fp_refactoring
REM ================================================================================

echo.
echo ================================================================================
echo  Pushing Pattern 1 Work to Remote
echo ================================================================================
echo.
echo This will push all Pattern 1 refactoring commits to remote branch:
echo   Claude_pattern1_pure_fp_refactoring
echo.
echo Commits to be pushed:
echo   62127b6 - docs: Add missing FP research documentation
echo   13194f6 - docs: Add comprehensive testing checklist for Pattern 1 work
echo   ddfca3a - docs: Update testing report with Pure FP v3 evolution
echo   90e5508 - feat: Fix Pure FP v3 - Remove nested functions, add tail recursion
echo   cf6cca3 - refactor(linear-regression): Apply Pattern 1 (Array Statistics)
echo   1e753f7 - refactor(kmeans): Apply Pattern 1 (Array Statistics) to K-means
echo   e656b32 - refactor: Add Pattern 1 - Array Statistics Template
echo.
pause
echo.

REM Check if we're on main branch
git branch --show-current > current_branch.tmp
set /p CURRENT_BRANCH=<current_branch.tmp
del current_branch.tmp

if not "%CURRENT_BRANCH%"=="main" (
    echo ERROR: Not on main branch! Current branch: %CURRENT_BRANCH%
    echo Please checkout main branch first: git checkout main
    pause
    exit /b 1
)

echo Creating and pushing to branch: Claude_pattern1_pure_fp_refactoring
echo.

REM Option 1: Create branch and push
git checkout -b Claude_pattern1_pure_fp_refactoring
if errorlevel 1 (
    echo Branch may already exist, checking it out...
    git checkout Claude_pattern1_pure_fp_refactoring
    if errorlevel 1 (
        echo ERROR: Failed to checkout/create branch
        pause
        exit /b 1
    )
)

echo.
echo Pushing to remote...
git push origin Claude_pattern1_pure_fp_refactoring

if errorlevel 1 (
    echo.
    echo ERROR: Push failed!
    pause
    exit /b 1
)

echo.
echo ================================================================================
echo  SUCCESS! Pushed to: Claude_pattern1_pure_fp_refactoring
echo ================================================================================
echo.
echo You can now:
echo   1. Create a pull request on GitHub
echo   2. Review the changes online
echo   3. Merge when ready
echo.

REM Return to main branch
git checkout main

pause

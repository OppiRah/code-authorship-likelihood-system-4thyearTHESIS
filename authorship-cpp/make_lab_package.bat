@echo off
REM -------------------------------------------------------------
REM  CALSS - build the laboratory deployment package.
REM  Run compile.bat and make_installer.bat first.
REM
REM    make_lab_package.bat
REM
REM  Output (in release\):
REM    CALSS_<version>_Lab_Deployment\     installer, SHA256SUMS.txt,
REM                                        INSTALLATION_GUIDE.pdf,
REM                                        README.txt, RELEASE_NOTES.txt,
REM                                        CALSS_SampleData_SYNTHETIC.zip
REM    CALSS_<version>_Lab_Deployment.zip  the same folder zipped
REM
REM  Sources: release_docs\lab\ (README, release notes, guide as
REM  Markdown). Builder: tools\make_lab_package.py (needs Python 3 and
REM  reportlab). The installer is copied, never rebuilt or signed.
REM -------------------------------------------------------------
setlocal
cd /d "%~dp0"
python tools\make_lab_package.py || (echo ERROR: lab package build failed & exit /b 1)
endlocal

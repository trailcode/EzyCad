@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0check-nonascii-src.ps1" %*

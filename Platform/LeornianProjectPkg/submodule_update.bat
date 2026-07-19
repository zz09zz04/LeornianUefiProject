@echo off
for /f "tokens=2" %%S in ('git submodule status') do (
    echo Updating %%S
    git submodule update --init --recursive "%%S"
    if errorlevel 1 echo Skip %%S
)

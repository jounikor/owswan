@echo off
@rem   From IDE: pause=%1, name=%2, project=%3, target=%4
co -l -y! %2
if [%1]==[no_pause] goto done
pause
:done

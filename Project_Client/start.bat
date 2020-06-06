SET /A first=%random% %%100
SET /A second=%random% %%100
SET /A third=%random% %%100

start "Wireless Climate Contorl" "%~dp0Project_Client.exe" 127.0.0.1 9889 ard_%first%
start "Wireless Climate Contorl" "%~dp0Project_Client.exe" 127.0.0.1 9889 ard_%second%
start "Wireless Climate Contorl" "%~dp0Project_Client.exe" 127.0.0.1 9889 ard_%third%
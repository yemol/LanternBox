# FT-02 v2.75j1 Compile Fix

修复 `main.cpp` 中 `FT02_HandleDeviceStatusInput()` 在
`FT02_OpenCompassCalibrationPage()` 定义之前调用，且没有前置声明导致的编译错误。

修复：
`static void FT02_OpenCompassCalibrationPage();`

功能逻辑保持 v2.75j 不变。

@echo off
gcc test_purity.c fp_percentile_wrappers.c fp_core_percentiles.o fp_core_outliers.o -o test_purity.exe 2>&1

set terminal table; set output "dissertation.filesuffix.table"; set format "%.5f"
set format "%.7e";; set samples 25; plot [x=-5:5] (-x**5 - 242);

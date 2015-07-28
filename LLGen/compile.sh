
../Parser/llparser test.c ../Parser/lltable.txt>tree.txt
rc=$?;
if [[ $rc == 0 ]]; then
	./sc>tree.ll
fi

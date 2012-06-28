cd ../battle/
echo t 10007 > /proc/rdnice
cat /proc/rdnice
let total=0
for i in $(seq 20)
	do
	./new_iotest
	res=$(./log_infor)
	total=$((total+res))
	echo $i, $res
	done
echo $((total / 20))

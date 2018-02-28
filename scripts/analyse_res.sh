file=$1
tmpfile="/tmp/live-rusty-cat-result-`date +%s`"
fail_times=`grep 'total_count 0' $1 | wc -l`
grep 'address' -v 'total_count 0' $1 > $tmpfile
suc_times=`cat $tmpfile | wc -l`

avg_e2e=$(cat $tmpfile | cut -d ',' -f 7  | grep -v '^$' | cut -d ' ' -f 3 | awk '{sum+=$1} END {print sum/NR}')
avg_iframe=$(cat $tmpfile | cut -d ',' -f 2  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum/NR}')
total_cnt=$(cat $tmpfile | cut -d ',' -f 3  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum}')
nonf_cnt=$(cat $tmpfile | cut -d ',' -f 4  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum}')
avg_nf=0.0
if [ $total_cnt -gt 0 ]; then
    avg_nf=$(echo "$total_cnt $nonf_cnt" | awk '{print $2/$1}')
fi
echo -n `hostname`:
echo -n ' try_times ' $(($fail_times+$suc_times))
echo -n '; fail_times ' $fail_times
echo -n '; avg_iframe ' $avg_iframe
echo -n '; avg_e2e ' $avg_e2e
echo -n '; total_cnt ' $total_cnt
echo -n '; nonf_cnt ' $nonf_cnt
echo -n '; avg_nf ' $avg_nf

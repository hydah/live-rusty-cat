file=$1
tmpfile="/tmp/live-rusty-cat-result-`date +%s`"
echo $tmpfile
grep 'first arrival of' $1 | grep -v "nonfluency rate 100.00" > $tmpfile
avg_e2e=$(cat $tmpfile | cut -d ' ' -f 13  | grep -v '^$' | awk '{sum+=$1} END {print sum/NR}')

avg_iframe=$(cat $tmpfile | cut -d ' ' -f 7  | grep -v '^$' | awk '{sum+=$1} END {print sum/NR}')

avg_nf=$(cat $tmpfile | cut -d ' ' -f 11  | grep -v '^$' | awk '{sum+=$1} END {print sum/NR}')
echo `hostname`: 'avg_iframe =' $avg_iframe'; avg_e2e =' $avg_e2e'; avg_nf =' $avg_nf
